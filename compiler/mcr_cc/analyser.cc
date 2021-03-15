/**
 * Static Dalvik bytecode analysis could be added here to decide which
 * methods to compile. This release uses McrCC::ReadCompileProfile to
 * find the list of methods to compile.
 *
 * profile.dbg_methods is another list of methods for debugging the backend,
 * as it emits more information during the IR-to-IR translation, and
 * during the execution.
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

#include "analyser.h"
#include "mcr_rt/mcr_rt.h"

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "class_linker-inl.h"
#include "dex/class_accessor-inl.h"
#include "dex/class_reference.h"
#include "dex/dex_file_loader.h"
#include "dex/dex_instruction-inl.h"
#include "mcr_cc/llvm/llvm_compiler.h"
#include "mcr_cc/os_comp.h"
#include "mcr_cc/linker.h"
#include "mcr_cc/match.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_rt/filereader.h"
#include "mcr_rt/oat_aux.h"
#include "mcr_rt/utils.h"
#include "mirror/class.h"
#include "thread.h"
#include "llvm/debug.h"

namespace art {
namespace mcr {

uint32_t methods_num_ = 0;
std::set<std::string> Analyser::dbg_methods_;
std::set<std::string> Analyser::cold_methods_;
std::set<std::string> Analyser::cold_methods_internal_;
std::set<std::string> Analyser::cant_compile_;
std::set<std::string> Analyser::llvm_compiled_;
std::set<std::string> Analyser::not_found_;
std::set<std::string> Analyser::histogram_additions_;
std::vector<const DexFile*> Analyser::dex_files_;

bool Analyser::HasDebugMethodsProfile() { return (dbg_methods_.size() > 0); }

/**
 * Used for printing additional debug information in LLVM.
 */
bool Analyser::IsInDebugMethodsProfile(std::string pretty_method) {
  for (std::string m : dbg_methods_) {
    if (m[m.length() - 1] == '*') {
      std::string rg = m.substr(0, m.length() - 1);
      if (pretty_method.find(rg) != std::string::npos) {
        return true;
      }
    } else {
      if (pretty_method.compare(m) == 0) {
        return true;
      }
    }
  }
  return false;
}

/**
 * Debug methods are used in compilation with llvm Debug option tools.
 * e.g. to restrict printing basic block debug info only
 * for particular methods
 */
void Analyser::ReadDebugMethodsProfile() {
  std::string filename = std::string(McrCC::GetProfileMain()) +
                         ".dbg_methods";
  FileReader fr(__func__, filename, false, true, true);
  if (fr.exists()) {
    std::vector<std::string> data = fr.GetData();
    dbg_methods_.insert(data.begin(), data.end());
  }
}

void Analyser::AddToLlvmCompiled(std::string pretty_method) {
  llvm_compiled_.insert(pretty_method);
}

void Analyser::AddColdMethod(std::string m) {
  D2LOG(INFO) << __func__ << ": "  << m;
  cold_methods_.insert(m);
}

bool Analyser::IsHotMethod(std::string hf) {
  for (std::string method : McrRT::hot_functions_) {
    if (method.compare(hf) == 0) { return true; }
  }
  return false;
}

