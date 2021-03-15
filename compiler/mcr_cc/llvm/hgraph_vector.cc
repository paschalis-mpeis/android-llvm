/** 
 * Vector operations
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
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"

#include "asm_arm64.h"
#include "asm_arm_thumb.h"
#include "dex/dex_file.h"
#include "dex/invoke_type.h"
#include "dex/method_reference.h"
#include "driver/compiler_options.h"
#include "hgraph_to_llvm-inl.h"
#include "llvm_utils.h"
#include "mirror/array-inl.h"
#include "optimizing/code_generator.h"
#include "optimizing/nodes.h"

#include "llvm_macros_irb_.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

// TODO
// VecSetScalars
// VecMultiplyAccumulate (skipped normal mulacc)
// SADAccumulate
// VecDotProd
// VisitVecReduce
// VisitVecExtractScalar

void HGraphToLLVM::HandleVecUnaryOp(HVecUnaryOperation* h) {
  DataType::Type vecType = h->GetPackedType();
  const bool is_fp = DataType::IsFloatingPointType(vecType);
  VERIFY_LLVMD3("Type: " << vecType << " FP: " << is_fp);

  Value* lhs = getValue(h->InputAt(0));

  bool is_signed = IRBuilder::IsSigned(vecType);

  // ExtractScalar
  // Reduce
  // Cnv
  // Abs
  // Not
  Value* res = nullptr;
  if (h->IsVecNeg()) {
    res = irb_->mCreateNeg(is_fp, is_signed, lhs);
    // CHECK char does not need trunc?
  } else {
    DIE_UNIMPLEMENTED_;
  }

  CHECK(res != nullptr);
  addValue(h, res);
}


void HGraphToLLVM::HandleVecBinaryOp(HVecBinaryOperation* h) {
  DataType::Type vecType = h->GetPackedType();
  const bool is_fp = DataType::IsFloatingPointType(vecType);
  VERIFY_LLVMD3("Type: " << vecType << " FP: " << is_fp);

  Value* lhs = getValue(h->InputAt(0));
  Value* rhs = getValue(h->InputAt(1));

  bool is_signed = IRBuilder::IsSigned(vecType);

  // TODO VecBinops
  // SaturationAdd
  // HalvingAdd 
  // SaturationSub
  // AndNot
  // Shl
  // Shr
  // UShr
  Value* res = nullptr;
  if (h->IsVecAdd()) {
    res = irb_->mCreateAdd(is_fp, lhs, rhs);
  } else if (h->IsVecSub()) {
    res = irb_->mCreateSub(is_fp, lhs, rhs);
  } else if (h->IsVecMul()) {
    res = irb_->mCreateMul(is_fp, lhs, rhs);
  } else if (h->IsVecDiv()) {
    res = irb_->mCreateDiv(is_fp, lhs, rhs);
  } else if (h->IsVecAnd()) {
    res = irb_->CreateAnd(lhs, rhs);
  } else if (h->IsVecOr()) {
    res = irb_->CreateOr(lhs, rhs);
  } else if (h->IsVecNeg()) {
    res = irb_->mCreateNeg(is_fp, is_signed, lhs);
    // CHECK char does not need trunc?
  } else if (h->IsVecXor()) {
    res = irb_->CreateXor(lhs, rhs); 
  // } else if (IsVecShiftOperation(h)) { TODO Shift
  //   res = GetVecShiftOperation(h, lhs, rhs);
  // } else if (h->IsVecMin() || h->IsVecMax()) { TODO
  //   if(is_fp) {
  //     res = ih_->llvm_MaxOrMinf(this, h, lhs, rhs);
  //   } else {
  //     res = ih_->canonical_MaxOrMin(this, h, lhs, rhs);
  //   }
  } else {
    DIE_UNIMPLEMENTED_;
  }

  CHECK(res != nullptr);
  addValue(h, res);
}

/**
 * Examples:
 *    4x32: https://godbolt.org/z/kkUazD
 *    2x32: https://godbolt.org/z/4U_43a
 *    ..
 *
 */
