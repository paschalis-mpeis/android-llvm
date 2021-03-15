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
#include "function_helper.h"
#include <llvm/IR/Argument.h>
#include <llvm/IR/Function.h>

#include "asm_arm64.h"
#include "hgraph_to_llvm.h"
#include "ir_builder.h"

#include "llvm_macros_irb.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

bool FunctionHelper::IsSuspendCheck(Function* F) {
    return F == func_suspend_check_; 
}

Function* FunctionHelper::SuspendCheckASM(
    HGraphToLLVM* HL,
    IRBuilder* irb,
    InstructionSet isa, HBasicBlock* successor) {
  // successor is utilized by quick because it builds additional
  // BBs for calling and returning from SC
  UNUSED(successor);

  if(func_suspend_check_ != nullptr) return func_suspend_check_;

  D3LOG(INFO) << "Creating function: SuspendCheck";
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  FunctionType* suspendCheckTy=FunctionType::get(irb->getJVoidTy(), false);

  std::string name = "SuspendCheck";
  Function *f = Function::Create(
      suspendCheckTy, Function::LinkOnceODRLinkage, name, irb->getModule());
  f->setDSOLocal(true);
  AddAttributesSuspendCheck(f);

  func_suspend_check_ = f;

  BasicBlock* entry_block =
    BasicBlock::Create(irb->getContext(), "entry", f);
  BasicBlock* perform_check =
    BasicBlock::Create(irb->getContext(), "test_suspend", f);
  BasicBlock* skip_check =
    BasicBlock::Create(irb->getContext(), "skip", f);
  irb->SetInsertPoint(entry_block);

  Value* state_and_flags=irb->CreateCall(LoadStateAndFlags(HL, irb));

  MDNode *BranchWeights=HL->MDB()->createBranchWeights(MAX_BRWEIGHT, 0);
  Value* is_zero = irb->mCreateCmpEQ(false,
      state_and_flags, irb->getJUnsignedInt16(0));

  irb->CreateCondBr(is_zero, skip_check, perform_check,  BranchWeights);

  irb->SetInsertPoint(perform_check);
  VERIFY_LLVMD3("perform_check");

  // INFO A debug option that is no longer in use!
  // We used to do way more suspend checks than ART
  // this was because we were loading state_and_flags
  // into IP/r12, and then checking on r12, just like
  // Optimizing backend. However, r12 is used differently
  // in LLVM and this caused too many testSuspend calls.
  // We now (in llvm inline asm) simply load state_and_flags
  // from Thread/r9 to an llvm reg, and check from there!
  if(!McrDebug::SkipSuspendCheck()) {
#ifdef ART_MCR_ANDROID_10
    HL->ArtCallTestSuspend();
#elif defined(ART_MCR_ANDROID_6)
    irb->CreateCall(__TestSuspend());
#endif
  }
  irb->CreateRetVoid();

  irb->SetInsertPoint(skip_check);
  irb->CreateRetVoid();

  irb->SetInsertPoint(pinsert_point);

  MDNode* N=MDNode::get(irb->getContext(), MDString::get(irb->getContext(),
        "Android Unroll optimization: SuspendCheck"));
  f->setMetadata("android.check.suspend", N);

  return f;
}
#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
