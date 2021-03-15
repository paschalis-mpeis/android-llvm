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
#include "stack.h"
#include <sstream>

#include "llvm_macros_IRBc.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

Function* FunctionHelper::ArraySetWriteBarrier(
    HGraphToLLVM* HL, IRBuilder* IRB, HArraySet* h, uint32_t offset) {
  HInstruction* index = HL->GetArrayIndex(h);
  const bool is_constant = index->IsConstant();
  const DataType::Type array_type = h->GetType();
  DataType::Type value_type = h->GetComponentType();
  bool may_need_runtime_call_for_type_check = h->NeedsTypeCheck();

  // INFO offset is an important part of the name, as in the case of
  // static invocation  part of the name, a compile time constant will
  // be used.
  // This will create 'duplicates' of this method with only difference
  // the offsets. So the code might increase, but since the method is
  // inlinable
  std::string name = "ArraySet";
  { std::stringstream ss; ss << value_type << offset; name+=ss.str();}
  name+="WriteBarrier";
  if(!is_constant) { 
    name+="Dynamic";
  } else {
    std::stringstream ss;
    ss << "StaticIndex" << Int64FromConstant(index->AsConstant());
    name+=ss.str();
  }
  if(may_need_runtime_call_for_type_check) name+="MayNeedRtTypeCheck";
  if (array_get_[name] != nullptr) return array_get_[name];

  BasicBlock* pinsert_point = IRB->GetInsertBlock();
  VERIFY_LLVMD4(name);

  std::vector<Type*> argsTy {IRB->getVoidPointerType(),
    IRB->getType(value_type)};
  argsTy.push_back(IRB->getJIntTy());

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
  Value* storeValue= &*arg_iter++;
  storeValue->setName("storeValue");
  Value* argIndex = nullptr;
  // sending argIndex in anyCase, because ArtCallAputObject needs it
  // Probably gettign a constant will never resort to RT, but we send it anyway.
  argIndex = &*arg_iter++;
  argIndex->setName("index");

  LLVMContext& ctx = IRB->getContext();
  BasicBlock* bbEntry = BasicBlock::Create(ctx, "entry", func);

  IRB->SetInsertPoint(bbEntry);

  // We use a block to end the scratch scope before the write barrier, thus
  // freeing the temporary registers so they can be used in `MarkGCCard`.
  Value* loffset = nullptr;
  if (index->IsConstant()) {
    offset += HL->Int64FromLocation(index) << DataType::SizeShift(value_type);
    loffset = IRB->getJInt(offset);
    loffset->setName("offsetStatic");
  } else {
    // destination = HeapOperand(temp,
    //     XRegisterFrom(index), LSL, DataType::SizeShift(value_type));
    loffset = HL->GetDynamicOffset(
        argIndex, DataType::SizeShift(value_type), offset);
    loffset->setName("offsetDynamic");
  }

  uint32_t class_offset = mirror::Object::ClassOffset().Int32Value();
  uint32_t super_offset = mirror::Class::SuperClassOffset().Int32Value();
  uint32_t component_offset = mirror::Class::ComponentTypeOffset().Int32Value();

  BasicBlock* bbNonZero = nullptr;
  BasicBlock* bbSlowPath =  nullptr;
  if (may_need_runtime_call_for_type_check) {
    bbNonZero = BasicBlock::Create(ctx, "non_zero", func);
    bbSlowPath= BasicBlock::Create(ctx, "slow_path", func);
    
    if (h->GetValueCanBeNull()) {
      // __ Cbnz(Register(value), &non_zero);
      Value*  nullVal = IRB->getJNull();
      Value* cmp = IRB->mCreateCmpNE(false, storeValue, nullVal);

      BasicBlock* bbZero = BasicBlock::Create(ctx, "zero", func);
      IRB->CreateCondBr(cmp, bbNonZero, bbZero);

      IRB->SetInsertPoint(bbZero);

      // if (!index->IsConstant()) { __ Add(temp, array, offset); }
      std::vector<Value*> __ofst{loffset};
      Value* gep = IRB->CreateInBoundsGEP(argArrayObj, __ofst);

      // __ Str(wzr, destination);
      VERIFY_LLVMD4("Storing Zero");
      CHECK(value_type == DataType::Type::kReference);
      Value* zeroVal = IRB->getJInt(0);
      Type* setTy = IRB->getJIntTy();

      Value* casted_value = HL->CastForStorage(zeroVal, value_type, setTy);
      Value* casted_gep = IRB->CreateBitCast(gep, setTy->getPointerTo());
      VERIFY_LLVMD4("CreatStore: Type: " << value_type);
      IRB->mCreateStore(casted_value, casted_gep, false);

      Arm64::MaybeRecordImplicitNullCheck(h);
      IRB->CreateRetVoid();
    } else {
      // value cannot be zero: e.g. when setting a newly created object:
      // JAVA: a[i] = new Object();
      IRB->CreateBr(bbNonZero);
    }

    IRB->SetInsertPoint(bbNonZero);
    // Note that when Baker read barriers are enabled, the type
    // checks are performed without read barriers.  This is fine,
    // even in the case where a class object is in the from-space
    // after the flip, as a comparison involving such a type would
    // not produce a false positive; it may of course produce a
    // false negative, in which case we would take the ArraySet
    // slow path.

    // /* HeapReference<Class> */ temp = array->klass_
    // __ Ldr(temp, HeapOperand(array, class_offset));
    Value* temp = HL->LoadWord<true>(argArrayObj, class_offset);
    temp->setName("arrayClass");
    Arm64::MaybeRecordImplicitNullCheck(h);
    temp = Arm64::MaybeUnpoisonHeapReference(IRB, temp);

    // /* HeapReference<Class> */ temp = temp->component_type_
    // __ Ldr(temp, HeapOperand(temp, component_offset));
    temp = HL->LoadWord<true>(temp, component_offset);
    temp->setName("classComponentOffset");

    // /* HeapReference<Class> */ temp2 = value->klass_
    // __ Ldr(temp2, HeapOperand(Register(value), class_offset));
    Value* temp2 = HL->LoadWord<true>(storeValue, class_offset);
    temp2->setName("valueClass");

    // If heap poisoning is enabled, no need to unpoison `temp`
    // nor `temp2`, as we are comparing two poisoned references.
    // __ Cmp(temp, temp2);
    // temps.Release(temp2);

    BasicBlock* bbDoPut = BasicBlock::Create(ctx, "do_put_fast", func);

    if (h->StaticTypeOfArrayIsObjectArray()) {
      Value* cmpTemps = IRB->mCreateCmpEQ(false, temp, temp2);
      
      BasicBlock* bbDoPutOther = BasicBlock::Create(ctx, "do_put_other", func);
      IRB->CreateCondBr(cmpTemps, bbDoPut, bbDoPutOther);
      IRB->SetInsertPoint(bbDoPutOther);
      // If heap poisoning is enabled, the `temp` reference has
      // not been unpoisoned yet; unpoison it now.
      temp = Arm64::MaybeUnpoisonHeapReference(IRB, temp);

      // /* HeapReference<Class> */ temp = temp->super_class_
      // __ Ldr(temp, HeapOperand(temp, super_offset));
      temp = HL->LoadWord<true>(temp, super_offset);

      // If heap poisoning is enabled, no need to unpoison
      // `temp`, as we are comparing against null below.
      // __ Cbnz(temp, slow_path->GetEntryLabel());
      Value* cmpTempNz = IRB->mCreateCmpNE(false, temp, IRB->getJNull());
      MDNode *N=HL->MDB()->createBranchWeights(0, MAX_BRWEIGHT);
      IRB->CreateCondBr(cmpTempNz, bbSlowPath, bbDoPut, N);

      IRB->SetInsertPoint(bbDoPut);
    } else {
      // __ B(ne, slow_path->GetEntryLabel());
      Value* cmpTemps = IRB->mCreateCmpNE(false, temp, temp2);
      MDNode *N=HL->MDB()->createBranchWeights(0, MAX_BRWEIGHT);
      IRB->CreateCondBr(cmpTemps, bbSlowPath, bbDoPut, N);
      IRB->SetInsertPoint(bbDoPut);
    }
  }

  VERIFY_LLVMD4("doPutFast");
  Value* source = storeValue;
  if (kPoisonHeapReferences) {
    VERIFY_LLVM("PoisonHeapReference");
    source = Arm64::PoisonHeapReference(IRB, source);
  }

  // if (!index->IsConstant()) { __ Add(temp, array, offset); }
  Value* casted_value = nullptr;
  std::vector<Value*> __ofst{loffset};
  Value* gep = IRB->CreateInBoundsGEP(argArrayObj, __ofst);
  Type* setTy = nullptr;
  if (value_type == DataType::Type::kReference) {
    casted_value = HL->GetHandleFromPointer(storeValue);
    casted_value->setName("arraySetval");
    // using JInt to force the use of w registers
    // str wzr, [x16, x3, lsl #2]
    setTy = IRB->getJIntTy();
  } else {
    // this might never be used, as this method is used only for objects
    VERIFY_LLVM("Type: " << value_type);
    setTy = IRB->getTypeExact(value_type);
    casted_value = storeValue;
  }

  casted_value = HL->CastForStorage(casted_value, value_type, setTy);
  Value* casted_gep = IRB->CreateBitCast(gep, setTy->getPointerTo());
  VERIFY_LLVMD4("CreatStore: Type: " << value_type);
  IRB->mCreateStore(casted_value, casted_gep, false);

  if (!may_need_runtime_call_for_type_check) {
    Arm64::MaybeRecordImplicitNullCheck(h);
  }

  HL->CallMarkGCCard(
      argArrayObj, storeValue, value_type, h->GetValueCanBeNull());
  IRB->CreateRetVoid();

  if (may_need_runtime_call_for_type_check) {
    CHECK(bbSlowPath !=nullptr);
    IRB->SetInsertPoint(bbSlowPath);
    HL->ArtCallAputObject(argArrayObj, argIndex, storeValue);
    IRB->CreateRetVoid();
  }

  IRB->SetInsertPoint(pinsert_point);
  return func;
}

#include "llvm_macros_undef.h"
}  // namespace LLVM
}  // namespace art