void Analyser::PrintCompilationReport() {
  DLOG(WARNING)
    << " \n \n \n"
    << "+====================================================+\n"
    << "|---             Compilation Report               ---|\n"
    << "+====================================================+";

  //////////////////////
  // IMPORTANCE: LOW
  //////////////////////
  if (McrRT::hot_functions_.size() > 0) {
    DLOG(INFO) << "|- PROFILE.main: " << McrRT::hot_functions_.size();
    for (std::string method : McrRT::hot_functions_) {
      DLOG(INFO) << "| " << method;
    }
  }

  // Split cold (not compiled) methods to:
  // .ones that were already added to a profile, and still failed to compile
  // .constructors
  // .remaining: which must be manually added to a profile
  std::set<std::string> method_ctors;
  std::set<std::string> method_inprofile;
  std::set<std::string> method_toadd;
  std::set<std::string> method_osblocklist;
  std::set<std::string> method_osfailed;
  if (cold_methods_.size()> 0) {
    DLOG(WARNING) << "| Methods: cold" << cold_methods_.size()
      << " (total)";
    for (std::string method : cold_methods_) {
      if (method.find(MATCH_CTOR) != std::string::npos) {
        method_ctors.insert(method);
      } else if (IsHotMethod(method)) {
        method_inprofile.insert(method);
      } else if (OsCompilation::IsOsMethodsBlocklisted(method)) {
        method_osblocklist.insert(method);
      } else {
        // check that method is not in histogram
        bool in_hist=false;
        for(std::string histmethod: histogram_additions_) {
          if(histmethod.compare(method) == 0) {
            in_hist = true;
            break;
          }
        }
        if(!in_hist) method_toadd.insert(method);
      }
    }
  }

  if (method_inprofile.size() > 0) {
    DLOG(ERROR) << "|- Methods: cold: OSblocklist or OSfailed: "
      << "\n|-- why in blocklist?"
      << method_osblocklist.size();
    for (std::string method : method_osblocklist) {
      DLOG(WARNING) << "| " << method;
    }
  }

  if (method_inprofile.size() > 0) {
    DLOG(ERROR) << "|- Methods: cold: already in profile: "
      << method_inprofile.size()
      << "\n|-- investigate why not compiled";
    for (std::string method : method_inprofile) {
      DLOG(WARNING) << "| " << method;
    }
  }

  if (method_ctors.size() > 0) {
    DLOG(WARNING) << "|- Methods: COLD: Constructors: " << method_ctors.size();
    for (std::string method : method_ctors) {
      DLOG(WARNING) << "| " << method;
    }
  }

  if (cold_methods_internal_.size() > 0) {
    DLOG(ERROR) << "|- Methods: COLD: Internal: "
      << cold_methods_internal_.size();
    for (std::string method : cold_methods_internal_) {
      DLOG(WARNING) << "| " << method;
    }
  }

  //////////////////////
  // IMPORTANCE: MEDIUM
  //////////////////////
  if (dbg_methods_.size() > 0) {
    DLOG(ERROR) << "|- PROFILE.dbg_methods: " << dbg_methods_.size();
    for (std::string method : dbg_methods_) {
      DLOG(ERROR) << "| " << method;
    }
  }

  //////////////////////
  // IMPORTANCE: HIGH
  //////////////////////
  // NOTE: appending might make sense only with some other code that is not part of the 
  // LLVM backend release
  std::string auto_add_msg = (McrCC::auto_update_profiles?
      "AUTOMATICALLY APPENDED!" :"MANUALLY ADD!");
  const std::string msg_backup = "| *** Make sure to BACKUP the profile! ***";

  if (cant_compile_.size() > 0) {
    DLOG(ERROR) << "| Methods: CANT COMPILE: " << cant_compile_.size();
    for (std::string method : cant_compile_) {
      DLOG(ERROR) << "| " << method;
    }
  }

  // Methods: not found
  if (not_found_.size() > 0) {
    DLOG(ERROR) << "|= Methods: NOT FOUND: " << not_found_.size();
    for (std::string method : not_found_) {
      DLOG(ERROR) << "| " << method;
    }
  }

  if (method_toadd.size()> 0) {
    DLOG(ERROR) << "|= Methods: COLD: " << method_toadd.size()
    << ": " << auto_add_msg;
    for (std::string method : method_toadd) {
      DLOG(ERROR) << "| " << method;
    }
    if(McrCC::auto_update_profiles) {
      McrCC::AppendToProfile(method_toadd);
      DLOG(ERROR) << msg_backup;
    }
  }

  if (histogram_additions_.size() > 0) {
    DLOG(WARNING) << "|- Histogram: new additions:" << histogram_additions_.size();
    for (std::string pretty_method : histogram_additions_) {
      DLOG(WARNING) << pretty_method;
    }
  } else {
    DLOG(INFO) << "|- Histogram: no new additions";
  } 

  LLVM::LlvmCompiler::PrintReport();

  if (mcr::McrCC::compilation_warnings_.size() > 0) {
    DLOG(ERROR) << "|- Other compiler warnings:" <<
      mcr::McrCC::compilation_warnings_.size();
    for (std::string w: mcr::McrCC::compilation_warnings_) {
      DLOG(ERROR) << "| " << w;
    }
  }

  DLOG(WARNING)
    << "+====================================================+\n"
    << "|---                 end of report                ---|\n"
    << "+====================================================+\n"
    << " \n \n";
}

