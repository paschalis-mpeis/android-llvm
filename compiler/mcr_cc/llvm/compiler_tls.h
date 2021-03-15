/**
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
#ifndef AR_COMPILER_LLVM_COMPILER_TLS_H_
#define AR_COMPILER_LLVM_COMPILER_TLS_H_

namespace art {
namespace LLVM {

// Thread-local storage compiler worker threads
class CompilerTls {
 public:
  CompilerTls()
      : llvm_info_(nullptr) {
  }
  ~CompilerTls() { }

  void* GetLLVMInfo() { return llvm_info_; }

  void SetLLVMInfo(void* llvm_info) {
    llvm_info_ = llvm_info;
  }

  static CompilerTls* GenerateCompilerTls() {
    return new CompilerTls();
  }

 private:
  void* llvm_info_;
};

}  // namespace LLVM
}  // namespace art

#endif  // AR_COMPILER_LLVM_COMPILER_TLS_H_
