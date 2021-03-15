/**
 * Load operations that are mostly used for runtime interractions.
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

#include "hgraph_load_ops-inl.h"
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"

#include "llvm_macros_irb_.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

Value* HGraphToLLVM::GenUnsafeGet(HInvoke* invoke,
                         DataType::Type type,
                         bool is_volatile) {
  // LocationSummary* locations = invoke->GetLocations();
  // Location base_loc = locations->InAt(1);
  // Register base = WRegisterFrom(base_loc);      // Object pointer.
  Value* lbase = getValue(invoke->InputAt(1));
  // Location offset_loc = locations->InAt(2);
  // Register offset = XRegisterFrom(offset_loc);  // Long offset.
  Value* loffset = getValue(invoke->InputAt(2));
  // Location trg_loc = locations->Out();
  // Register trg = RegisterFrom(trg_loc, type);

  const bool isReference = (type == DataType::Type::kReference);
  const bool i64=(type == DataType::Type::kInt64);
  const bool i32=(type == DataType::Type::kInt32);
  Value* loaded=nullptr;
  if (isReference && kEmitCompilerReadBarrier && kUseBakerReadBarrier) {
    // UnsafeGetObject/UnsafeGetObjectVolatile with Baker's read barrier case.
    // Register temp = WRegisterFrom(locations->GetTemp(0));
    // MacroAssembler* masm = codegen->GetVIXLAssembler();
    // Piggy-back on the field load path using introspection for the Baker read barrier.
    // __ Add(temp, base, offset.W());  // Offset should not exceed 32 bits.
    // codegen->GenerateFieldLoadWithBakerReadBarrier(
    //     invoke,
    //     trg_loc,
    //     base,
    //     MemOperand(temp.X()),
    //     /* needs_null_check= */ false,
    //     is_volatile);
    loaded = fh_->GenerateFieldLoadWithBakerReadBarrier(
        this, irb_, invoke, lbase, loffset, false, is_volatile);
  } else {
    // Other cases.
    // MemOperand mem_op(base.X(), offset);
    if (is_volatile) {
      VERIFY_LLVM("LoadAcquire");
      loaded=Arm64::LoadAcquire(irb_, invoke, type, lbase, loffset, false);
    } else {
      if(i64) {
        loaded=Load<false, false>(lbase, loffset);
      } else if (i32) {
        loaded=Load<false, false>(lbase, loffset);
      } else {
        loaded=Load<true, false>(lbase, loffset);
      }
    }

  if (type == DataType::Type::kReference) {
    // codegen->MaybeGenerateReadBarrierSlow(invoke, trg_loc, trg_loc, base_loc, 0u, offset_loc);
    VERIFY_LLVM("Reference w/o Barrier: maybe 'loaded' as a base?");
    Value* nullref = irb_->getJNull();
    loaded= MaybeGenerateReadBarrierSlow(
        invoke, nullref, lbase, loffset, irb_->getJInt(0), is_volatile);
  }
}
  return loaded;
}

/**
 * @brief Traditional field load
 *
 * @return 
 */
Value* HGraphToLLVM::_LoadForFieldGet(
    HInstruction *h, Value* lobj, Value* offset, bool is_volatile) {
  DataType::Type field_type = _GetFieldType(h);
  // ptr to load
  std::vector<Value*> idx_list;
  idx_list.push_back(offset);
  Value* gep = irb_->CreateInBoundsGEP(lobj, idx_list);
  // size to load
  Value* cast = irb_->CreateBitCast(
      gep, irb_->getTypeExact(field_type)->getPointerTo());

  Value* load = irb_->CreateLoad(cast, is_volatile);
  Type* storeTy = irb_->getType(h);

  Value* casted_val = CastForStorage(load, field_type, storeTy);
  casted_val->setName("fieldGet");

  return casted_val;
}

