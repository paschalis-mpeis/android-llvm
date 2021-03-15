/**
 * Supporting more intrinsics, and implementing more by converting JNI
 * calls to specific Math lib operations can significantly speedup LLVM code.
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

ExtraIntrinsics IntrinsicHelper::GetExtraIntrinsic(std::string pretty_method) {
  if (pretty_method.compare("double java.lang.Math.sin(double)") == 0) {
    return ExtraIntrinsics::kMathSin;
  } else if (pretty_method.compare("double java.lang.Math.pow(double, double)") == 0) {
    return ExtraIntrinsics::kMathPow;
  } else if (pretty_method.compare("java.lang.Class java.lang.Object.getClass()") == 0) {
    return ExtraIntrinsics::kObjectGetClass;
  }
  return ExtraIntrinsics::kNone;
}

void IntrinsicHelper::HandleExtraIntrinsic(
    HGraphToLLVM* HL, HInvoke* invoke,
    std::vector<Value*> callee_args, std::string callee_name) {
  ExtraIntrinsics intrinsic = GetExtraIntrinsic(callee_name);
  D3LOG(INFO) << __func___ << intrinsic << " HF: " << callee_name;

  Function* f = nullptr;
  Value* result = nullptr;
  switch (intrinsic) {
    case ExtraIntrinsics::kMathSin:
      {
        result = CallMathSin(intrinsic, invoke, callee_args);
      } break;
    case ExtraIntrinsics::kMathPow:
      {
        result = CallMathPow(intrinsic, invoke, callee_args);
      } break;
    case ExtraIntrinsics::kObjectGetClass:
      {
        DLOG(ERROR) << "XGH: GetObjClass: args: " << callee_args.size();
        result = HL->GetObjectClassDirect(callee_args.at(0));
      } break;

    default:
      DIE_TODO << intrinsic
        << " (" << callee_name << ")\nCaller: "
        << HL->GetPrettyMethod();
      UNREACHABLE();
  }

  if (!result) {  // we haven't done the call yet
    result = irb_->CreateCall(f, callee_args);
  }

  if (invoke->GetType() != DataType::Type::kVoid) {
    HL->addValue(invoke, result);
  }
}
bool IntrinsicHelper::UnimplementedIntrinsic(Intrinsics intrinsic) {
  switch (intrinsic) {
    // UnimplementedIntrinsic in arm64 backend
    case Intrinsics::kStringIndexOf:
    case Intrinsics::kStringStringIndexOf:
    case Intrinsics::kStringStringIndexOfAfter:
    case Intrinsics::kStringBuilderLength:
    case Intrinsics::kUnsafeGetAndAddInt:
    case Intrinsics::kStringBufferLength:
      return true;
    // OPTIMIZE_LLVM Implement these:
    case Intrinsics::kUnsafeCASLong: // Blowfish
    case Intrinsics::kUnsafeCASObject: // Blowfish
    case Intrinsics::kUnsafeCASInt: // FNV
    case Intrinsics::kSystemArrayCopyChar:
    case Intrinsics::kStringIndexOfAfter: // Droidfish
    case Intrinsics::kStringNewStringFromChars: // Droidfish
    case Intrinsics::kIntegerValueOf: // Droidfish
    case Intrinsics::kStringGetCharsNoCheck: // Droidfish
    case Intrinsics::kStringEquals: // Droidfish
      {
        // OPTIMIZE_LLVM once done, remove from IsSimplified
        std::stringstream ss;
        ss << "OPTIMIZE: Intrinsic: " << intrinsic;
        LLVM::LlvmCompiler::LogWarning(ss.str());
      }
      return true;
    default:
      return false;
  }
}

bool IntrinsicHelper::MustHandle(Intrinsics intrinsic) {
  if (intrinsic == Intrinsics::kNone ||
      UnimplementedIntrinsic(intrinsic)) {
    return false;
  }
  return true;
}

bool IntrinsicHelper::ExcludeFromHistogram(Intrinsics intrinsic) {
  // CHECK_LLVM might have to use the same list as IsSimplified
  switch(intrinsic) {
    case Intrinsics::kStringBufferToString:
    case Intrinsics::kStringBuilderToString:
    case Intrinsics::kStringBuilderAppend:
    case Intrinsics::kStringBufferAppend:
      return true;
    default:
      return false;
  }
}

bool IntrinsicHelper::IsSimplified(Intrinsics intrinsic) {
  switch(intrinsic) {
    case Intrinsics::kStringBufferToString:
    case Intrinsics::kStringBuilderToString:
    case Intrinsics::kStringBuilderAppend:
    case Intrinsics::kStringBufferAppend:
    case Intrinsics::kSystemArrayCopy:
      return true;
    default:
      return false;
  }
}

void IntrinsicHelper::HandleIntrinsic(
    HGraphToLLVM* HL, HInvoke* invoke,
    std::vector<Value*> callee_args, std::string callee_name) {
  Intrinsics intrinsic = invoke->GetIntrinsic();
  D2LOG(INFO) << "HandleIntrinsic: " << intrinsic << " HF: " << callee_name;

  Function* f = nullptr;
  Value* result = nullptr;
  switch (intrinsic) {
    case Intrinsics::kUnsafeGetObjectVolatile:
      { result=HL->GenUnsafeGet(
          invoke, DataType::Type::kReference, /*volatile*/ true);
      } break;
    case Intrinsics::kUnsafeGet:
      { result=HL->GenUnsafeGet(
          invoke, DataType::Type::kInt32, /*volatile*/ false);
      } break;
    case Intrinsics::kUnsafeGetVolatile:
      { result=HL->GenUnsafeGet(
          invoke, DataType::Type::kInt32, /*volatile*/ true);
      } break;
    case Intrinsics::kFloatIsInfinite:
      {
        CHECK(callee_args.size() == 1) << intrinsic << ": args must be 1.";
        result = HL->GenIsInfinite(callee_args.at(0), false);
      } break;
    case Intrinsics::kDoubleIsInfinite:
      {
        CHECK(callee_args.size() == 1) << intrinsic << ": args must be 1.";
        result = HL->GenIsInfinite(callee_args.at(0), true);
      } break;
    case Intrinsics::kUnsafeGetLongVolatile:
      { result=HL->GenUnsafeGet(
          invoke, DataType::Type::kInt64, /*volatile*/ true);
      } break;
    case Intrinsics::kThreadCurrentThread:
      {
        result = LoadThreadCurrentThread(HL);
      } break;
    case Intrinsics::kStringCharAt:
      {
#ifdef ART_MCR_ANDROID_10
        // SimplifyStringCharAt converts this intrinsic to ArrayGet
        DIE << intrinsic << " is no an ArrayGetMaybeCompressedChar";
#elif defined(ART_MCR_ANDROID_6)
        f = CharAt(invoke, HL);
#endif
      } break;
    // They do nothing for this case
    case Intrinsics::kReachabilityFence: { return; }
    case Intrinsics::kMathFloor:
      { result = llvm_floor(intrinsic, invoke, callee_args); } break;
    case Intrinsics::kMathExp:
      { result = llvm_exp(intrinsic, invoke, callee_args); } break;
    case Intrinsics::kMathLog:
      { result = llvm_log(intrinsic, invoke, callee_args); } break;
    case Intrinsics::kMathCos:
      { result = llvm_cos(intrinsic, invoke, callee_args); } break;
    case Intrinsics::kMathTan:
      { result = irb_->CreateCall(MathTan(), callee_args); } break;
    case Intrinsics::kMathAtan2:
      { result = irb_->CreateCall(MathATan2(), callee_args); } break;
    case Intrinsics::kMathAsin:
      { result = CallCMathAsin(callee_args); } break;
    case Intrinsics::kMathCeil:
      { result = llvm_ceil(intrinsic, invoke, callee_args); } break;
    case Intrinsics::kMathRoundDouble:
      FALLTHROUGH_INTENDED;
    case Intrinsics::kMathRoundFloat:
      { result = llvm_round(intrinsic, invoke, callee_args); } break;
    case Intrinsics::kMathSqrt:
      { result = llvm_sqrt(intrinsic, invoke, callee_args); } break;
    case Intrinsics::kIntegerBitCount:
      { result = llvm_bitcount(invoke, intrinsic, callee_args); } break;
    case Intrinsics::kLongNumberOfLeadingZeros:
    case Intrinsics::kIntegerNumberOfLeadingZeros: {
      result = llvm_count_zeros(invoke, intrinsic, callee_args, true);
    } break;
    case Intrinsics::kLongNumberOfTrailingZeros:
    case Intrinsics::kIntegerNumberOfTrailingZeros: {
      result = llvm_count_zeros(invoke, intrinsic, callee_args, false);
    } break;
    case Intrinsics::kStringCompareTo: {
      f = fh_->__StringCompareTo();
#ifdef CODE_UNUSED
      // INFO enable this only if I manage to inline the call in plugin code
      // pop arguments
      Value* str1 = callee_args.at(0);
      Value* str2 = callee_args.at(1);
      str1->setName("str1");
      str2->setName("str2");
      callee_args.clear();

      // cast to mirror::String* and push back
      Type* ty = irb_->getMirrorStringPointerTy();
      str1 = irb_->CreateBitCast(str1, ty);
      str2 = irb_->CreateBitCast(str2,ty);
      callee_args.push_back(str1);
      callee_args.push_back(str2);
      f = fh_->__mirrorStringCompareTo();
      f = fh_->__StringCompareTo();
#endif
    } break;
    case Intrinsics::kMathAbsInt:
    case Intrinsics::kMathAbsFloat:
    case Intrinsics::kMathAbsDouble:
    case Intrinsics::kMathAbsLong: {
      DIE_ANDROID10() << "Intrinsic now HNode: " << intrinsic;
      result = CallMathAbs(invoke, callee_args);
    } break;
    case Intrinsics::kMathMaxIntInt:
    case Intrinsics::kMathMaxLongLong:
    case Intrinsics::kMathMaxDoubleDouble:
    case Intrinsics::kMathMaxFloatFloat:
    case Intrinsics::kMathMinIntInt:
    case Intrinsics::kMathMinLongLong:
    case Intrinsics::kMathMinDoubleDouble:
    case Intrinsics::kMathMinFloatFloat:
    {
      DIE_ANDROID10() << "Intrinsic now HNode: " << intrinsic;
      f = MinMax(intrinsic, invoke->GetType());
    } break;
    case Intrinsics::kFloatIntBitsToFloat:
    {
      result = CallIntBitsToFloat(intrinsic, callee_args);
    } break;
    case Intrinsics::kDoubleLongBitsToDouble:
    {
      result = CallLongBitsToDouble(intrinsic, callee_args);
    } break;
    case Intrinsics::kDoubleDoubleToRawLongBits: {
      result = CallDoubleDoubleToRawLongBits(intrinsic, callee_args);
    } break;
    case Intrinsics::kFloatFloatToRawIntBits: {
      result = CallFloatFloatToRawIntBits(intrinsic, callee_args);
    } break;
    case Intrinsics::kSystemArrayCopyChar: {
      //  INFO this is an UnimplementedIntrinsic (goes through RT)
      DIE_TODO << "Unimplemented: kSystemArrayCopyChar:" << callee_name;
      result = CallSystemArrayCopyChar(HL, intrinsic, invoke, callee_args);
    } break; 
    default:
      DLOG(FATAL) << __func__ << ": TODO: " << intrinsic
                 << " (" << callee_name << "/graphmethod:" << HL->GetPrettyMethod() << ")";
      UNREACHABLE();
  }

  if (!result) {  // we haven't done the call yet
    result = irb_->CreateCall(f, callee_args);
  }

  if (invoke->GetType() != DataType::Type::kVoid) {
    HL->addValue(invoke, result);
  }
}


