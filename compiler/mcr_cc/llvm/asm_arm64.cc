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

#include "asm_arm64.h"

#include <llvm/IR/DerivedTypes.h>
#include "art_method.h"
#include "llvm_macros_irb.h"
#include "asm_arm_thumb.h"
#include "hgraph_to_llvm.h"
#include "ir_builder.h"
#include "mcr_rt/art_impl_arch_arm-inl.h"
#include "thread.h"

using namespace ::llvm;

namespace art {
namespace LLVM {
namespace Arm64 {

Value* GetMrRegister(IRBuilder* irb) {
  return __Mov(irb, LLVM_MR_REG_ARM64);
}

void GenerateMemoryBarrier(IRBuilder* irb, MemBarrierKind kind) {
  ArmThumb::GenerateMemoryBarrier(irb, kind);
}

/**
 * StoreRelease: used for volatile settres
 *
 * Could be done with LLVM Atomics (like GenerateClassInitializationCheck)
 * (there was an issue with a some unresolved symbols thought)
 *
 * @param instruction
 * @param type
 * @param src
 * @param dst uses HeapOperand so must be wREG
 * @param needs_null_check
 */
void StoreRelease(IRBuilder* irb,
    HInstruction* instruction,
    DataType::Type type,
    Value* src, Value* base,
    Value* offset, bool needs_null_check) {

  // __ Add(temp_base, dst.GetBaseRegister(), op);
  // MemOperand base = MemOperand(temp_base);
  std::vector<Type*> params{irb->getVoidPointerType(),
    irb->getJIntTy()};
  Value* temp_base = __Add(irb, base, offset, params);
  temp_base->setName("temp_base");

  // Ensure that between store and MaybeRecordImplicitNullCheck there are no pools emitted.
  bool is32=false;
  switch (type) {
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kInt8:
      {
        // VERIFIED
        // example:
        // add x16, x1, #0x28 (40)
        // stlrb wzr, [x16]
        __Stlrb(irb, src, temp_base);
      }
      break;
    case DataType::Type::kUint16:
    case DataType::Type::kInt16:
      {
        // VERIFIED
        __Stlrh(irb, src, temp_base);
      }
      break;
    case DataType::Type::kReference:
      {
        VERIFY_LLVM("Reference");
        __Stlrb(irb, src, temp_base);
      } break;
    case DataType::Type::kInt32:
      is32=true;
      FALLTHROUGH_INTENDED;
    case DataType::Type::kInt64:
      // for 64 bits might have to use X instead of w
      {
        // __ stlr(Register(src), base);
        __Stlr(irb, src, temp_base, is32, false);
        if(!is32){
          VERIFY_LLVM("64bit int");
        } else {
          VERIFY_LLVM("32bit int/kRef");
        }

      }
      break;
    case DataType::Type::kFloat32:
      is32=true;
      FALLTHROUGH_INTENDED;
    case DataType::Type::kFloat64:
      {
        DIE_TODO << type;

        // float:
        // fmov w17, s0
        // double:
        // TODO: CreateFPToSI (convert fp to signed int)
        Value* temp_src = __Fmov(irb, irb->getJFloat(0), is32);
        // CHECK replaced this: https://godbolt.org/z/9qK6c9
        // Value* temp_src = __Fmov(irb, src, is32);
        // if (src.IsZero()) {
        //   // The zero register is used to avoid synthesizing zero constants.
        //   temp_src = Register(src);
        // } else {
        //   temp_src = src.Is64Bits() ? temps.AcquireX() : temps.AcquireW();
        //   __ Fmov(temp_src, FPRegister(src));
        // }

        {
          // ExactAssemblyScope eas(masm, kInstructionSize, CodeBufferCheckScope::kExactSize);
          __Stlr(irb, temp_src, temp_base, is32, true);
          // __ stlr(temp_src, base);
        }
        break;
      }
    case DataType::Type::kUint32:
    case DataType::Type::kUint64:
    case DataType::Type::kVoid:
      LOG(FATAL) << "Unreachable type " << type;
  }

  if (needs_null_check) {
    MaybeRecordImplicitNullCheck(instruction);
  }
}

Value* LoadAcquire(IRBuilder* irb,HInstruction* instruction,
    DataType::Type type,
    Value* base, Value* offset, bool needs_null_check) {
  std::vector<Type*> params{irb->getVoidPointerType(),
    irb->getJIntTy()};
  Value* temp_base = __Add(irb, base, offset, params);
  temp_base->setName("temp_base");

  Value* value=nullptr;
  switch (type) {
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kInt8:
      {
        value=__Ldarb(irb, temp_base);
        if (needs_null_check) {
          MaybeRecordImplicitNullCheck(instruction);
        }
        if (type == DataType::Type::kInt8) {
          value=__Sbfx(irb, true, value, value, 0, DataType::Size(type) * kBitsPerByte);
        }
      }
      break;
    case DataType::Type::kUint16:
    case DataType::Type::kInt16:
      {
        value=__Ldarh(irb, temp_base);
        if (needs_null_check) {
          MaybeRecordImplicitNullCheck(instruction);
        }
      }
      if (type == DataType::Type::kInt16) {
        value=__Sbfx(irb, true, value, value, 0, DataType::Size(type) * kBitsPerByte);
      }
      break;
    case DataType::Type::kReference:
    case DataType::Type::kInt32:
      // is32=true;
      FALLTHROUGH_INTENDED;
    case DataType::Type::kInt64:
      {
        value=__Ldar(irb, temp_base, type);
        if (needs_null_check) {
          MaybeRecordImplicitNullCheck(instruction);
        }
      }
      break;
    case DataType::Type::kFloat32:
      // is32=true;
      FALLTHROUGH_INTENDED;
    case DataType::Type::kFloat64:
      {
        DIE_TODO << type;
        // {
        //   value=__Ldar(irb, temp_base);
        //   if (needs_null_check) {
        //     MaybeRecordImplicitNullCheck(instruction);
        //   }
        // }
        // value=__Fmov(value, temp_base);
        // __ Fmov(FPRegister(dst), temp);
        break;
      }
    case DataType::Type::kUint32:
    case DataType::Type::kUint64:
    case DataType::Type::kVoid:
      LOG(FATAL) << "Unreachable type " << type;
  }
  CHECK(value!=nullptr);
  return value;
}


/**
 * @brief 
 *
 * ofst = Thread::ThreadFlagsOffset ..
 *  __ Ldrh(temp, MemOperand(tr, ofst)
 * 
 *  FULL EXAMPLE (DoBinaryChecks):
 *
 *    ldrh w16, [tr] ; state_and_flags
 *    cbnz w16, #+0x7c (addr lbl1) 
 *    ...
 *    lbl2:
 *    mov w16, #0x3d09
 *    cmp w0, w16, lsl #6
 *    b.ge #+0x54 (addr return)
 *    ...
 *    ldrh w16, [tr] ; state_and_flags
 *    cbz w16, #-0x54 (addr lbl1)
 *    ...
 *    lbl1:
 *    ldr lr, [tr, #1320] ; pTestSuspend
 *    blr lr
 *
 */
Value* LoadStateAndFlagsASM(IRBuilder* irb) {
  uint32_t offset_state_and_flags =
    Thread::ThreadFlagsOffset<kArm64PointerSize>().SizeValue();
  Value* v = Ldrh(irb, LLVM_THREAD_REG_ARM64, offset_state_and_flags);
  v->setName("state_and_flags");
  return v;
}

Value* PoisonHeapReference(IRBuilder* irb, Value* reg) {
  AVOID_ASM;
  VERIFY_LLVM_;
  return __wNeg(irb, reg);
}

// same as poision
Value* UnpoisonHeapReference(IRBuilder* irb, Value* reg) {
  AVOID_ASM;
  VERIFY_LLVM_;
  return __wNeg(irb, reg);
}

// Move in llvm load ops
Value* MaybePoisonHeapReference(IRBuilder* irb, Value* reg) {
  if (kPoisonHeapReferences) {
    return PoisonHeapReference(irb, reg);
  }
  return reg;
}

// Mmove in llvm load ops
Value* MaybeUnpoisonHeapReference(IRBuilder* irb, Value* reg) {
  if (kPoisonHeapReferences) {
    return UnpoisonHeapReference(irb, reg);
  }
  return reg;
}

// TODO_LLVM GenerateBitstringTypeCheckCompare

Value* __Bic(IRBuilder* irb, Value* lhs, Value* rhs, bool i32) {
  return ___BinaryOp(irb, "bic", lhs, rhs, i32);
}

Value* __Orn(IRBuilder* irb, Value* lhs, Value* rhs, bool i32) {
  return ___BinaryOp(irb, "orn", lhs, rhs, i32);
}

Value* __Eon(IRBuilder* irb, Value* lhs, Value* rhs, bool i32) {
  return ___BinaryOp(irb, "eon", lhs, rhs, i32);
}

Value* __Ror(IRBuilder* irb, Value* lhs, Value* rhs, bool i32) {
  return ___BinaryOp(irb, "ror", lhs, rhs, i32);
}

Value* __Ror(IRBuilder* irb, Value* lhs, uint64_t imm2, bool i32) {
  return ___BinaryOpImm2(irb, "ror", lhs, imm2, i32);
}

void GenerateMarkingRegisterCheck(IRBuilder* irb, int code) {
  // The Marking Register is only used in the Baker read barrier configuration.
  DCHECK(kEmitCompilerReadBarrier);
  DCHECK(kUseBakerReadBarrier);
  TODO_LLVM("");

  UNUSED(code);
  // Sample code:
  // vixl::aarch64::Register mr = reg_x(MR);  // Marking Register.
  // vixl::aarch64::Register tr = reg_x(TR);  // Thread Register.
  // vixl::aarch64::Label mr_is_ok;

  // // temp = self.tls32_.is.gc_marking
  // ___ Ldr(temp, MemOperand(tr, Thread::IsGcMarkingOffset<kArm64PointerSize>().Int32Value()));
  // // Check that mr == self.tls32_.is.gc_marking.
  // ___ Cmp(mr.W(), temp);
  // ___ B(eq, &mr_is_ok);
  // ___ Brk(code);
  // ___ Bind(&mr_is_ok);
}

void MaybeRecordImplicitNullCheck(HInstruction* instruction) {
  UNUSED(instruction);
}

#include "llvm_macros_undef.h"

}  // namespace Arm64
}  // namespace LLVM
}  // namespace art