Value* HGraphToLLVM::GenerateReadBarrierSlow(
    HInstruction* h, Value* lref, Value* lobj,
    Value* loffset, Value* lindex) {
  VERIFIED_;

  if (lindex != nullptr) {
    // Handle `index_` for HArrayGet and
    // UnsafeGetObject/UnsafeGetObjectVolatile intrinsics.
    if (h->IsArrayGet()) {
      // Compute the actual memory offset and store it in `index`.
      // LLVM should be ok for: IsCoreCalleeSaveRegister
      
      //// Shifting the index value contained in `index_reg` by the scale
      //// factor (2) cannot overflow in practice, as the runtime is
      //// unable to allocate object arrays with a size larger than
      //// 2^26 - 1 (that is, 2^28 - 4 bytes).
      
      //__ Lsl(index_reg, index_reg, DataType::SizeShift(type));
      //__ Add(index_reg, index_reg, Operand(offset_));
      DataType::Type array_type = h->GetType();

      HInstruction* index = GetArrayIndex(h->AsArrayGet());
      const bool is_constant = index->IsConstant();
      size_t lsl_shift = 0;
      if (is_constant) {
        //__ Lsl(index_reg, index_reg, DataType::SizeShift(type));
        //__ Add(index_reg, index_reg, Operand(offset_));
        size_t array_shift = DataType::SizeShift(array_type);
        lsl_shift = array_shift;
      } else {
        // __ ldr(ref_reg, MemOperand(temp.X(), index_reg.X(), LSL, scale_factor));
        size_t scale_factor = DataType::SizeShift(DataType::Type::kReference);
        lsl_shift = scale_factor;
      }
      loffset = GetDynamicOffset(lindex, lsl_shift, loffset);
    } else {
      // In the case of the UnsafeGetObject/UnsafeGetObjectVolatile
      // intrinsics, `index_` is not shifted by a scale factor of 2
      // (as in the case of ArrayGet), as it is actually an offset
      // to an object field within an object.
    }
  }

  //   __ Ldr(out, HeapOperand(out, class_offset);
  //   GenerateReadBarrierSlow(instruction, out_loc, out_loc, out_loc, offset);
  Value* val = ArtCallReadBarrierSlow(lref, lobj, loffset);

  ANDROID_LOG_HEX(WARNING, this, h, val);
  return val;
}

/** VERIFIED BakerRead
 * This was the TwoRegisters variation, but we don't need to
 * specialize in LLVM. The two versions would have been:
 * A. out = *(obj + offset)
 * B. out = *(out + offset) ?
 *
 * The output will result in a virtual LLVM IR register anyway.
 * Assembler will optimize whatever is needed.
 *
 * @param instruction
 * @param lobj
 * @param loffset
 * @param read_barrier_option
 *
 * @return 
 */
// GenerateReferenceLoadOneRegister
// GenerateReferenceLoadTwoRegisters
Value* HGraphToLLVM::GenerateReferenceLoad(
    IRBuilder* irb, HInstruction* instruction,
    Value* lobj, uint32_t offset,
    ReadBarrierOption read_barrier_option) {
  Value* loaded_ref = nullptr;
  if (read_barrier_option == kWithReadBarrier) {
    if (kUseBakerReadBarrier) {
      // INFO this needs link-time support so we go slow path
      VERIFIED("FastBakerRead");
      // Load with fast path based Baker's read barrier.
      // /* HeapReference<Object> */ out = *(obj + offset)
      // codegen_->GenerateFieldLoadWithBakerReadBarrier
      loaded_ref = fh_->GenerateFieldLoadWithBakerReadBarrier(
          this, irb_, instruction, lobj, offset,
          false, false);
      loaded_ref->setName("loadedRefBakerRead");
    } else {
      CHECK_LLVM("ART does also : LDR + obj_reg+offset");
      // then it GenerateReadBarrierSlow on that result
      // Load with slow path based read barrier.
      // /* HeapReference<Object> */ out = *(obj + offset)
      // __ Ldr(out_reg, HeapOperand(obj_reg, offset));
      // GenerateReadBarrierSlow(irb, instruction, out, out, obj, offset);
      Value* loffset = irb_->getInt32(offset);
      Value* nullref = irb_->getJNull();
      loaded_ref = GenerateReadBarrierSlow(instruction, nullref,
          lobj, loffset, nullptr /*index*/);
      loaded_ref->setName("loadedRefBakerReadSlow");
      LOGLLVM2val(INFO, "BakerRead", loaded_ref);
    }
  } else {
    // Plain load with no read barrier.
    VERIFY_LLVMD4("PlainLoad (no read barrier)");
    loaded_ref = LoadWord<true>(lobj, offset);
    loaded_ref->setName("loadRef");
    loaded_ref = Arm64::MaybeUnpoisonHeapReference(irb_, loaded_ref);
    LOGLLVM4val(INFO, "PlainLoad: ", loaded_ref);
  }
  return loaded_ref;
}

}  // namespace LLVM
}  // namespace art

#include "llvm_macros_undef.h"
