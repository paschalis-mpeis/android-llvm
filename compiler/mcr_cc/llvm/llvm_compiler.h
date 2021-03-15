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
#ifndef ART_COMPILER_LLVM_COMPILER_H_
#define ART_COMPILER_LLVM_COMPILER_H_

#include <cstdio>
#include <vector>
#include <set>
#include <string>

#ifdef ART_MCR
#define DISABLE_PASS_ON_LLVM(graph) \
  if((graph)->IsCompiledLLVMAny()) { \
    D2LOG(WARNING) << __func__ << ": disabled in LLVMany"; \
    return false; \
  } 
#define DISABLE_PASS_ON_LLVMok(graph) \
  if((graph)->IsCompiledLLVMok()) { \
    D2LOG(WARNING) << __func__ << ": disabled in LLVMok"; \
    return; \
  } 
#else
#define DISABLE_PASS_ON_LLVM(graph)
#define DISABLE_PASS_ON_LLVMok(graph)
#endif

namespace art {
class HInvoke;

namespace LLVM {

class LlvmCompiler final {
 public:
  static void Initialize();

#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
  static void SetGenerateInvokeHistogram() {
    gen_invoke_histogram_ = true;
  }

  static bool GenerateInvokeHistogram();
#endif

  static void LogError(std::string msg);
  static void LogWarning(std::string msg);
  static void WriteCompErr(std::string err);
  static bool PrintReport();

  static std::set<std::string> errors_;
  static std::set<std::string> warnings_;

 private:
  static bool gen_invoke_histogram_;
};

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_COMPILER_H_