/**
 *
 * @return if false, means wasn't handled so it
 * must continue with HL->HandleInvoke
 */
bool IntrinsicHelper::HandleSimplified(
    HGraphToLLVM* HL, HInvoke* invoke,
    std::vector<Value*> callee_args, std::string callee_name) {
  Intrinsics intrinsic = invoke->GetIntrinsic();
  D2LOG(INFO) << "HandleIntrinsic: " << intrinsic << " HF: " << callee_name;

  switch (intrinsic)
  {
    case Intrinsics::kStringBufferToString:
    case Intrinsics::kStringBuilderToString:
      {
        return HL->SimplifyAllocationIntrinsic(invoke);
      }
    case Intrinsics::kStringBuilderAppend:
    case Intrinsics::kStringBufferAppend:
      {
        // simplification does not remove the call
        HL->SimplifyReturnThis(invoke);
        return false;
      } 
    case Intrinsics::kSystemArrayCopy:
      { 
        // TODO_LLVM
        // std::stringstream ss;
        // ss << " (not all invocations are optimized)";
        // LLVM::LlvmCompiler::LogWarning(ss.str());
        return false;
      }
    default:
      DLOG(FATAL) << __func__ << ": TODO: " << intrinsic
        << " (" << callee_name << "/graphmethod:" << HL->GetPrettyMethod() << ")";
      UNREACHABLE();
  }
}

