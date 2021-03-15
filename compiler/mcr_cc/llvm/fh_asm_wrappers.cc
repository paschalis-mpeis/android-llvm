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

#include <regex>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Metadata.h>

#include "llvm_utils.h"
#include "asm_arm64.h"
#include "asm_arm_thumb.h"
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"

#include "llvm_macros_irb.h"

using namespace ::llvm;

namespace art {
namespace LLVM {


Function* FunctionHelper::LoadStateAndFlags(
    HGraphToLLVM* HL, IRBuilder* irb) {
  if(func_load_state_and_flags_ != nullptr) return func_load_state_and_flags_;

  FunctionType *fTy=FunctionType::get(irb->getInt16Ty(), false);

  Function* f = Function::Create(
      fTy, Function::LinkOnceODRLinkage, __func__, irb->getModule());
  AddAttributesFastASM(f);

  D3LOG(INFO) << "Creating function: " << __func__;
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  BasicBlock* entry_block=BasicBlock::Create(irb->getContext(), "entry", f);
  irb->SetInsertPoint(entry_block);
  Value* state_and_flags=Arm64::LoadStateAndFlagsASM(irb);
  state_and_flags->setName("state_and_flags");
  irb->CreateRet(state_and_flags);

  irb->SetInsertPoint(pinsert_point);
  func_load_state_and_flags_ = f;
  return f;
}

// Make also a call?? in hgraph
Function* FunctionHelper::GetQuickEntrypoint(
    HGraphToLLVM* HL, IRBuilder* irb, QuickEntrypointEnum qpoint) {
  if (art_entrypoints_.find(qpoint) != art_entrypoints_.end()) {
    return art_entrypoints_[qpoint];
  }

  FunctionType* fTy = FunctionType::get(
      irb->getJLongTy()->getPointerTo(), false);

  std::stringstream ss;
  ss << qpoint;

  // GetArtCallTestSuspend
  std::string name = 
    "GetEntrypoint" + std::regex_replace(ss.str(), std::regex("ArtCall"), "");

  Function* f = Function::Create(
      fTy, Function::LinkOnceODRLinkage, name, irb->getModule());
  AddAttributesFastASM(f);

  D3LOG(INFO) << "Creating function: " << __func__;
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  BasicBlock* entry_block=BasicBlock::Create(irb->getContext(), "entry", f);
  irb->SetInsertPoint(entry_block);
  Value* e= Arm64::GetQuickEntrypoint(irb, qpoint);
  e->setName("quick" + std::to_string(static_cast<int>(qpoint)));
  irb->CreateRet(e);

  irb->SetInsertPoint(pinsert_point);
  art_entrypoints_[qpoint] = f;
  return f;
}

Function* FunctionHelper::LoadThread(HGraphToLLVM* HL, IRBuilder* irb) {

  if(func_load_thread_ != nullptr) return func_load_thread_;

  FunctionType *fTy =
    FunctionType::get(irb->getVoidPointerType(), false);

  Function* f = Function::Create(
      fTy, Function::LinkOnceODRLinkage, __func__, irb->getModule());
  AddAttributesFastASM(f);

  D3LOG(INFO) << "Creating function: " << __func__;
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  BasicBlock* entry_block =
    BasicBlock::Create(irb->getContext(), "entry", f);

  irb->SetInsertPoint(entry_block);
  Value* thread_register = nullptr;
  InstructionSet isa = HL->GetLlvmCompilationUnit()->GetInstructionSet();
  switch (isa) {
    case InstructionSet::kThumb2:
      thread_register = ArmThumb::GetThreadRegister(irb);
      break;
    case InstructionSet::kArm64:
      thread_register = Arm64::GetThreadRegister(irb);
      break;
    default:
      DLOG(FATAL) << "Unimplemented for architecture: " << isa;
      UNREACHABLE();
  }

  thread_register->setName("thread");
  irb->CreateRet(thread_register);

  irb->SetInsertPoint(pinsert_point);
  func_load_thread_ = f;
  return f;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
