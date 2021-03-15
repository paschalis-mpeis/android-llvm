/**
 * Most generic utilities of the mcr_cc submodule.
 *
 * Copyright (C) 2021  Paschalis Mpeis (paschalis.mpeis-AT-gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "mcr_cc/mcr_cc.h"
#include "mcr_rt/mcr_rt.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>

// string_format
#include <memory>
#include <string>
#include <stdexcept>

#include "art_method-inl.h"
#include "art_method.h"
#include "base/logging.h"
#include "base/os.h"
#include "dex/code_item_accessors.h"
#include "mcr_cc/analyser.h"
#include "mcr_cc/llvm/debug.h"
#include "mcr_cc/match.h"
#include "mcr_cc/pass_manager.h"
#include "mcr_rt/filereader.h"
#include "optimizing/nodes.h"

namespace art {
namespace mcr {

long McrCC::compiled_optimizing_=0;
std::string McrCC::llvm_entrypoint_function_;
bool McrCC::recompile_=false;
std::set<std::string> McrCC::recompile_reasons_;
std::set<std::string> McrCC::compilation_warnings_;
const bool McrCC::auto_update_profiles=true;

template<typename ... Args>
inline std::string string_format( const std::string& format, Args ... args )
{
    // Extra space for '\0'
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    // if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}


void McrCC::StoreMethodBytecode(const MethodReference method_ref,
                                const dex::CodeItem* code_item) {
  std::string method = method_ref.PrettyMethod();
  D3LOG(INFO) << "StoreMethodBytecode(): " << method;
  std::string outFilename(GetFileSrc(method, FILE_BYTECODE));
  umask(0);
  std::ofstream out(outFilename, std::ofstream::app);

  // Ignore empty methods, like the ones of support libraries
  if (code_item == nullptr) {
    DLOG(ERROR) << "StoreMethodBytecode(): Empty is method: " << method;
    return;
  }

  CodeItemDataAccessor acc(*method_ref.dex_file, code_item);

  out << "# " << method << "\n";

  std::string pkg(McrRT::GetPackage());
  const uint16_t* code_ptr = acc.Insns();
  const uint16_t* code_end = acc.Insns() + acc.InsnsSizeInCodeUnits();

  // Iterate all over the instructions
  uint32_t dex_pc = 0;
  while (code_ptr < code_end) {
    const Instruction& instruction = *Instruction::At(code_ptr);

    dex_pc += instruction.SizeInCodeUnits();
    code_ptr += instruction.SizeInCodeUnits();

    std::string cmd(instruction.DumpString(method_ref.dex_file));
    // add a hash before the calls to other functions of the app
    if (instruction.IsInvoke()) {
      // find boundaries of pkg_method
      std::size_t e = cmd.find('(');
      std::size_t s = cmd.rfind(' ', e);  // last space
      std::string pkg_method(cmd.substr(s + 1, e - s - 1));

      // if method belongs to the application add it to the graph
      if (!pkg_method.substr(0, pkg.size()).compare(pkg)) {
        out << cmd << " // [CALL]\n";
        continue;
      }
    }
    out << cmd << std::endl;
  }

  out.close();
  chmod(outFilename.c_str(), 0666);
}

/**
 * @brief Read the profile that contains the list of methods to compile.
 *        All these methods constitute a hot region, and the first
 *        method is the entrypoint to that region.
 *
 * This had more complex configuration files that were driving a static
 * bytecode analysis. That analysis is a string-based and slow, therefore
 * it was not published.
 */
