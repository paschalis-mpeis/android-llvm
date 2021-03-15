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
#include "intrinsic_helper.h"

#include <llvm/IR/Intrinsics.h>
#include "asm_arm_thumb.h"
#include "asm_arm64.h"
#include "function_helper.h"
#include "hgraph_printers.h"
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"
#include "ir_builder.h"
#include "llvm_compilation_unit.h"
#include "mirror/string.h"
#include "optimizing/data_type-inl.h"

#include "llvm_macros_irb_.h"

using namespace ::llvm;
namespace art {
namespace LLVM {


void IntrinsicHelper::LlvmInvariantStart(Value* variable) {
  if(!isa<GlobalVariable>(variable)) {
    DLOG(ERROR) << __func__ << ": Not global. skip for: "
      << variable->getName().str();
    return;
  }
  // TODO: cast
  // LlvmInvariantStart(cast<GlobalVariable*>(variable));
}

void IntrinsicHelper::LlvmInvariantStart(GlobalVariable* variable) {
  Type* varTy = variable->getType();
  DataLayout* DL = new DataLayout(mod_);

  uint32_t typeSize = DL->getTypeAllocSize(varTy); 
  std::vector<Value*> args{irb_->getJLong(typeSize), variable};
  std::vector<Type *> ty{varTy};
  Function *F= Intrinsic::getDeclaration(
      mod_, Intrinsic::invariant_start, ty);

  irb_->CreateCall(F, args);
}


Value* IntrinsicHelper::llvm_round(Intrinsics intrinsic, HInvoke* invoke,
    std::vector<Value*> callee_args) {
  // to follow exactly the Java8 standard, something like the below
  // should be implemented:
  // llvm.round:
  // fcvtas	w0, s8
  // optimizing/intrinsics_arm64.cc TODO
  // ARM64:
  // fcvtas w0, s0
  // tbz w0, #31, #+0x18 (addr 0x23a0dc) (basically ignoring the rest..)
  // frinta s1, s0
  // fsub s1, s0, s1
  // fmov s31, #0x60 (0.5000)
  // fcmp s1, s31
  // cinc w0, w0, eq
  _LOGLLVM2(irb_, ERROR, "Round: TODO_LLVM for Java8 standard");

  CHECK(intrinsic == Intrinsics::kMathRoundFloat ||
        intrinsic == Intrinsics::kMathRoundDouble)
      << "Not Math.round";

  // return type is i32
  DataType::Type htypein = invoke->InputAt(0)->GetType();
  DataType::Type htypeout = invoke->GetType();
  Type* ltypein = irb_->getType(htypein);
  Type* ltypeout = irb_->getType(htypeout);

  std::vector<Type *> ty(1, ltypein); // to get float/double variant
  Function *llvm_round= Intrinsic::getDeclaration(mod_, Intrinsic::round, ty);
  Value* result = irb_->CreateCall(llvm_round, callee_args);
  Value* casted_result = result;

  CHECK(DataType::IsIntegralType(htypeout)) << "result must be integer.";

  // Result must be converted to integral type
  casted_result = irb_->CreateFPToSI(result, ltypeout);
  return casted_result;
}

Value* IntrinsicHelper::callLlvmDoubleIntrinsic(Intrinsic::ID id,
    Intrinsics intrinsic, HInvoke* invoke, std::vector<Value*> args) {
  Type* ltype = irb_->getType(invoke->GetType());
  std::vector<Type *> ty(1, ltype);
  Function *f= Intrinsic::getDeclaration(mod_, id, ty);
  return irb_->CreateCall(f, args);
}

Value* IntrinsicHelper::llvm_sqrt(
    Intrinsics intrinsic, HInvoke* invoke, std::vector<Value*> args) {
  // llvm.round: fsqrt	d0, d8
  // ARM64: fsqrt d0, d0

  return callLlvmDoubleIntrinsic(
      Intrinsic::sqrt, intrinsic, invoke, args);
}

Value* IntrinsicHelper::llvm_ceil(
    Intrinsics intrinsic, HInvoke* invoke, std::vector<Value*> args) {
  // llvm.ceil: frintp	d0, d8
  // ARM64:     frintp d0, d0
return callLlvmDoubleIntrinsic(
    Intrinsic::ceil, intrinsic, invoke, args);
}

Value* IntrinsicHelper::llvm_floor(
    Intrinsics intrinsic, HInvoke* invoke, std::vector<Value*> args) {
  // llvm.floor: frintm	d0, d8
  // ARM64:      frintm d0, d0
  return callLlvmDoubleIntrinsic(
      Intrinsic::floor, intrinsic, invoke, args);
}

Value* IntrinsicHelper::llvm_exp(
    Intrinsics intrinsic, HInvoke* invoke, std::vector<Value*> args) {
  return callLlvmDoubleIntrinsic(
      Intrinsic::exp, intrinsic, invoke, args);
}

Value* IntrinsicHelper::llvm_cos(
    Intrinsics intrinsic, HInvoke* invoke, std::vector<Value*> args) {
  return callLlvmDoubleIntrinsic(
      Intrinsic::cos, intrinsic, invoke, args);
}


// https://godbolt.org/z/7s8hed
Value* IntrinsicHelper::llvm_log(
    Intrinsics intrinsic, HInvoke* invoke, std::vector<Value*> args) {
  return callLlvmDoubleIntrinsic(
      Intrinsic::log, intrinsic, invoke, args);
}

Value* IntrinsicHelper::llvm_fshl(DataType::Type type, std::vector<Value*> args) {
  Type* ltype = irb_->getType(type);
  std::vector<Type *> ty(1, ltype);
  Function *f= Intrinsic::getDeclaration(mod_, Intrinsic::fshl, ty);
  return irb_->CreateCall(f, args);
}

// Count the number of 1 bits Integer.bitcount
Value* IntrinsicHelper::llvm_bitcount(
    HInvoke* h, Intrinsics intrinsic, std::vector<Value*> callee_args) {
  DLOG(INFO) << __func___;
  Type* ltype = nullptr;

  bool cast_to_i32 = false;
  switch(intrinsic) {
    // case Intrinsics::kLongBitCount:
    // ltype = irb_->getJLongTy();
    // cast_to_i32 = true;
    // break;
    case Intrinsics::kIntegerBitCount: 
      ltype = irb_->getJIntTy();
      break;
    default:
      DIE << "Wrong type: must be int or long: " << intrinsic;
  }

  std::vector<Type *> ty(1, ltype); // to get int/long variants
  Function *F = Intrinsic::getDeclaration(mod_, Intrinsic::ctpop, ty);

  std::vector<Value*> args{callee_args.begin(), callee_args.end()};
  Value* result = irb_->CreateCall(F, args);

  if(cast_to_i32) {
    result = irb_->CreateTrunc(result, irb_->getInt32Ty());
  }

  return result;
}



// Count the number of leading/trailing zeros.
Value* IntrinsicHelper::llvm_count_zeros(
    HInvoke* h, Intrinsics intrinsic, std::vector<Value*> callee_args,
    bool leading) {
  DLOG(INFO) << __func___;
  Type* ltype = nullptr;

  bool cast_to_i32 = false;
  switch(intrinsic) {
    case Intrinsics::kLongNumberOfLeadingZeros:
    case Intrinsics::kLongNumberOfTrailingZeros:
      ltype = irb_->getJLongTy();
      cast_to_i32 = true;
      break;
    case Intrinsics::kIntegerNumberOfLeadingZeros: 
    case Intrinsics::kIntegerNumberOfTrailingZeros: 
      ltype = irb_->getJIntTy();
      break;
    default:
      DIE << "Wrong type: must be int or long: " << intrinsic;
  }

  std::vector<Type *> ty(1, ltype); // to get int/long variants
  Function *F = nullptr;
  if(leading) {
    F = Intrinsic::getDeclaration(mod_, Intrinsic::ctlz, ty);
  } else {
    F = Intrinsic::getDeclaration(mod_, Intrinsic::cttz, ty);
  }

  std::vector<Value*> args{callee_args.begin(), callee_args.end()};

  // is_zero_undef: true to allow zero (e.g. cttz(i32 0) is 32
  args.push_back(irb_->getInt1(1));

  Value* result = irb_->CreateCall(F, args);

  if(cast_to_i32) {
    result = irb_->CreateTrunc(result, irb_->getInt32Ty());
  }

  return result;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