Value* IntrinsicHelper::LoadThreadCurrentThread(HGraphToLLVM* HL) {
  D3LOG(INFO) << __func__;
  Value* thread = HL->GetLoadedThread();

  uint32_t offset = irb_->IsCompilingArm64()?
    Thread::PeerOffset<kArm64PointerSize>().Int32Value():
    Thread::PeerOffset<kArmPointerSize>().Int32Value();

  // Emited asm (arm64)
  // INFO LLVM looks ok (can resolve method when using this)
  // Also LLVMtoJNI uses it, so I can verify it there as well..
  //
  // ART:
  // ldr w1, [tr, #232] ; peer
  // LLVM:
  // ldr	x21, [x9,#232]
  Value* current_thread = HL->LoadWord<true>(thread, offset);
  VERIFY_LLVMD2("Converted LoadFromAddress to LoadWord");
  return current_thread;
}

Value* IntrinsicHelper::CallIntBitsToFloat(
    Intrinsics intrinsic, std::vector<Value*> args) {
  CHECK(intrinsic == Intrinsics::kFloatIntBitsToFloat);
  return irb_->CreateBitCast(args.at(0), irb_->getJFloatTy());
}

Value* IntrinsicHelper::CallLongBitsToDouble(
    Intrinsics intrinsic, std::vector<Value*> args) {
  CHECK(intrinsic == Intrinsics::kDoubleLongBitsToDouble);
  return irb_->CreateBitCast(args.at(0), irb_->getJDoubleTy());
}

