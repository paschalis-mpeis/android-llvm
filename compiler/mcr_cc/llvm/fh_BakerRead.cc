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

#include "llvm_macros_IRBc.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

constexpr uint32_t kReferenceLoadMinFarOffset = 16 * KB;

/**
 * @brief performs the actual load operations of the fast path
 */
ALWAYS_INLINE Value* _FieldLoadWithBakerReadBarrier(
    HGraphToLLVM* HL, IRBuilder* IRB, HInstruction* instruction,
    Value* lobj, Value* loffset,
    bool needs_null_check, bool use_load_acquire) {

  Value* fastVal=nullptr;
  {
    if (use_load_acquire) {
      // __ ldar(ref_reg, src);
      fastVal=Arm64::__Ldar(IRB, lobj, DataType::Type::kReference);
    } else {
      // __ ldr(ref_reg, src);
      fastVal=HL->LoadWord<true>(lobj, loffset);
    }
    if (needs_null_check) {
      Arm64::MaybeRecordImplicitNullCheck(instruction);
    }
    // Unpoison the reference explicitly if needed.
    // MaybeUnpoisonHeapReference() uses
    // macro instructions disallowed in ExactAssemblyScope.
    if (kPoisonHeapReferences) {
      // __ neg(ref_reg, Operand(ref_reg));
      VERIFIED("PoisonHeapReference");
      fastVal=Arm64::PoisonHeapReference(IRB, fastVal);
    }
  }
  HL->MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
  fastVal->setName("fastVal");
  return fastVal;
}

Function* FunctionHelper::FieldLoadWithBakerReadBarrier(
    HGraphToLLVM* HL, IRBuilder* IRB, HInstruction* instruction,
    bool needs_null_check, bool use_load_acquire) {
  BasicBlock* pinsert_point = IRB->GetInsertBlock();

  std::string name = "FieldLoad";
  if (use_load_acquire) name += "Acquire";
  name+="WithBakerReadBarrier";
  if (needs_null_check) name += "_NullCheck";
  if (baker_read_load_.find(name) != baker_read_load_.end()) {
    return baker_read_load_[name];
  }

  std::vector<Type*> argsTy {
    IRB->getJObjectTy()->getPointerTo(),  IRB->getJIntTy()};
  FunctionType* ty = FunctionType::get(
      IRB->getVoidPointerType(), argsTy, false);

  Function* f = Function::Create(
      ty, Function::LinkOnceODRLinkage, name, IRB->getModule());
  f->addFnAttr(Attribute::AlwaysInline);
  f->setDSOLocal(true);
  AddAttributesCommon(f);

  Function::arg_iterator arg_iter(f->arg_begin());
  Argument* lobjBase = &*arg_iter++;
  Argument* loffset = &*arg_iter++;
  lobjBase->setName("lobjBase");
  loffset->setName("offset");

  BasicBlock* entry_block =
    BasicBlock::Create(IRB->getContext(), "fast_path", f);
  BasicBlock* slow_path =
    BasicBlock::Create(IRB->getContext(), "slow_path", f);
  BasicBlock* resolved =
    BasicBlock::Create(IRB->getContext(), "resolved", f);

  IRB->SetInsertPoint(entry_block);

  Value* fastVal = _FieldLoadWithBakerReadBarrier(
    HL, IRB, instruction, lobjBase, loffset,
    needs_null_check, use_load_acquire);

  Value* is_null = IRB->CreateCmpIsNull(fastVal);
  MDNode *N=HL->MDB()->createBranchWeights(0, MAX_BRWEIGHT);
  IRB->CreateCondBr(is_null, slow_path, resolved, N);

  IRB->SetInsertPoint(slow_path);
  Value* nullref = IRB->getJNull();
  Value* slowVal = HL->GenerateReadBarrierSlow(
      instruction, nullref, lobjBase, loffset, nullptr);

  ANDROID_LOG_HEXD4(WARNING, HL, instruction, slowVal);
  IRB->CreateRet(slowVal);

  IRB->SetInsertPoint(resolved);
  IRB->CreateRet(fastVal);

  baker_read_load_[name] = f;
  IRB->SetInsertPoint(pinsert_point);
  return f;
}

/**
 * @brief calls Function FieldLoadWithBakerReadBarrier
 *
 */
Value* FunctionHelper::GenerateFieldLoadWithBakerReadBarrier(
    HGraphToLLVM* HL, IRBuilder* IRB, HInstruction* instruction,
    Value* lobj, uint32_t offset,
    bool needs_null_check, bool use_load_acquire) {
  // INFO disabled CHECK for kReference:
  // `GenerateReferenceLoad` might come from a non kReference HInstruction,
  // e.g. InstanceOf -> GenerateReferenceObjectClass->
  // GenerateReferenceLoad -> GenerateFieldLoadWithBakerReadBarrier
  // CHECK(instruction->GetType() == DataType::Type::kReference)
  //   << __func___ << "type must be object";

  Value* lbase = lobj;
  Type* baseTy =IRB->getType(DataType::Type::kReference)->getPointerTo();
  if (use_load_acquire) {
    VERIFY_LLVM("LoadAcquire");
    // base = WRegisterFrom(maybe_temp); // WReg?
    // __ Add(base, obj, offset);
    // base = HL->LoadFromObjectOffset(lobj, offset, baseTy);
    lbase= HL->LoadWord<true, false>(lobj, offset);
    offset = 0u;
  } else if (offset >= kReferenceLoadMinFarOffset) {
    CHECK_LLVM("FarOffset: Might have to use HL->Load?");
    // Or ptr operations?
    // base = WRegisterFrom(maybe_temp); // CHECK WReg?
    // __ Add(base, obj, Operand(offset & ~(kReferenceLoadMinFarOffset - 1u)));
    lbase=HL->LoadFromObjectOffset(lobj,
        offset & ~(kReferenceLoadMinFarOffset - 1u), baseTy);
    offset &= (kReferenceLoadMinFarOffset - 1u);
  } else {
    VERIFIED("NormalOffset");
  }

  // std::vector<Value*> args { lbase, IRB->getJInt(offset)};
  // Value* res =  IRB->CreateCall(
  //     FieldLoadWithBakerReadBarrier(
  //       HL, IRB, instruction, needs_null_check, use_load_acquire), args);
  // // return res;

  // MemOperand src(base.X(), offset);
  return GenerateFieldLoadWithBakerReadBarrier(HL, IRB, instruction,
      lbase, IRB->getJInt(offset), needs_null_check, use_load_acquire);
}