void McrCC::ReadLlvmProfile() {
  std::string filename = GetProfileMain();
  D2LOG(INFO) << __func__ << ": " << filename;

  uint32_t maxHFs = 500;
  uint32_t addedMethods=0;

  if (!OS::FileExists(filename.c_str())) {
    DLOG(FATAL) << "ERROR: LLVM profile not found:" << filename;
    exit(EXIT_FAILURE);
  }

  mcr::FileReader fr(__func__, filename, false, true, true);
  if (fr.exists()) {
    std::vector<std::string> data = fr.GetData();
   
    for(std::string method: data) { 
      // ignore empty lines and comments
      if(method.size() == 0) continue;
      else if (method.rfind("#", 0) == 0) continue;

      addedMethods++;
      if(addedMethods <= maxHFs) {
        // Add method to profile
        McrRT::hot_functions_.insert(method);

        // the first method is the entrypoint
        if(addedMethods==1) SetLlvmEntrypoint(method);

      } else { 
        DLOG(ERROR) << "maxHFs reached: skipping: " << method;
      }
    }
  }

  if (addedMethods == 0) {
    DLOG(ERROR) << "ERROR: LLVM profile empty:" << filename;
    exit(EXIT_FAILURE);
  }

  DLOG(INFO) << "LLVM entrypoint method: '" << GetLlvmEntrypoint() << "'";
}

bool McrCC::isHot(std::string pretty_method) {
  // here the logic to decide whether a method is hot or not can be put.
  // In this version it simply compares against the compile profile.
  for(std::string hf: McrRT::hot_functions_) {
    if(pretty_method.compare(hf) == 0) {
      return true;
    }
  }

  return false;
}

/**
 * Appends to compilation profile from histogram additions
 *
 * Not tested with the released code.
 */
void McrCC::AppendToProfile(std::set<std::string> newmethods) {
  if(newmethods.size()>0) {
    std::string filename = GetProfileMain();
    D2LOG(INFO) << __func__ << ": newmethods: " << newmethods.size()
      << ": "<< filename;


    std::set<std::string> all_methods(newmethods);

    // read old methods
    uint32_t orig_size = 0;

    mcr::FileReader fr(__func__, filename, false, true, true);
    if (fr.exists()) {
      std::vector<std::string> data = fr.GetData();
      orig_size = data.size();
      // append old methods
      all_methods.insert(data.begin(), data.end());
    }

    std::ofstream profile(filename, std::ofstream::app);
    for(std::string method: all_methods) {
      profile << method;
      // added = true;
    }
    if(orig_size < all_methods.size()) {
      AddRecompilationReason("Appended methods profile.");
    }
    profile.close();
  }
}

void McrCC::AddRecompilationReason(std::string reason) {
  recompile_ = true;
  recompile_reasons_.insert(reason);
}

inline void PrintCentered(std::stringstream &ss, std::string str, int width) {
  int spaces=(width-str.size())/2;
  ss << "\n||" << std::string(spaces, ' ');
  ss << str;
  ss << std::string(spaces, ' ');
  if(spaces*2+str.size()<(unsigned)width) ss << " ";
  ss << "||";
}

void McrCC::DieWithRecompilationReport() {
  if(recompile_) {
    const int linewidth = 50;
    std::stringstream ss;
    ss << "\n++" << std::string(linewidth, '=') << "++";
    PrintCentered(ss, "RECOMPILATION NEEDED:", linewidth);
    for(std::string reason: mcr::McrCC::recompile_reasons_) {
      PrintCentered(ss, reason, linewidth);
    }
    ss << "\n++" << std::string(linewidth, '=') << "++";

    DLOG(ERROR) << ss.str();

    McrDebug::SetRecompilation();
    exit(EXIT_FAILURE);
  }
}

/**
 * Each line of this file contains methods (in string format)
 * to be compiled by the LLVM backend.
 */
std::string McrCC::GetProfileMain() {
  std::string s=GetFileApp(F_LLVM_PROFILE);
  return s;
}

std::string McrCC::GetBitcodeFilename(bool is_outer, std::string postfix) {
  std::string s = FILE_BITCODE_PREFIX;
  if (postfix.size()) {
    s += "." + postfix;
  }
  s += std::string(".") + (is_outer ? "outer" : "inner");
  s += "." FILE_BITCODE_EXT;
  return s;
}

/**
 * @brief Very similar to mcr_rt/utils.h but modified for llvm bitcode
 */
const char* McrCC::EscapeNameForLLVM(std::string hf) {
  static const struct {
    const char before;
    const char after;
  } match[] = {
    { '/', '-' }, { ';', '_' }, { ' ', '_' }, { '$', '_' }, { '(', '_' }, 
    { ')', '_' }, { '<', '_' }, { '>', '_' }, { '[', '-' }, { ']', '-' }
  };

  for (unsigned int i = 0; i < sizeof(match) / sizeof(match[0]); i++) {
    std::replace(hf.begin(), hf.end(), match[i].before, match[i].after);
  }

  return hf.c_str();
}