Value* IntrinsicHelper::CallDoubleDoubleToRawLongBits(
    Intrinsics intrinsic, std::vector<Value*> args) {
  CHECK(intrinsic == Intrinsics::kDoubleDoubleToRawLongBits);
  return irb_->CreateBitCast(args.at(0), irb_->getJLongTy());
}

Value* IntrinsicHelper::CallFloatFloatToRawIntBits(
    Intrinsics intrinsic, std::vector<Value*> args) {
  CHECK(intrinsic == Intrinsics::kFloatFloatToRawIntBits);
  return irb_->CreateBitCast(args.at(0), irb_->getJIntTy());
}

Value* IntrinsicHelper::Call_CMathRound(Intrinsics intrinsic,
    HInvoke* invoke, std::vector<Value*> callee_args) {
  DIE_ANDROID10();
  Function* funcRound = nullptr;

  CHECK(intrinsic == Intrinsics::kMathRoundFloat ||
        intrinsic == Intrinsics::kMathRoundDouble)
      << "Not Math.round";

  const bool f32= intrinsic == Intrinsics::kMathRoundFloat;
  if (f32) {
    funcRound = C_MathRoundFloat();
  } else {
    funcRound = C_MathRoundDouble();
  }

  Value* result = irb_->CreateCall(funcRound, callee_args);
  Value* casted_result = result;

  DataType::Type htype = invoke->GetType();
  Type* ltype = irb_->getType(htype);
  CHECK(DataType::IsIntegralType(htype)) << "round must result in integral type";

  casted_result = irb_->CreateFPToSI(result, ltype);
  return casted_result;
}

