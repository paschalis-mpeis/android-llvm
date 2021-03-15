/**
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
#ifndef ART_COMPILER_MCR_CC_H_
#define ART_COMPILER_MCR_CC_H_

#include <map>
#include <set>
#include <android-base/logging.h>

#include "base/macros.h"
#include "dex/dex_file.h"
#include "dex/method_reference.h"
#include "mcr_rt/invoke_info.h"
#include "mcr_rt/mcr_rt.h"

// excluding nothing
#define SMALLEST_CODE_SIZE 0
#define MCR_MAX_HFS 500

#define F_LLVM_PROFILE "/profile"

#define COMP_TYPE_GEN_LLVM_BITCODE "llvm-gen-bitcode"
#define COMP_TYPE_LLVM_BASELINE "llvm-base"

#define HFlnkbc "hf.lnk.bc"
#define HFdcebc "hf.lnk.dce.bc"
#define HFoptbc "hf.lnk.opt.bc"
#define HFcc "hf.cc"
#define HFo "hf.o"
#define HFasm "hf.s"
#define HFso "hf.so"

#define FILE_PRETTY_LLVM_BITCODE_PREFIX "hf.pretty."
#define FILE_PRETTY_BITCODE_EXT ".ll"

#define FILE_BITCODE_PREFIX "hf"
#define FILE_BITCODE_EXT "bc"
#define FILE_HGRAPH_TO_C_PREFIX "hf"
#define FILE_HGRAPH_TO_C_EXT "cc"
#define FILE_BYTECODE "bytecode.txt"
#define FILE_MIR "mir.txt"
#define FILE_HGRAPH_PREFIX "hgraph"
#define FILE_HGRAPH_EXT "txt"

#define FILE_DEPS_LINK "deps.lnk"
namespace art {

class ArtMethod;
class HGraph;

namespace mcr {

class McrCC final {
 public:
  static void StoreMethodBytecode(const MethodReference method_ref,
                                  const dex::CodeItem* code_item);

  static void ReadLlvmProfile();
  static bool isHot(MethodReference m) { return isHot(m.PrettyMethod()); }
  static bool isHot(std::string pretty_method);
  static void AppendToProfile(std::set<std::string> methods);
  static void AddRecompilationReason(std::string reason);
  static bool MustRecompile() { return recompile_; }
  static void DieWithRecompilationReport();
  static std::string popTopHotFunction(std::set<std::string>* profileDataRaw);

  static std::string GetBitcodeFilename(bool is_outer, std::string postfix = "");
  static std::string GetOuterBitcodeFilename() {
    return GetBitcodeFilename(true);
  }
  static std::string GetInnerBitcodeFilename() {
    return GetBitcodeFilename(false);
  }

  /**
   * This method will be the outermost method of an LLVM hot region
   * (or the entrypoint to LLVM).
   * During an LLVM compilation, one such hot region is targetted.
   *
   * Multiple compilations could be performed with some adaptations
   * to the codethough, for generating LLVM bitcode for multiple hot regions.
   *
   * i.e., having a list of llvm_entrypoint methods, and a separate list
   * for the hot methods (which could also be derived with some static analysis,
   * or dynamic through the InvokeHistograms)
   */
  static void SetLlvmEntrypoint(std::string hf) {
    llvm_entrypoint_function_=hf;
  }

  static std::string GetLlvmEntrypoint() {
    return llvm_entrypoint_function_;
  }

  static std::string GetLlvmEntrypointStripped() {
    return StripHf(GetLlvmEntrypoint());
  }

  static bool IsLlvmEntrypoint(std::string method);

  static std::string GetProfileMain();

  static const char* EscapeNameForLLVM(std::string hf);
  static void LogLongMessage(std::string msg, android::base::LogSeverity severity);
  static std::string GetClassName(std::string pretty_method);
  static std::string remove_extra_whitespaces(const std::string& input);

  static void StoreMethodsToFile(
      std::set<std::string> methods,std::string filename);

  static std::string PrettyDexFile(std::string sdexFile);
  static std::string PrettyHistogramHeader();
  static std::string PrettyHistogramLine(mcr::InvokeInfo ii, int cnt);

  static std::string PrettyMethod(art::HGraph* graph);

  static long compiled_optimizing_;
  static bool recompile_;
  static std::set<std::string> recompile_reasons_;
  static std::set<std::string> compilation_warnings_;
  static const bool auto_update_profiles;

 private: 
  static std::string llvm_entrypoint_function_;
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_CC_H_