void HGraphToLLVM::VisitVecReplicateScalar(HVecReplicateScalar* h) {
  VERIFY_LLVMD4(GetTwine(h));
  size_t vecLen = h->GetVectorLength();
  DataType::Type vecType = h->GetPackedType();;
  D3LOG(INFO) << __func___ << "Vec: PackedType: " <<  vecLen;
  D3LOG(INFO) << __func___ << "Vec: len: " <<  vecType;

  Type* vecTy = irb_->getTypeExact(vecType);
  HInstruction* hinput = h->InputAt(0);
  DataType::Type inputTy = hinput->GetType();
  Value* input = getValue(hinput); // value to be copied

  // packType size smaller than inputType size: trunc
  // Special case: if param is smaller than i32 (e.g. i8), and it is an
  // HParameterValue, then it is upcast to i32.
  // However, inputTy will have i8 type, and the above check will fail
  Type* inputLlvmType = input->getType();
  bool trunc_param = inputLlvmType != vecTy;

  if(trunc_param ||
      (DataType::Size(vecType) < DataType::Size(inputTy))) {
    if(DataType::IsIntegralType(vecType)) {
      input = irb_->CreateTrunc(input, vecTy);
    } else {
      input = irb_->CreateFPTrunc(input, vecTy);
    }
  }

  // INFO I think optimizing is using index 0 all of the times
  Value* index = irb_->getJInt(0);
  // N x Type
  Value* vec = UndefValue::get(VectorType::get(vecTy, vecLen));
  Value* vecIns = irb_->CreateInsertElement(vec, input, index, "vecIns");

  SmallVector<uint32_t, 16> Elts;  // Elts: element types
  for (unsigned i = 0; i < vecLen; i++) Elts.push_back(0);

  Value* mask = ConstantDataVector::get(*ctx_, Elts);
  Value* res = irb_->CreateShuffleVector(vecIns, vec, mask);

  addValue(h, res);
}

void HGraphToLLVM::VisitVecLoad(HVecLoad* h) {
  // LocationSummary* locations = h->GetLocations();
  size_t size = DataType::Size(h->GetPackedType());
  // VRegister reg = VRegisterFrom(locations->Out());
  // UseScratchRegisterScope temps(GetVIXLAssembler());
  // Register scratch;

  size_t vecLen = h->GetVectorLength();
  DataType::Type vecType = h->GetPackedType();;
  Type* vecTy = irb_->getTypeExact(vecType);
  Type* loadTy = VectorType::get(vecTy, vecLen);

  Value* loadedVec = nullptr;
  switch (h->GetPackedType()) {
    case DataType::Type::kInt16:
      // (short) s.charAt(.) can yield HVecLoad/Int16/StringCharAt.
    case DataType::Type::kUint16:
      // DCHECK_EQ(8u, h->GetVectorLength());
      // Special handling of compressed/uncompressed string load.
      if (mirror::kUseStringCompression && h->IsStringCharAt()) {
        DIE_UNIMPLEMENTED("charAt w/ stringCompression");
        // vixl::aarch64::Label uncompressed_load, done;
        // // Test compression bit.
        // static_assert(static_cast<uint32_t>(mirror::StringCompressionFlag::kCompressed) == 0u,
        //               "Expecting 0=compressed, 1=uncompressed");
        // uint32_t count_offset = mirror::String::CountOffset().Uint32Value();
        // Register length = temps.AcquireW();
        // __ Ldr(length, HeapOperand(InputRegisterAt(h, 0), count_offset));
        // __ Tbnz(length.W(), 0, &uncompressed_load);
        // temps.Release(length);  // no longer needed
        // // Zero extend 8 compressed bytes into 8 chars.
        // __ Ldr(DRegisterFrom(locations->Out()).V8B(),
        //        VecAddress(h, &temps, 1, /*is_string_char_at*/ true, &scratch));
        // __ Uxtl(reg.V8H(), reg.V8B());
        // __ B(&done);
        // if (scratch.IsValid()) {
        //   temps.Release(scratch);  // if used, no longer needed
        // }
        // // Load 8 direct uncompressed chars.
        // __ Bind(&uncompressed_load);
        // __ Ldr(reg, VecAddress(h, &temps, size, /*is_string_char_at*/ true, &scratch));
        // __ Bind(&done);
        // CHECK set loadVec before returning
        // return;
      }
      FALLTHROUGH_INTENDED;
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kInt8:
    case DataType::Type::kInt32:
    case DataType::Type::kFloat32:
    case DataType::Type::kInt64:
    case DataType::Type::kFloat64:
      {
        // DCHECK_LE(2u, h->GetVectorLength());
        // DCHECK_LE(h->GetVectorLength(), 16u);
        Value* vecAddr = VecAddress(h, size, h->IsStringCharAt());

        Value* castedVecAddr =
          irb_->CreateBitCast(vecAddr, loadTy->getPointerTo());

        loadedVec = irb_->CreateLoad(castedVecAddr);
      }
      // __ Ldr(reg, VecAddress(h, &temps, size, h->IsStringCharAt(), &scratch));
      break;
    default:
      LOG(FATAL) << "Unsupported SIMD type: " << h->GetPackedType();
      UNREACHABLE();
  }
  CHECK(loadedVec!=nullptr);

  addValue(h, loadedVec);
}

