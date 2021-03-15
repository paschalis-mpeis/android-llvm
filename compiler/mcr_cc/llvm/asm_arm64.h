/**
 * LLVM inline assembly:
 * Performing necessary low level operations for implementing a few 
 * instructions, and for calling the runtime entrypoints.
 *
 * LLVM ASM Template info:
 * https://llvm.org/docs/LangRef.html#asm-template-argument-modifiers
 *
 * NOTE:
 * HeapOperand: A heap reference must be 32bit, so fit in a W register.
 *
 * The inline ASM must be used as little as possible. It limits
 * the LLVM optimizations, regardless of whether the necessary ASM
 * constraints are provided.
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
#ifndef ART_COMPILER_LLVM_ASM_ARM64_H_
#define ART_COMPILER_LLVM_ASM_ARM64_H_

#include <llvm/IR/InlineAsm.h>
#include "optimizing/nodes.h"

using namespace ::llvm;
namespace art {
namespace LLVM {
class IRBuilder;
class HGraphToLLVM;
namespace Arm64 {

  void SaveCalleeRegisters(IRBuilder* irb, HGraphToLLVM* HL);
  void RestoreCalleeRegisters(IRBuilder* irb, HGraphToLLVM* HL);

  void SetThreadRegister(IRBuilder* irb, Value* thread_self);
  Value* GetThreadRegister(IRBuilder* irb);
  Value* GetMrRegister(IRBuilder* irb);
  Value* LoadStateAndFlagsASM(IRBuilder* irb);

  void __Mov(IRBuilder* irb,  std::string to, Value* from);
  Value* __Mov(IRBuilder* irb, std::string from, bool w=false);
  Value* __Mov(IRBuilder* irb, Value* arg1, Type* inTy, Type* outTy, bool i32);
  Value* __Fmov(IRBuilder* irb, Value* llvmreg, bool is32);
  Value* __Fmov_toFP(IRBuilder* irb, Value* llvmreg, bool i32);
  Value* __LoadFpFromUnion(IRBuilder* irb, Value* arg1, bool i32); 
  Value* __Neg(IRBuilder* irb, Value* lreg, bool w=false);
  Value* __wNeg(IRBuilder* irb, Value* lreg);


  // Binary operations
  Value* ___BinaryOp(IRBuilder* irb, std::string op,
      Value* lhs, Value* rhs, bool i32);
  Value* ___BinaryOpImm2(IRBuilder* irb, std::string op,
      Value* lhs, u_int32_t imm2, bool i32);
  Value* __Bic(IRBuilder* irb, Value* lhs, Value* rhs, bool i32);
  Value* __Orn(IRBuilder* irb, Value* lhs, Value* rhs, bool i32);
  Value* __Eon(IRBuilder* irb, Value* lhs, Value* rhs, bool i32);
  Value* __Ror(IRBuilder* irb, Value* lhs, Value* rhs, bool i32);
  Value* __Ror(IRBuilder* irb, Value* lhs, uint64_t imm2, bool i32);

  Value* __Add(IRBuilder* irb, Value* lreg1, Value* lreg2,
      std::vector<Type*> tp_params, bool w=false);
  Value* __Add(IRBuilder* irb, Value* lreg1, uint32_t offset, bool w=false);
  Value* __Ldarb(IRBuilder* irb, Value* lreg);
  Value* __Ldarh(IRBuilder* irb, Value* arg1);
  Value* __Ldar(IRBuilder* irb, Value* arg1, DataType::Type type);

  Value* __Sbfx(IRBuilder* irb, bool i32, Value* arg1, Value* arg2,
      uint32_t imm1, uint32_t imm2);

  void __Stlrb(IRBuilder* irb, Value *src, Value* base);
  void __Stlrh(IRBuilder* irb, Value *src, Value* base);
  void __Stlr(IRBuilder* irb, Value *src, Value* base, bool is32, bool isFP);
  Value* wLdr(IRBuilder* irb, Value* arg1, uint32_t offset);
  Value* Ldr(IRBuilder* irb, Value* arg1, bool i32);
  Value* Ldr_ptr(IRBuilder* irb, Value* arg1);
  Value* sLdr(IRBuilder* irb, Value* arg1, bool is32);
  Value* __Ldr(IRBuilder* irb, Value* arg1, Value* argofst, bool is32);
  Value* __Ldr(IRBuilder* irb, Value* arg1, uint32_t offset, bool is32 = false);
  Value* Ldr(IRBuilder* irb, std::string xreg, uint32_t offset);
  Value* Ldrh(IRBuilder* irb, std::string xreg, uint32_t offset);
  Value* wLdr(IRBuilder* irb, std::string xreg, uint32_t offset);
  Value* wLdr(IRBuilder* irb, Value* arg1, Value* argofst);
  void wStrb(IRBuilder* irb, Value* arg1, Value* arg2, Value* arg3);
  Value* wLsr(IRBuilder* irb, Value* arg1, uint32_t offset);

  void Mov(IRBuilder* irb, std::string regname, Value* llvmreg);
  void MovConst(IRBuilder* irb, std::string regname, uint32_t offset);

  uint32_t offset(QuickEntrypointEnum qpoint);
  void ClobberLR(IRBuilder* irb);
  Value* IsClassInited(IRBuilder* irb, Value* klass, uint32_t offset);

  // Heap operations
  Value* PoisonHeapReference(IRBuilder* irb, Value* reg);
  Value* UnpoisonHeapReference(IRBuilder* irb, Value* reg);
  Value* MaybePoisonHeapReference(IRBuilder* irb, Value* reg);
  Value* MaybeUnpoisonHeapReference(IRBuilder* irb, Value* reg);

  Value* LoadAcquire(IRBuilder* irb, HInstruction* instruction,
      DataType::Type type, Value* base,
      Value* offset, bool needs_null_check);

  void StoreRelease(IRBuilder* irb,
      HInstruction* instruction,
      DataType::Type type,
      Value* src,
      Value* base,
      Value* offset,
      bool needs_null_check);

  // Entrypoints
  void _voidZeroArgsDowncall(IRBuilder* irb, int32_t offset);

  void _voidOneArgDowncall(IRBuilder* irb,
      int32_t entrypoint_offset,
      Value* arg1);

  Value* _OneArgDowncall(IRBuilder* irb,
      int32_t entrypoint_offset,
      Value* arg1);

  Value* _OneArgDowncallI(
      IRBuilder* irb, int32_t entrypoint_offset, uint32_t imm1);

  void _voidTwoArgDowncall(
      IRBuilder* irb, int32_t entrypoint_offset,Value* arg1, Value* arg2);

  Value* _TwoArgDowncall(
      IRBuilder* irb, int32_t entrypoint_offset,
      Value* arg1, Value* arg2, Type* retTy); 

  Value* _ThreeArgDowncallII(IRBuilder* irb,
      int32_t entrypoint_offset,
      Value* arg1, uint32_t imm2, uint32_t imm3);

  Value* _ThreeArgDowncallI(IRBuilder* irb, int32_t entrypoint_offset,
      std::vector<Type*> tp_params,
      Value* arg1, Value *arg2, uint32_t imm3);

  Value* _ThreeArgDowncall(IRBuilder* irb,
    int32_t entrypoint_offset,
    std::vector<Type*> params,
    Value* arg1, Value *arg2, Value *arg3);

  void _FourArgDowncall(IRBuilder* irb,
      int32_t entrypoint_offset,
      std::vector<Type*> params,
      Value* arg1, Value *arg2, Value *arg3, Value* arg4);

  Value* _FiveArgDowncall(IRBuilder* irb,
      int32_t entrypoint_offset,
      std::vector<Type*> params,
      Value* arg1, Value *arg2, Value *arg3, Value* arg4, Value* arg5);
  Value* _FiveArgDowncallII(IRBuilder* irb,
      int32_t offset,
      Value* arg1, Value *arg2, Value *arg3,
      uint32_t imm4, uint32_t imm5);

  // Runtime support
  void MaybeRecordImplicitNullCheck(HInstruction* instruction);

  // Entrypoints
  void art_llvm_invoke_quick__INDIRECT(IRBuilder* irb,
      Value* art_method, Value* args,
      Value* args_size, Value* jvalue, bool is_static);

  void art_quick_invoke_stub(IRBuilder* irb,
      Value* art_method, Value* qargs,
      Value* qargs_size, Value* jvalue, Value* shorty, bool is_static) ;

  void art_quick_test_suspend(IRBuilder* irb);
  void art_llvm_test_suspend(IRBuilder* irb);

  void GenerateMemoryBarrier(IRBuilder* irb, MemBarrierKind kind);
  void GenerateMarkingRegisterCheck(IRBuilder* irb, int code);

  Value* GetQuickEntrypoint(IRBuilder* irb, QuickEntrypointEnum qpoint);

}  // namespace Arm64
}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_ASM_ARM64_H_
