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
#include "fh_instanceOf-inl.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Metadata.h>
#include "asm_arm64.h"
#include "base/logging.h"
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"
#include "ir_builder.h"
#include "ir_builder.h"
#include "stack.h"
#include <sstream>

#include "llvm_macros_IRBc.h"


using namespace ::llvm;

namespace art {
namespace LLVM {

Function* FunctionHelper::ArrayGetMaybeCompressedChar(
    HGraphToLLVM* HL, IRBuilder* IRB, HArrayGet* h) {
  D3LOG(INFO) << __func__;

  const DataType::Type array_type = h->GetType();
  HInstruction* index = HL->GetArrayIndex(h);
  uint32_t offset = CodeGenerator::GetArrayDataOffset(h);
  const bool is_constant = index->IsConstant();
  const bool is_bakerRead = 
    (array_type == DataType::Type::kReference &&
     kEmitCompilerReadBarrier && kUseBakerReadBarrier);
  const bool maybe_compressed_char_at =
    mirror::kUseStringCompression && h->IsStringCharAt();
  CHECK(!is_bakerRead) << "Should be handled by VisitArrayGet";
  CHECK(maybe_compressed_char_at) << "Only for MaybeCompressedCharAt";

  std::string name = "ArrayGetMaybeCompressedCharAt";
  { std::stringstream ss;
    ss << (is_constant?"Constant":"Dynamic")<< offset;
    name+=ss.str();
  }

  if (array_get_[name] != nullptr) return array_get_[name];
  BasicBlock* pinsert_point = IRB->GetInsertBlock();
  VERIFY_LLVMD(name);

  std::vector<Type*> argsTy {IRB->getVoidPointerType()};
  if(!is_constant) {
    argsTy.push_back(IRB->getJIntTy());
  }
  FunctionType* ty =
    FunctionType::get(IRB->getType(array_type), argsTy, false);
  Function* func = Function::Create(
      ty, Function::LinkOnceODRLinkage, name, IRB->getModule());
  array_get_[name] = func;
  // func->addFnAttr(Attribute::AlwaysInline);
  func->setDSOLocal(true);
  AddAttributesCommon(func);

  // process arguments
  Function::arg_iterator arg_iter(func->arg_begin());
  Value* argArrayObj= &*arg_iter++;
  argArrayObj->setName("arrayObj");
  Value* argIndex = nullptr;
  if(!is_constant) {
    argIndex = &*arg_iter++;
    argIndex->setName("index");
  }

  LLVMContext& ctx = IRB->getContext();
  BasicBlock* bbEntry = BasicBlock::Create(ctx, "entry", func);
  IRB->SetInsertPoint(bbEntry);

  // General case.
  Value* length = nullptr;
  uint32_t count_offset = mirror::String::CountOffset().Uint32Value();
  // length = temps.AcquireW();
  length = HL->LoadWord<false>(argArrayObj, count_offset);
  // __ Ldr(length, HeapOperand(obj, count_offset));
  Arm64::MaybeRecordImplicitNullCheck(h);

  // Test bit and branch if nonzero
  // __ Tbnz(length.W(), 0, &uncompressed_load);
  BasicBlock* bbUncompressedLoad = BasicBlock::Create(
      IRB->getContext(), "char_uncompressed_load", func);
  BasicBlock* bbNormalLoad = BasicBlock::Create(
      IRB->getContext(), "char_normal_load", func);

  Value* zeroBit = IRB->CreateTrunc(length, IRB->getInt1Ty());
  Value*  zeroVal = IRB->getInt1(false);
  Value* cmp = IRB->mCreateCmpNE(false, zeroBit, zeroVal);
  IRB->CreateCondBr(cmp, bbUncompressedLoad, bbNormalLoad);

  if (is_constant) {
    // tbnz logic moved outside

    // Normal load
    IRB->SetInsertPoint(bbNormalLoad);
    {
      uint32_t nOffset = offset;
      // __ Ldrb(Register(OutputCPURegister(h)),
      //         HeapOperand(obj, offset + Int64FromLocation(index)));
      nOffset += HL->Int64FromLocation(index);
      Value* byte = HL->LoadByte<false>(argArrayObj, nOffset);
      byte = IRB->UpcastInt(byte, array_type);
      IRB->CreateRet(byte);
    }

    // Uncompressed laod
    IRB->SetInsertPoint(bbUncompressedLoad);
    {
      uint32_t cOffset = offset;
      // __ Ldrh(Register(OutputCPURegister(h)),
      // HeapOperand(obj, offset + (Int64FromLocation(index) << 1)));
      cOffset += HL->Int64FromLocation(index) << 1;
      Value* halfword = HL->LoadHalfWord<false>(argArrayObj, cOffset);
      halfword = IRB->UpcastInt(halfword, array_type);
      IRB->CreateRet(halfword);
    }
  } else { // dynamic
    CHECK(argIndex!=nullptr);
    // The below add is taken into account by GetDynamicOffset
    // __ Add(temp, obj, offset);
    // tbnz logic moved outside
    // Normal load
    IRB->SetInsertPoint(bbNormalLoad);
    {
      // __ Ldrb(Register(OutputCPURegister(h)),
      //         HeapOperand(temp, XRegisterFrom(index), LSL, 0));
      Value* dynOffset = HL->GetDynamicOffset(argIndex, 0, offset);
      Value* byte = HL->LoadByte<false>(argArrayObj, dynOffset);
      byte = IRB->UpcastInt(byte, array_type);
      IRB->CreateRet(byte);
    }

    // Uncompressed laod
    IRB->SetInsertPoint(bbUncompressedLoad);
    {
      // __ Bind(&uncompressed_load);
      // __ Ldrh(Register(OutputCPURegister(h)),
      //         HeapOperand(temp, XRegisterFrom(index), LSL, 1));
      Value* dynOffset = HL->GetDynamicOffset(argIndex, 1, offset);
      Value* halfword = HL->LoadHalfWord<false>(argArrayObj, dynOffset);
      halfword = IRB->UpcastInt(halfword, array_type);
      IRB->CreateRet(halfword);
    }
  }

  IRB->SetInsertPoint(pinsert_point);
  return func;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