/**
 * IR Example:
 *    pval = bitcast i32* %val to <4 x i32>*
 *    store <4 x i32> %array, <4 x i32>* %pval
 *    ret <4 x i32> %array
 *
 */
void HGraphToLLVM::VisitVecStore(HVecStore* h) {
  VERIFY_LLVMD4(GetTwine(h));
  size_t size = DataType::Size(h->GetPackedType());
  size_t vecLen = h->GetVectorLength();
  DataType::Type vecType = h->GetPackedType();;
  D3LOG(INFO) << __func___ << "Vec: PackedType: " <<  vecType;
  D3LOG(INFO) << __func___ << "Vec: len: " <<  vecLen;

  Type* vecTy = irb_->getTypeExact(vecType);
  Value* value = getValue(h->InputAt(2));
  Type* storeTy = VectorType::get(vecTy, vecLen); 
  Value* vecAddr = VecAddress(h, size, false  /*string_char_at*/);
  Value* castedVecAddr = irb_->CreateBitCast(vecAddr, storeTy->getPointerTo());
  irb_->CreateStore(value, castedVecAddr);
}

/**
 * Example (first line only):
 *  1.    add w16, w0, w4, lsl #3
 *  2.    str q1, [x16, #16]
 *
 * INFO the instruction might not end up with a single add+lsl
 * instruction because in some cases the w4 value comes from a phi
 * therefore its separate.
 *
 * Optimizing might load it in a separate instruction and then do
 * add + lsl
 *
 * In both cases it's 2 instructions
 */
Value* HGraphToLLVM::VecAddress(HVecMemoryOperation* h,
    size_t size, bool is_string_char_at) {
  CHECK_LLVMD3("XVEC: bug on 2nd load");  // CHECK
  D3LOG(INFO) << __func__;
  HInstruction* base = h->InputAt(0);
  HInstruction* index = h->InputAt(1);
  Value* lbase = getValue(base);
  Value* lindex = getValue(index);

  uint32_t offset = is_string_char_at
      ? mirror::String::ValueOffset().Uint32Value()
      : mirror::Array::DataOffset(size).Uint32Value();
  size_t shift = ComponentSizeShiftWidth(size);

  Value* vecAddr=nullptr;
  if (index->IsConstant()) {
    VERIFY_LLVM("Constant");
    // Int64FromLocation
    offset += index->AsConstant()->GetValueAsUint64() << shift;
    vecAddr=lbase;
    // return HeapOperand(base, offset);
  } else {
    // __ Add(*scratch, base, Operand(WRegisterFrom(index), LSL, shift));
    Value* shl = irb_->CreateShl(
        lindex, irb_->getJUnsignedInt(shift), "vecShl");
    Value *temp = irb_->CreatePtrToInt(lbase, irb_->getInt32Ty(), "vecPtI");
    temp = irb_->CreateAdd(temp, shl, "vecAdd");
    vecAddr= irb_->CreateIntToPtr(
        temp, irb_->getJByteTy()->getPointerTo(), "vecItP");
  }
  
  // add offset
  std::vector<Value*> args{irb_->getJUnsignedInt(offset)};
  Value* gep = irb_->CreateInBoundsGEP(vecAddr, args, "vecGEP");

  return gep;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
