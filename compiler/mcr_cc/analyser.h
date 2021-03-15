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
 */
#ifndef ART_COMPILER_MCR_ANALYSER_H_
#define ART_COMPILER_MCR_ANALYSER_H_

#include "base/timing_logger.h"
#include "dex/dex_file.h"
#include "dex/dex_instruction.h"

#define REPLAY_BASELINE "baseline"
#define REPLAY_IC "ic"

namespace art {

namespace mirror {
class Class;
}
namespace mcr {

extern uint32_t methods_num_;

class Analyser final {
 public:
  static void PrintAppMethods(jobject jclass_loader,
                              const std::vector<const DexFile*>& dex_files)
      REQUIRES(!Locks::mutator_lock_);

  static void AddColdMethod(std::string m);

  static void AddColdMethodInternal(std::string m) {
    cold_methods_internal_.insert(m);
  }

  static void PrintCompilationReport();
  static void AddToLlvmCompiled(std::string pretty_method);
  static void AddToDontCompileInUse(std::string method);
  static std::set<std::string> not_found_;
  static std::set<std::string> histogram_additions_;
  static bool IsInDebugMethodsProfile(std::string pretty_method);
  static bool HasDebugMethodsProfile();

  static bool IsHotMethod(std::string hf);

 private:
  static void ReadDebugMethodsProfile();
  
  static std::set<std::string> dbg_methods_;
  static std::set<std::string> cold_methods_;
  static std::set<std::string> cold_methods_internal_;
  static std::vector<const DexFile*> dex_files_;

  static std::set<std::string> cant_compile_;
  static std::set<std::string> llvm_compiled_;

  Analyser();
  ~Analyser();
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_ANALYSER_H_
