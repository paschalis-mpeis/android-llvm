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

#include "asm_arm_thumb.h"

#include <llvm/IR/DerivedTypes.h>
#include "arch/arm/registers_arm.h"
#include "art_method.h"
#include "ir_builder.h"
#include "thread.h"

#include "llvm_macros_irb.h"

using namespace ::llvm;

namespace art {
namespace LLVM {
namespace ArmThumb {

void nop(IRBuilder* irb) {
  FunctionType* ty = FunctionType::get(irb->getVoidTy(), false);
  InlineAsm* nop = InlineAsm::get(ty, "nop;", "", true);
  irb->CreateCall(nop);
}

void SetThreadRegister(IRBuilder* irb, Value* thread_self) {
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType *asmTy=
    FunctionType::get(irb->getVoidTy(), params, false);
  std::vector<Value*> args;
  args.push_back(thread_self);

  arm::Register thread_register = arm::Register::TR;
  std::stringstream ss;
  ss << "mov " << thread_register << ", $0";

  D3LOG(INFO) << "SetThreadRegister: asm: " << ss.str();

  InlineAsm* setR9 = InlineAsm::
      get(asmTy, ss.str(), "r", false);
  irb->CreateCall(setR9, args);
}

Value* GetThreadRegister(IRBuilder* irb) {
  FunctionType* asmTy =
    FunctionType::get(irb->getVoidPointerType(), false);

  arm::Register thread_register = arm::Register::TR;
  std::stringstream ss;
  ss << "mov $0, " << thread_register;

  D3LOG(INFO) << "GetThreadRegister: asm: " << ss.str();

  InlineAsm* getR9 = InlineAsm::
    get(asmTy, ss.str(), "=r", false);

  return irb->CreateCall(getR9);
}

Value* LoadStateAndFlagsASM(IRBuilder* irb) {
  FunctionType *asmTy = FunctionType::get(irb->getInt16Ty(), false);
  arm::Register thread_register = arm::Register::TR;
  std::stringstream ss;

  uint32_t offset_state_and_flags =
    Thread::ThreadFlagsOffset<kArmPointerSize>().Int32Value();

  ss << "ldrh.w $0, [" << thread_register << ", #"
    << std::to_string(offset_state_and_flags) << "]";

  InlineAsm* ldrhw = InlineAsm::
    get(asmTy, ss.str(), "=r", false);
  return irb->CreateCall(ldrhw);
}

void GenerateMemoryBarrier(IRBuilder* irb, MemBarrierKind kind) {
  D1LOG(INFO) << "ArmThumb::GenerateMemoryBarrier: dmb " << kind;

  // LLVM IR: fence acquire;
  //  dmb     ishld
  std::string flavour = "";
  switch (kind) {
    case MemBarrierKind::kAnyStore:
    case MemBarrierKind::kLoadAny:
    case MemBarrierKind::kAnyAny: 
      flavour = "ish";
      break;
    case MemBarrierKind::kStoreStore: 
      flavour = "ishst";
      break;
    default:
      DLOG(FATAL) << "Unexpected memory barrier " << kind;
  }

  FunctionType *ty = FunctionType::get(irb->getVoidTy(), false);

  std::string dmb = "dmb " + flavour;
  InlineAsm* ia= InlineAsm::get(ty, dmb, "~{memory}", true);
  irb->CreateCall(ia);
}

#include "llvm_macros_undef.h"

}  // namespace ArmThumb
}  // namespace LLVM
}  // namespace art