Value* IntrinsicHelper::CallMathCeil(Intrinsics intrinsic, HInvoke* invoke,
                                             std::vector<Value*> callee_args) {
  CHECK(intrinsic == Intrinsics::kMathCeil) << "Not Math.ceil";
  Function* funcCeil = MathCeil();

  return irb_->CreateCall(funcCeil, callee_args);
}

Value* IntrinsicHelper::CallSystemArrayCopyChar(
    HGraphToLLVM* HL, Intrinsics intrinsic, HInvoke* invoke,
    std::vector<Value*> callee_args) {
  CHECK(intrinsic == Intrinsics::kSystemArrayCopyChar) << "Not SystemArrayCopyChar";
  CHECK(callee_args.size() == 5)
    << __func__ << ": must have 5 arguments";

  callee_args.at(0)->setName("src");
  callee_args.at(1)->setName("p1");
  callee_args.at(2)->setName("dest");
  callee_args.at(3)->setName("p2");
  callee_args.at(4)->setName("length");

  int cnt = 0;
  for (Value* arg : callee_args) {
    D3LOG(INFO) << "ARG:" << ++cnt << ":" << arg->getName().str()
              << ":type:" << Pretty(arg->getType());
  }
  DLOG(FATAL) << "DYING";

  return nullptr;
}

Value* IntrinsicHelper::CallMathFloor(Intrinsics intrinsic, HInvoke* invoke,
                                              std::vector<Value*> callee_args) {
  CHECK(intrinsic == Intrinsics::kMathFloor) << "Not Math.floor";
  Function* funcFloor = MathFloor();
  return irb_->CreateCall(funcFloor, callee_args);
}

Value* IntrinsicHelper::llvm_absf(HGraphToLLVM *HL, HInstruction* h) {
  DLOG(INFO) << __func___;
  DataType::Type htype = h->GetType();
  Type* ltype = irb_->getType(htype);
  CHECK(DataType::IsFloatingPointType(htype));

  Value* input = HL->getValue(h->InputAt(0));

  std::vector<Type *> ty(1, ltype); // to get float/double variant
  Function *F = Intrinsic::getDeclaration(mod_, Intrinsic::fabs, ty);
  std::vector<Value*> args;
  args.push_back(input);
  // call directly the llvm intrinsic
  Value* result = irb_->CreateCall(F, args);
  return result;
}

Value* IntrinsicHelper::llvm_MaxOrMinf(HGraphToLLVM *HL,
    HBinaryOperation* h, Value* lhs, Value* rhs) {
  DLOG(INFO) << __func___;
  _LOGLLVM4(irb_, INFO, __func__);
  DataType::Type htype = h->GetType();
  Type* ltype = irb_->getType(htype);
  CHECK(DataType::IsFloatingPointType(htype));

  std::vector<Type *> ty(1, ltype); // to get float/double variant
  Function *f = nullptr;

  if(h->IsMax()) {
    f=Intrinsic::getDeclaration(mod_, Intrinsic::maxnum, ty);
  } else {
    f=Intrinsic::getDeclaration(mod_, Intrinsic::minnum, ty);
  }

  std::vector<Value*> args;
  args.push_back(lhs);
  args.push_back(rhs);
  // call directly the llvm intrinsic
  Value* result = irb_->CreateCall(f, args);
  return result;
}

