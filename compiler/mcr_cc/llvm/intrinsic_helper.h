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
#ifndef ART_COMPILER_LLVM_INTRINSIC_HELPER_H_
#define ART_COMPILER_LLVM_INTRINSIC_HELPER_H_

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "intrinsic_helper-enum.h"
#include "hgraph_printers.h"
#include "llvm_info.h"
#include "optimizing/nodes.h"
#include "optimizing/data_type.h"

using namespace ::llvm;

namespace art {

class HGraphFilePrettyPrinter;
namespace LLVM {

class IRBuilder;
class FunctionHelper;
class HGraphToLLVM;

class IntrinsicHelper {
 public:
  explicit IntrinsicHelper(LLVMContext& context,
                           Module& module, IRBuilder* irb,
                           FunctionHelper* fh)
      : ctx_(&context),
        mod_(&module),
        irb_(irb),
        fh_(fh) {
  }

  void setHgraphPrinter(art::HGraphFilePrettyPrinter* prt) {
    prt_ = prt;
  }

  ExtraIntrinsics GetExtraIntrinsic(std::string pretty_method);

  void HandleExtraIntrinsic(
      HGraphToLLVM* HL, HInvoke* invoke,
      std::vector<Value*> callee_args, std::string callee_name);

  bool UnimplementedIntrinsic(Intrinsics intrinsic);
  bool MustHandle(Intrinsics intrinsic);
  bool ExcludeFromHistogram(Intrinsics intrinsic);
  bool IsSimplified(Intrinsics intrinsic);
  void HandleIntrinsic(HGraphToLLVM* hgraph_to_llvm, HInvoke* invoke,
      std::vector<Value*> callee_args, std::string callee_name);
  bool HandleSimplified(HGraphToLLVM* HL, HInvoke* invoke,
      std::vector<Value*> callee_args, std::string callee_name);

  Value* LoadThreadCurrentThread(HGraphToLLVM* HL);

  // LLVM and canonical intrinsics
  Value* llvm_absf(HGraphToLLVM *HL, HInstruction* h);
  Value* canonical_absi(HGraphToLLVM *HL, HInstruction* h);
  Value* llvm_MaxOrMinf(HGraphToLLVM *HL,
      HBinaryOperation* h, Value* lhs, Value* rhs);
  Value* llvm_bitcount(HInvoke* h, Intrinsics intrinsic,
      std::vector<Value*> callee_args);
  Value* llvm_count_zeros(
      HInvoke* h, Intrinsics intrinsic,
      std::vector<Value*> callee_args, bool leading);
  Value* canonical_MaxOrMin(HGraphToLLVM *HL,
      HBinaryOperation* h, Value* lhs, Value* rhs);

  void LlvmInvariantStart(Value* variable);
  void LlvmInvariantStart(GlobalVariable* variable);
  Value* llvm_fshl(DataType::Type type, std::vector<Value*> args);

 private:
  LLVMContext* ctx_;
  Module* mod_;
  IRBuilder* irb_;
  FunctionHelper* fh_;
  art::HGraphFilePrettyPrinter* prt_;

  // Math.min, Math.max variants
  std::map<std::string, Function*> Math_min_max_;

  // String.CharAt
  Function* String_CharAt_ = nullptr;

  std::string GetTypeName(DataType::Type type);
  std::string GetMinMaxName(bool is_min, DataType::Type type);
  Function* MinMax(Intrinsics intrinsic, DataType::Type type);
  FunctionType* GetMinMaxTy(DataType::Type type);

  Function* CharAt(HInvoke* invoke, HGraphToLLVM* hgraph_to_llvm);
 
  Value* CallMathAbs(
      HInvoke* invoke, std::vector<Value*> callee_args);

  Function* MathAbsDouble();
  Function* MathAbsFloat();
  void SetAttributesNoUnwindReadNone(Function* f);
  void SetAttributesNoUnwind(Function* f);
  Value* CallIntBitsToFloat(Intrinsics intrinsic, std::vector<Value*> args);
  Value* CallLongBitsToDouble(Intrinsics intrinsic, std::vector<Value*> args);
  Value* CallDoubleDoubleToRawLongBits(
      Intrinsics intrinsic, std::vector<Value*> args);
  Value* CallFloatFloatToRawIntBits(
      Intrinsics intrinsic, std::vector<Value*> args);
  Value* Call_CMathRound(Intrinsics intrinsic, HInvoke* invoke,
      std::vector<Value*> callee_args);

  Value* callLlvmDoubleIntrinsic(Intrinsic::ID id, Intrinsics intrinsic,
      HInvoke* invoke, std::vector<Value*> args);

  Value* llvm_round(Intrinsics iintr, HInvoke* h, std::vector<Value*> args);
  Value* llvm_sqrt(Intrinsics intr, HInvoke* h, std::vector<Value*> args);
  Value* llvm_floor(Intrinsics intr, HInvoke* h, std::vector<Value*> args);
  Value* llvm_exp(Intrinsics intr, HInvoke* h, std::vector<Value*> args);
  Value* llvm_ceil(Intrinsics intr, HInvoke* h, std::vector<Value*> args);
  Value* llvm_cos(Intrinsics intr, HInvoke* h, std::vector<Value*> args);
  Value* llvm_log(Intrinsics intr, HInvoke* h, std::vector<Value*> args);

  Function* C_MathRoundFloat(); // CLR_LLVM won't be needed
  Function* C_MathRoundDouble(); // CLR_LLVM won't be needed
  Function* MathCeil();
  Function* MathFloor();
  Function* MathSin();
  Function* MathPow();
  Function* MathTan();
  Function* isinf(bool is64);
  Function* MathATan2();
  Function* MathAsin();

  Value* CallMathCeil(Intrinsics intrinsic, HInvoke* invoke,
                              std::vector<Value*> callee_args);
  Value* CallSystemArrayCopyChar(
      HGraphToLLVM* HL, Intrinsics intrinsic, HInvoke* invoke,
      std::vector<Value*> callee_args);
  Value* CallMathFloor(Intrinsics intrinsic, HInvoke* invoke,
                               std::vector<Value*> callee_args);
  Value* CallMathSin(ExtraIntrinsics intrinsic, HInvoke* invoke,
                             std::vector<Value*> callee_args);
  Value* CallMathPow(ExtraIntrinsics intrinsic, HInvoke* invoke,
                             std::vector<Value*> callee_args);
  Value* CallCMathTan(std::vector<Value*> args);
  Value* CallCMathAsin(std::vector<Value*> args);
  Value* CallMathSqrt(Intrinsics intrinsic, HInvoke* invoke,
      std::vector<Value*> callee_args);
  Function* MathSqrtDouble();
};

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_INTRINSIC_HELPER_H_