void McrCC::LogLongMessage(std::string msg, LogSeverity severity) {
  const int chunk_size = 300;
  for (unsigned i = 0; i < msg.length(); i += chunk_size) {
    LOG(severity) << msg.substr(i, chunk_size);
  }
}

std::string McrCC::GetClassName(std::string pretty_method) {
  std::string t1 = pretty_method.substr(0, pretty_method.find("("));
  std::string t2 = t1.substr(0, t1.find_last_of("."));

  return t2.substr(t1.find(" ") + 1);
}

std::string McrCC::remove_extra_whitespaces(const std::string& input) {
  std::string output;
  output.clear();  // unless you want to add at the end of existing sring...
  unique_copy(input.begin(), input.end(), std::back_insert_iterator<std::string>(output),
              [](char a, char b) { return isspace(a) && isspace(b); });
  return output;
}

void McrCC::StoreMethodsToFile(std::set<std::string> methods, std::string filename){
  std::ofstream out(filename);
  for(std::string m: methods) {
    out << m << std::endl;
  }
  out.close();
}

bool McrCC::IsLlvmEntrypoint(std::string method) {
  return (method.compare(llvm_entrypoint_function_) == 0);
}

std::string McrCC::PrettyMethod(art::HGraph* graph) {
      MethodReference mref(&graph->GetDexFile(), graph->GetMethodIdx());
      return mref.PrettyMethod();
}

static bool endsWith(const std::string& str, const std::string& suffix)
{
  return str.size() >= suffix.size() &&
    0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

std::string McrCC::PrettyDexFile(std::string sdexFile) {
  if(endsWith(sdexFile, "base.apk")) {
    std::string pkg(McrRT::GetPackage());
    if(sdexFile.find(pkg) != std::string::npos) {
      return "base.apk";
    }
    return sdexFile;
  } else if(endsWith(sdexFile, "core-oj.jar")) {
    return "core-oj";
  }
  return sdexFile;
}

std::string McrCC::PrettyHistogramHeader() {
  std::stringstream ss;
  ss << string_format("%3s ", "#")
    << string_format("%-10s ", "CallerDF")
    << string_format("%-10s ", "CalleeDF")
    << string_format("%4s ", "PC")
    << string_format("%8s ", "From")
    << string_format("%8s ", "To")
    << string_format("%8s ", "To cls")
    << string_format("%-8s ", "Type")
    << string_format("%8s ", "Invokes")
    << string_format("%-8s ", "Comp");

  return ss.str();
}

std::string McrCC::PrettyHistogramLine(mcr::InvokeInfo ii, int cnt) {
  std::stringstream ssit;
  ssit << ii.GetSpecInvokeType();
  std::string dexlocCaller = PrettyDexFile(ii.GetDexLocationCaller());
  std::string dexloc = PrettyDexFile(ii.GetDexLocation());
  std::string dexfile = PrettyDexFile(ii.GetDexFilename());

  // assuming a static call (os methods)
  std::string callerIdx = "static"; 
  if(ii.GetCallerMethodIdx() != 0) {
    callerIdx = std::to_string(ii.GetCallerMethodIdx());
  }

  std::stringstream ss;
  ss << string_format("%3d ", cnt)
    << string_format("%-10s ", dexlocCaller.c_str())
    << string_format("%-10s ", dexfile.c_str())
    << string_format("%4d ", ii.GetDexPC())
    << string_format("%8s ", callerIdx.c_str())
    << string_format("%8d ", ii.GetSpecMethodIdx())
    << string_format("%8d ", ii.GetSpecClassIdx())
    << string_format("%-8s ", ssit.str().c_str())
    << string_format("%8d ", ii.GetInvokeTimes())
    << string_format("%-8s ", (ii.IsInternalMethod()?"internal":"normal"));

  if(dexloc.compare(dexfile) != 0) {
    ss << "\nDexLoc different: " << dexloc;
  }

  return ss.str();
}

}  // namespace mcr
}  // namespace art