Value* IntrinsicHelper::canonical_MaxOrMin(HGraphToLLVM *HL,
    HBinaryOperation* h, Value* lhs, Value* rhs) {
  D4LOG(INFO) << __func___;
  _LOGLLVM4(irb_, INFO, __func__);

  bool is_signed = IRBuilder::IsSigned(h->GetType());
  // __ Cmp(lhs, rhs);
  Value* lcond = irb_->mCreateCmpGT(false, is_signed, lhs, rhs);
  // __ Csel(dst, lhs, rhs, instr->IsMin() ? lt : gt);
  Value* ltruev, *lfalsev;
  if(h->IsMax()) {
    ltruev = lhs;
    lfalsev = rhs;
  } else {
    ltruev = rhs;
    lfalsev = lhs;
  }

  const char* reason =
    SelectInst::areInvalidOperands(lcond, ltruev, lfalsev);
  CHECK(reason == nullptr) << "SelectInst: " << reason;
  return irb_->CreateSelect(lcond, ltruev, lfalsev);
}


Value* IntrinsicHelper::canonical_absi(HGraphToLLVM *HL, HInstruction* h) {
  DLOG(INFO) << __func___;
  DataType::Type htype = h->GetType();
  CHECK(!DataType::IsFloatingPointType(htype));
  // __ Cmp(in_reg, Operand(0));
  // __ Cneg(out_reg, in_reg, lt);
  // Goal assembly:
  // Int:
  // cmp w2, #0x0 (0)
  // cneg w0, w2, lt
  // Long:
  // cmp x2, #0x0 (0)
  // cneg x0, x2, lt
  // https://godbolt.org/z/DNz_4Q
  // https://godbolt.org/z/2Vu6v9
  // LLVM output:
  // cmp	x23, #0x0
  // cneg	x22, x23, mi

  HInstruction* hinput = h->InputAt(0);
  Value* linput = HL->getValue(hinput);
  Value* zero = irb_->getJZero(htype);

  // Lower than zero:
  // %2 = icmp slt i32 %0, 0
  Value* lcond = irb_->mCreateCmpLT(false, true /*signed*/, linput, zero);

  // Create negative version:
  // %3 = sub nsw i32 0, %0
  Value* lneginput = irb_->mCreateSub(false, zero, linput);

  // Select the number:
  // %4 = select i1 %2, i32 %3, i32 %0
  const char* reason =
    SelectInst::areInvalidOperands(lcond, lneginput, linput);
  CHECK(reason == nullptr) << "SelectInst: " << reason;
  Value* result = irb_->CreateSelect(lcond, lneginput, linput);
  return result;
}

Value* IntrinsicHelper::CallMathAbs(
    HInvoke* invoke, std::vector<Value*> callee_args) {
  DataType::Type htype = invoke->GetType();
  Type* ltype = irb_->getType(htype);
  Type* doubleTy = Type::getDoubleTy(mod_->getContext());

  bool is_signed = IRBuilder::IsSigned(htype);
  bool is_fp = DataType::IsFloatingPointType(htype);
  CHECK(is_signed) << "CallMathAbs: abs of unsigned has no effect!";

  Function* abs_f = nullptr;
  if (htype == DataType::Type::kFloat32) {
    abs_f = MathAbsFloat();
  } else if (htype == DataType::Type::kFloat64) {
    abs_f = MathAbsDouble();
  } else {  // use double in all other cases (might have to cast twice)
    DLOG(FATAL) << "CallMathAbs: type: " << htype;
    UNREACHABLE();
  }

  CHECK(callee_args.size() == 1) << "Math.abs calle_args: size must be 1";
  Value* val = *callee_args.begin();
  Value* casted_val = val;
  callee_args.clear();

  if (!is_fp) {  // all non FP will use double version of abs
    casted_val = irb_->CreateSIToFP(val, doubleTy);
  }

  callee_args.push_back(casted_val);
  Value* result = irb_->CreateCall(abs_f, callee_args);

  Value* casted_result = result;

  if (!is_fp) {  // cast back
    casted_result = irb_->CreateFPToSI(result, ltype);
  }

  return casted_result;
}