Value* FunctionHelper::GenerateFieldLoadWithBakerReadBarrier(
    HGraphToLLVM* HL, IRBuilder* IRB, HInstruction* instruction,
    Value* lbase, Value* loffset,
    bool needs_null_check, bool use_load_acquire) {
  std::vector<Value*> args {lbase, loffset};
  return  IRB->CreateCall(
      FieldLoadWithBakerReadBarrier(
        HL, IRB, instruction, needs_null_check, use_load_acquire), args);
}


ALWAYS_INLINE Value* _ArrayLoadWithBakerReadBarrier(
    HGraphToLLVM* HL, IRBuilder* IRB, HArrayGet* instruction,
    Value* lobj, Value* ldata_offset, Value* lindex) {
  size_t scale_factor = DataType::SizeShift(DataType::Type::kReference);

  CHECK_NO_INTERMEDIATE_ACCESS(instruction);

  // __ Add(temp.X(), obj.X(), Operand(data_offset));
  // __ ldr(ref_reg, MemOperand(temp.X(), index_reg.X(), LSL, scale_factor));
  Value* loffset = HL->GetDynamicOffset(lindex, scale_factor, ldata_offset);
  Value* fastVal = HL->LoadWord<true>(lobj, loffset);

  if (kPoisonHeapReferences) {
    // __ neg(ref_reg, Operand(ref_reg));
    fastVal = Arm64::PoisonHeapReference(IRB, fastVal);
  }
  HL->MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);

  return fastVal;
}

Function* FunctionHelper::ArrayLoadWithBakerReadBarrier(
    HGraphToLLVM* HL, IRBuilder* IRB,
    HArrayGet* instruction, bool needs_null_check) {
  BasicBlock* pinsert_point = IRB->GetInsertBlock();

  std::string name = "ArrayLoadWithBakerReadBarrier";
  if (needs_null_check) name += "_NullCheck";
  if (baker_read_load_.find(name) != baker_read_load_.end()) {
    return baker_read_load_[name];
  }

  std::vector<Type*> argsTy { IRB->getJObjectTy()->getPointerTo(),
    IRB->getJIntTy(), IRB->getJIntTy()};
  FunctionType* ty =
    FunctionType::get(IRB->getVoidPointerType(), argsTy, false);

  Function* f = Function::Create(
      ty, Function::LinkOnceODRLinkage, name, IRB->getModule());
  f->addFnAttr(Attribute::AlwaysInline);
  f->setDSOLocal(true);
  AddAttributesCommon(f);

  Function::arg_iterator arg_iter(f->arg_begin());
  Argument* lobj= &*arg_iter++;
  Argument* ldata_offset = &*arg_iter++;
  Argument* lindex= &*arg_iter++;
  lobj->setName("lobj");
  ldata_offset->setName("data_offset");
  lindex->setName("index");

  BasicBlock* entry_block =
    BasicBlock::Create(IRB->getContext(), "fast_path", f);
  BasicBlock* slow_path =
    BasicBlock::Create(IRB->getContext(), "slow_path", f);
  BasicBlock* resolved =
    BasicBlock::Create(IRB->getContext(), "resolved", f);

  IRB->SetInsertPoint(entry_block);

  Value* fastVal = _ArrayLoadWithBakerReadBarrier(
      HL, IRB, instruction, lobj, ldata_offset, lindex);

  Value* is_null = IRB->CreateCmpIsNull(fastVal);
  MDNode *N=HL->MDB()->createBranchWeights(0, MAX_BRWEIGHT);
  IRB->CreateCondBr(is_null, slow_path, resolved, N);

  IRB->SetInsertPoint(slow_path);
  // CHECK_LLVM("null: calling RT");
  Value* nullref = IRB->getJNull();
  Value* slowVal = HL->GenerateReadBarrierSlow(
      instruction, nullref, lobj, ldata_offset, nullptr);

  IRB->CreateRet(slowVal);
  IRB->SetInsertPoint(resolved);
  VERIFIED_;

  IRB->CreateRet(fastVal);
  baker_read_load_[name] = f;
  IRB->SetInsertPoint(pinsert_point);
  return f;
}

/**
 * @brief Calls ArrayLoadWithBakerReadBarrier
 *
 */
Value* FunctionHelper::GenerateArrayLoadWithBakerReadBarrier(
    HGraphToLLVM* HL,
    IRBuilder* IRB,
    HArrayGet* instruction,
    Value* lobj,
    uint32_t data_offset,
    Value* lindex,
    bool needs_null_check) {

  std::vector<Value*> args {lobj, IRB->getJInt(data_offset), lindex};
  Value* res = IRB->CreateCall(ArrayLoadWithBakerReadBarrier(
    HL,IRB, instruction, needs_null_check), args);

  return res;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