inline void PrintMethodInfo(const dex::ClassDef& declaring_class_def,
    std::string pretty_class, const MethodReference& method_ref,
    const ClassAccessor::Method& method, bool isDirect)
      REQUIRES_SHARED(Locks::mutator_lock_) {

  std::string pretty_caller = method_ref.PrettyMethod();
  std::string package(McrRT::GetPackage());
  methods_num_++;
  UNUSED(declaring_class_def);
  UNUSED(pretty_class);

  std::stringstream ss;
  ss << "| Method:" << method_ref.index << ":" << pretty_caller;
  ss << ":" << method_ref.index;

  const dex::CodeItem* code_item = method.GetCodeItem();
  if (code_item == nullptr) {  // abstract or native methods
    ss << " [CodeItem null]";
  } else {
    CodeItemDataAccessor codeItemDA = method.GetInstructionsAndData();
    if (codeItemDA.TriesSize() > 0) {
      ss << " [try-catch]";
    }
  }
  ss << (isDirect ? ":Direct" : "");
  DLOG(INFO) << ss.str();
}

void Analyser::PrintAppMethods(jobject jclass_loader,
    const std::vector<const DexFile*>& dex_files) {
  D5LOG(INFO) << __func__;
  for (const DexFile* dex_file : dex_files) {
    dex_files_.push_back(dex_file);
  }

  ClassLinker* const class_linker = Runtime::Current()->GetClassLinker();
  int dfi = 0;
  DLOG(INFO) << "|-- DexFiles: " << dex_files_.size();
  for (const DexFile* dex_file : dex_files_) {
    CHECK(dex_file != nullptr);
    DLOG(INFO) << "| DexFile:" << ++dfi << ":Location: "
      << DexFileLoader::GetBaseLocation(dex_file->GetLocation())
      << " Classes: " << dex_file->NumClassDefs();

    int cli = 1;
    for (size_t j = 0; j != dex_file->NumClassDefs(); ++j, ++cli) {
      ClassReference ref(dex_file, j);
      const dex::ClassDef& class_def = dex_file->GetClassDef(j);
      ClassAccessor accessor(*dex_file, j);

      // Use a scoped object access to perform to the quick SkipClass check.
      ScopedObjectAccess soa(Thread::Current());
      StackHandleScope<3> hs(soa.Self());
      Handle<mirror::ClassLoader> class_loader(
          hs.NewHandle(soa.Decode<mirror::ClassLoader>(jclass_loader)));
      Handle<mirror::Class> klass(
          hs.NewHandle(class_linker->FindClass(soa.Self(),
              accessor.GetDescriptor(), class_loader)));

      if (klass == nullptr) continue;

      std::string pretty_class = klass->PrettyClass();


      // Compile direct and virtual methods.
      int64_t previous_method_idx = -1;

      // no methods to compile
      if (accessor.NumDirectMethods() + accessor.NumVirtualMethods() == 0) {
        continue;
      }

      for (const ClassAccessor::Method& method : accessor.GetMethods()) {
        const uint32_t method_idx = method.GetIndex();
        if (method_idx == previous_method_idx) {
          // smali can create dex files with two encoded_methods
          // sharing the same method_idx
          // http://code.google.com/p/smali/issues/detail?id=119
          continue;
        }
        previous_method_idx = method_idx;
        MethodReference method_ref(dex_file, method_idx);
        PrintMethodInfo(class_def, pretty_class,
            method_ref, method, false);
      }  // for each method
    }    // for classes
  }      // for dex_files
}

}  // namespace mcr
}  // namespace art