FunctionType* IntrinsicHelper::GetMinMaxTy(DataType::Type type) {
  Type* ty = irb_->getType(type);
  Type* ret_type = ty;
  std::vector<Type*> args_type{ty, ty};

  return FunctionType::get(ret_type, args_type, false);
}

std::string IntrinsicHelper::GetTypeName(DataType::Type type) {
  std::stringstream ss;
  ss << type;
  std::string stype = ss.str();
  DLOG(INFO) << "CHECK_LLVM: GetTypeName: " << stype;
  return stype;
}

std::string IntrinsicHelper::GetMinMaxName(bool is_min, DataType::Type type) {
  std::string result = "Math_";
  result+=(is_min?"min":"max");
  result += GetTypeName(type) + GetTypeName(type);
  return result;
}

Function* IntrinsicHelper::MinMax(
    Intrinsics intrinsic, DataType::Type type) {
  bool is_min;
  switch (intrinsic) {
    case Intrinsics::kMathMinIntInt:
    case Intrinsics::kMathMinLongLong:
    case Intrinsics::kMathMinFloatFloat:
    case Intrinsics::kMathMinDoubleDouble:
      is_min = true;
      break;
    case Intrinsics::kMathMaxIntInt:
    case Intrinsics::kMathMaxFloatFloat:
    case Intrinsics::kMathMaxLongLong:
    case Intrinsics::kMathMaxDoubleDouble:
      is_min = false;
      break;
    default:
      DLOG(FATAL) << "Should be Math.min or Math.max";
      UNREACHABLE();
  }

  std::string name = GetMinMaxName(is_min, type);
  if (Math_min_max_.find(name) != Math_min_max_.end()) {
    return Math_min_max_[name];
  }

  D2LOG(INFO) << "Creating function: " << name;
  BasicBlock* old_bb = irb_->GetInsertBlock();
  FunctionType* fTy = GetMinMaxTy(type);
  Function* f = Function::Create(
      fTy, Function::LinkOnceODRLinkage,
      name, mod_);

  Function::arg_iterator arg_iter(f->arg_begin());
  Argument* num1 = &*arg_iter++;
  Argument* num2 = &*arg_iter++;
  num1->setName("num1");
  num2->setName("num2");

  BasicBlock* entry_block =
      BasicBlock::Create(irb_->getContext(), "entry", f);
  BasicBlock* true_block =
      BasicBlock::Create(irb_->getContext(), "true", f);
  BasicBlock* false_block =
      BasicBlock::Create(irb_->getContext(), "false", f);

  bool is_fp = DataType::IsFloatingPointType(type);
  bool is_signed = IRBuilder::IsSigned(type);
  irb_->SetInsertPoint(entry_block);
  Value* cmp = nullptr;
  if (is_min) {
    cmp = irb_->mCreateCmpLT(is_fp, is_signed, num1, num2);
  } else {
    cmp = irb_->mCreateCmpGT(is_fp, is_signed, num1, num2);
  }
  irb_->CreateCondBr(cmp, true_block, false_block);

  irb_->SetInsertPoint(true_block);
  irb_->CreateRet(num1);

  irb_->SetInsertPoint(false_block);
  irb_->CreateRet(num2);

  irb_->SetInsertPoint(old_bb);
  Math_min_max_.insert(std::pair<std::string, Function*>(
      name, f));
  return f;
}

