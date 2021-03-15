/**
 * TODO this is NOT implemented.
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
#include "function_helper.h"
#include "fh_instanceOf-inl.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Metadata.h>
#include "art_method-inl.h"
#include "art_method.h"
#include "asm_arm64.h"
#include "asm_arm_thumb.h"
#include "base/logging.h"
#include "class_status.h"
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"
#include "index_bss_mapping.h"
#include "ir_builder.h"
#include "llvm_compiler.h"
#include "llvm_info.h"
#include "llvm_utils.h"
#include "mcr_cc/analyser.h"
#include "mcr_cc/invoke_histogram.h"
#include "mcr_cc/linker_interface.h"
#include "mcr_cc/os_comp.h"
#include "mcr_rt/art_impl_arch_arm-inl.h"
#include "mcr_rt/invoke_info.h"
#include "oat_file.h"
#include "optimizing/code_generator.h"
#include "stack.h"
#include <sstream>

#include "llvm_macros_IRBc.h"


using namespace ::llvm;

namespace art {
namespace LLVM {

Function* FunctionHelper::StringCompareTo(
    HGraphToLLVM* HL, IRBuilder* IRB, HInvoke* h) {
  D3LOG(INFO) << __func__;

  std::string name = "StringCompareTo";
  if (mirror::kUseStringCompression) {
    name+="Compression";
  }

  if (string_compare_to_[name] != nullptr) return string_compare_to_[name];
  BasicBlock* pinsert_point = IRB->GetInsertBlock();

  VERIFY_LLVMD(name);
  std::vector<Type*> argsTy {2, IRB->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(IRB->getJIntTy(), argsTy, false);
  Function* func = Function::Create(
      ty, Function::LinkOnceODRLinkage, name, IRB->getModule());
  string_compare_to_[name] = func;
  func->setDSOLocal(true);
  AddAttributesCommon(func);

  // process arguments
  Function::arg_iterator arg_iter(func->arg_begin());
  Value* str1= &*arg_iter++;
  Value* str2= &*arg_iter++;
  str1->setName("str1");
  str2->setName("str2");

  LLVMContext& ctx = IRB->getContext();
  BasicBlock* bbEntry = BasicBlock::Create(ctx, "entry", func);
  IRB->SetInsertPoint(bbEntry);

  // MacroAssembler* masm = GetVIXLAssembler();
  // LocationSummary* locations = invoke->GetLocations();
  // Register str = InputRegisterAt(invoke, 0);
  // Register arg = InputRegisterAt(invoke, 1);
  // Register out = OutputRegister(invoke);

  // Register temp0 = WRegisterFrom(locations->GetTemp(0));
  // Register temp1 = WRegisterFrom(locations->GetTemp(1));
  // Register temp2 = WRegisterFrom(locations->GetTemp(2));
  // Register temp3;
  if (mirror::kUseStringCompression) {
    // temp3 = WRegisterFrom(locations->GetTemp(3));
  }

  // vixl::aarch64::Label loop;
  // vixl::aarch64::Label find_char_diff;
  // vixl::aarch64::Label end;
  // vixl::aarch64::Label different_compression;

  // Get offsets of count and value fields within a string object.
  const int32_t count_offset = mirror::String::CountOffset().Int32Value();
  const int32_t value_offset = mirror::String::ValueOffset().Int32Value();


  // Take slow path and throw if input can be and is null.
  const bool can_slow_path = h->InputAt(1)->CanBeNull();
  if (can_slow_path) {
    DLOG(FATAL) << __func___ << ": CanThrow";
  }

  // TODO very simple check: if same value then ret zero!
  // Reference equality check, return 0 if same reference.
  __ Subs(out, str, arg);
  __ B(&end, eq);

  // TODO Loads
  if (mirror::kUseStringCompression) {
    // Load `count` fields of this and argument strings.
    __ Ldr(temp3, HeapOperand(str, count_offset));
    __ Ldr(temp2, HeapOperand(arg, count_offset));
    // Clean out compression flag from lengths.
    __ Lsr(temp0, temp3, 1u);
    __ Lsr(temp1, temp2, 1u);
  } else {
    // Load lengths of this and argument strings.
    __ Ldr(temp0, HeapOperand(str, count_offset));
    __ Ldr(temp1, HeapOperand(arg, count_offset));
  }
  // out = length diff.
  __ Subs(out, temp0, temp1);
  // temp0 = min(len(str), len(arg)).
  __ Csel(temp0, temp1, temp0, ge);
  // Shorter string is empty?
  __ Cbz(temp0, &end);

  if (mirror::kUseStringCompression) {
    // Check if both strings using same compression style to use this comparison loop.
    __ Eor(temp2, temp2, Operand(temp3));
    // Interleave with compression flag extraction which is needed for both paths
    // and also set flags which is needed only for the different compressions path.
    __ Ands(temp3.W(), temp3.W(), Operand(1));
    __ Tbnz(temp2, 0, &different_compression);  // Does not use flags.
  }
  // Store offset of string value in preparation for comparison loop.
  __ Mov(temp1, value_offset);
  if (mirror::kUseStringCompression) {
    // For string compression, calculate the number of bytes to compare (not chars).
    // This could in theory exceed INT32_MAX, so treat temp0 as unsigned.
    __ Lsl(temp0, temp0, temp3);
  }

  UseScratchRegisterScope scratch_scope(masm);
  Register temp4 = scratch_scope.AcquireX();

  // Assertions that must hold in order to compare strings 8 bytes at a time.
  DCHECK_ALIGNED(value_offset, 8);
  static_assert(IsAligned<8>(kObjectAlignment), "String of odd length is not zero padded");

  const size_t char_size = DataType::Size(DataType::Type::kUint16);
  DCHECK_EQ(char_size, 2u);

  // Promote temp2 to an X reg, ready for LDR.
  temp2 = temp2.X();

  // Loop to compare 4x16-bit characters at a time (ok because of string data alignment).
  __ Bind(&loop);
  __ Ldr(temp4, MemOperand(str.X(), temp1.X()));
  __ Ldr(temp2, MemOperand(arg.X(), temp1.X()));
  __ Cmp(temp4, temp2);
  __ B(ne, &find_char_diff);
  __ Add(temp1, temp1, char_size * 4);
  // With string compression, we have compared 8 bytes, otherwise 4 chars.
  __ Subs(temp0, temp0, (mirror::kUseStringCompression) ? 8 : 4);
  __ B(&loop, hi);
  __ B(&end);

  // Promote temp1 to an X reg, ready for EOR.
  temp1 = temp1.X();

  // Find the single character difference.
  __ Bind(&find_char_diff);
  // Get the bit position of the first character that differs.
  __ Eor(temp1, temp2, temp4);
  __ Rbit(temp1, temp1);
  __ Clz(temp1, temp1);

  // If the number of chars remaining <= the index where the difference occurs (0-3), then
  // the difference occurs outside the remaining string data, so just return length diff (out).
  // Unlike ARM, we're doing the comparison in one go here, without the subtraction at the
  // find_char_diff_2nd_cmp path, so it doesn't matter whether the comparison is signed or
  // unsigned when string compression is disabled.
  // When it's enabled, the comparison must be unsigned.
  __ Cmp(temp0, Operand(temp1.W(), LSR, (mirror::kUseStringCompression) ? 3 : 4));
  __ B(ls, &end);

  // Extract the characters and calculate the difference.
  if (mirror:: kUseStringCompression) {
    __ Bic(temp1, temp1, 0x7);
    __ Bic(temp1, temp1, Operand(temp3.X(), LSL, 3u));
  } else {
    __ Bic(temp1, temp1, 0xf);
  }
  __ Lsr(temp2, temp2, temp1);
  __ Lsr(temp4, temp4, temp1);
  if (mirror::kUseStringCompression) {
    // Prioritize the case of compressed strings and calculate such result first.
    __ Uxtb(temp1, temp4);
    __ Sub(out, temp1.W(), Operand(temp2.W(), UXTB));
    __ Tbz(temp3, 0u, &end);  // If actually compressed, we're done.
  }
  __ Uxth(temp4, temp4);
  __ Sub(out, temp4.W(), Operand(temp2.W(), UXTH));

  if (mirror::kUseStringCompression) {
    __ B(&end);
    __ Bind(&different_compression);

    // Comparison for different compression style.
    const size_t c_char_size = DataType::Size(DataType::Type::kInt8);
    DCHECK_EQ(c_char_size, 1u);
    temp1 = temp1.W();
    temp2 = temp2.W();
    temp4 = temp4.W();

    // `temp1` will hold the compressed data pointer, `temp2` the uncompressed data pointer.
    // Note that flags have been set by the `str` compression flag extraction to `temp3`
    // before branching to the `different_compression` label.
    __ Csel(temp1, str, arg, eq);   // Pointer to the compressed string.
    __ Csel(temp2, str, arg, ne);   // Pointer to the uncompressed string.

    // We want to free up the temp3, currently holding `str` compression flag, for comparison.
    // So, we move it to the bottom bit of the iteration count `temp0` which we then need to treat
    // as unsigned. Start by freeing the bit with a LSL and continue further down by a SUB which
    // will allow `subs temp0, #2; bhi different_compression_loop` to serve as the loop condition.
    __ Lsl(temp0, temp0, 1u);

    // Adjust temp1 and temp2 from string pointers to data pointers.
    __ Add(temp1, temp1, Operand(value_offset));
    __ Add(temp2, temp2, Operand(value_offset));

    // Complete the move of the compression flag.
    __ Sub(temp0, temp0, Operand(temp3));

    vixl::aarch64::Label different_compression_loop;
    vixl::aarch64::Label different_compression_diff;

    __ Bind(&different_compression_loop);
    __ Ldrb(temp4, MemOperand(temp1.X(), c_char_size, PostIndex));
    __ Ldrh(temp3, MemOperand(temp2.X(), char_size, PostIndex));
    __ Subs(temp4, temp4, Operand(temp3));
    __ B(&different_compression_diff, ne);
    __ Subs(temp0, temp0, 2);
    __ B(&different_compression_loop, hi);
    __ B(&end);

    // Calculate the difference.
    __ Bind(&different_compression_diff);
    __ Tst(temp0, Operand(1));
    static_assert(static_cast<uint32_t>(mirror::StringCompressionFlag::kCompressed) == 0u,
                  "Expecting 0=compressed, 1=uncompressed");
    __ Cneg(out, temp4, ne);
  }

  __ Bind(&end);

  if (can_slow_path) {
    __ Bind(slow_path->GetExitLabel());
  }

    // General case.
    Value* length = nullptr;
    uint32_t count_offset = mirror::String::CountOffset().Uint32Value();
    // length = temps.AcquireW();
    length = HL->LoadWord<false>(argArrayObj, count_offset);
    // __ Ldr(length, HeapOperand(obj, count_offset));
    Arm64::MaybeRecordImplicitNullCheck(h);
    // IRB->AndroidLogPrintInt(INFO,
    //     "ArrayGet: CompressedCharAt: length", length);

      // __ Tbnz(length.W(), 0, &uncompressed_load);
      // Test bit and branch if nonzero
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
        // vixl::aarch64::Label uncompressed_load, done;
        // __ Ldrb(Register(OutputCPURegister(h)),
        //         HeapOperand(obj, offset + Int64FromLocation(index)));
        // __ B(&done);
        nOffset += HL->Int64FromLocation(index);
        Value* byte = HL->LoadByte<false>(argArrayObj, nOffset);
        byte = IRB->UpcastInt(byte, array_type);
        IRB->CreateRet(byte);
      }

      // Uncompressed laod
      IRB->SetInsertPoint(bbUncompressedLoad);
      {
        uint32_t cOffset = offset;
        // __ Bind(&uncompressed_load);
        // __ Ldrh(Register(OutputCPURegister(h)),
        // HeapOperand(obj, offset + (Int64FromLocation(index) << 1)));
        // __ Bind(&done);
        cOffset += HL->Int64FromLocation(index) << 1;
        Value* halfword = HL->LoadHalfWord<false>(argArrayObj, cOffset);
        halfword = IRB->UpcastInt(halfword, array_type);
        IRB->CreateRet(halfword);
      }
    } else { // dynamic
      CHECK(argIndex!=nullptr);
      // The below add is taken inot account by GetDynamicOffset
      // __ Add(temp, obj, offset);
      // tbnz logic moved outside
      // Normal load
      IRB->SetInsertPoint(bbNormalLoad);
      {
        // __ Ldrb(Register(OutputCPURegister(h)),
        //         HeapOperand(temp, XRegisterFrom(index), LSL, 0));
        // __ B(&done);
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
        // __ Bind(&done);
        Value* dynOffset = HL->GetDynamicOffset(argIndex, 1, offset);
        Value* halfword = HL->LoadHalfWord<false>(argArrayObj, dynOffset);
        halfword = IRB->UpcastInt(halfword, array_type);
        IRB->CreateRet(halfword);
      }
    }

  IRB->SetInsertPoint(pinsert_point);
  return func;
}

void IntrinsicCodeGeneratorARM64::VisitStringCompareTo(HInvoke* invoke) {
  
}



#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
