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
#ifndef ART_COMPILER_LLVM_INFO_H_
#define ART_COMPILER_LLVM_INFO_H_

#include <llvm/IR/IRBuilder.h>

using namespace ::llvm;
namespace art {
namespace LLVM {

class LLVMInfo {
 public:
  LLVMInfo();
  ~LLVMInfo();

  LLVMContext* GetLLVMContext() {
    return llvm_context_.get();
  }

 private:
  std::unique_ptr< LLVMContext> llvm_context_;
};

class LlvmInserter: public IRBuilderDefaultInserter {
 public:
  LlvmInserter() : node_(NULL) { }

  void InsertHelper(::llvm::Instruction* I, const Twine& Name,
                    BasicBlock* BB,
                    BasicBlock::iterator InsertPt) const {
    IRBuilderDefaultInserter::InsertHelper(I, Name, BB, InsertPt);
    if (node_ != NULL) {
      // set particular metadata here (if needed)
    }
  }

  void SetDexOffset(MDNode* node) { node_ = node; }

 private:
  MDNode* node_;
};

typedef ::llvm::IRBuilder<ConstantFolder, LlvmInserter> LLVMIRBuilder;

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_INFO_H_