#ifdef ART_MCR_ANDROID_6
Function* IntrinsicHelper::CharAt(
    HInvoke* invoke, HGraphToLLVM* hgraph_to_llvm) {
  D2LOG(INFO) << "CharAt";
  if (String_CharAt_ != nullptr) {
    return String_CharAt_;
  }

  DLOG(INFO) << "creating method";
  BasicBlock* old_bb = irb_->GetInsertBlock();

  std::vector<Type*> argsTy;
  argsTy.push_back(irb_->getJObjectTy()->getPointerTo());
  argsTy.push_back(irb_->getJIntTy());
  FunctionType* ty =
      FunctionType::get(irb_->getJIntTy(), argsTy, false);

  String_CharAt_ =
      Function::Create(ty, Function::LinkOnceODRLinkage,
                               "intrinsic_String_CharAt", mod_);

  Function::arg_iterator arg_iter(String_CharAt_->arg_begin());
  Argument* str_obj = &*arg_iter++;
  Argument* idx = &*arg_iter++;
  str_obj->setName("str_obj");
  idx->setName("idx");

  BasicBlock* entry =
      BasicBlock::Create(*ctx_, "entry", String_CharAt_);
  BasicBlock* range_check_failed =
      BasicBlock::Create(*ctx_, "range_check_failed", String_CharAt_);
  BasicBlock* range_check_succeed =
      BasicBlock::Create(*ctx_, "range_check_succeed", String_CharAt_);

  irb_->SetInsertPoint(entry);
  Value* str_len = hgraph_to_llvm->GetStringLength(str_obj);
  TODO_LLVM("Android10: result will be wrong!");
  // For this I should check the ASM..

  irb_->SetInsertPoint(entry);
  Value* is_out_of_range = irb_->mCreateCmpGE(false, true, idx, str_len);
  irb_->CreateCondBr(is_out_of_range, range_check_failed, range_check_succeed);

  irb_->SetInsertPoint(range_check_failed);
  std::vector<Value*> args_err;
  args_err.push_back(irb_->AndroidLogSeverity(ERROR));
  args_err.push_back(irb_->mCreateGlobalStringPtr(
      "String.CharAt: out of range."
      "\nIndex: %d String.Length: %d DexPc: %u"));
  args_err.push_back(idx);
  args_err.push_back(str_len);
  args_err.push_back(irb_->getJUnsignedInt(invoke->GetDexPc()));
  irb_->CreateCall(fh_->AndroidLog(), args_err);
  fh_->CallExit(irb_, 1);

  irb_->SetInsertPoint(range_check_succeed);

  Value* value_offset =
      irb_->getJUnsignedInt(mirror::String::ValueOffset().Int32Value());

  size_t shift_amount = DataType::Size(DataType::Type::kUint16);
  Value* shifted_idx = irb_->CreateShl(
      idx, irb_->getJInt(shift_amount));

  Value* char_offset = irb_->mCreateAdd(false, value_offset, shifted_idx); 
  std::vector<Value*> offst;
  offst.push_back(char_offset);
  Value* gep = irb_->CreateInBoundsGEP(str_obj, offst);
  Value* cast = irb_->CreateBitCast(gep,
                                            irb_->getJCharTy()
                                                ->getPointerTo());
  Value* res = irb_->CreateLoad(cast);

  // it's char, so do a Zext (unsigned)
  bool is_signed = IRBuilder::IsSigned(
      DataTypeFromPrimitive(Primitive::kPrimChar));
  Value* casted_res = irb_->CreateIntCast(res, irb_->getJIntTy(), is_signed);
  irb_->CreateRet(casted_res);

  irb_->SetInsertPoint(old_bb);
  return String_CharAt_;
}
#endif

Value* IntrinsicHelper::CallMathSin(ExtraIntrinsics intrinsic, HInvoke* invoke,
                                            std::vector<Value*> callee_args) {
  CHECK(intrinsic == ExtraIntrinsics::kMathSin) << "Not Math.Sin";
  Function* funcSin = MathSin();
  return irb_->CreateCall(funcSin, callee_args);
}

Value* IntrinsicHelper::CallMathPow(ExtraIntrinsics intrinsic, HInvoke* invoke,
                                            std::vector<Value*> callee_args) {
  CHECK(intrinsic == ExtraIntrinsics::kMathPow) << "Not Math.Pow";
  return irb_->CreateCall(MathPow(), callee_args);
}

Value* IntrinsicHelper::CallCMathTan(std::vector<Value*> args) {
  return irb_->CreateCall(MathTan(), args);
}

Value* IntrinsicHelper::CallCMathAsin(std::vector<Value*> args) {
  return irb_->CreateCall(MathAsin(), args);
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
