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

#include "llvm_macros_irb_.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

/**
   * @brief It is called for all the HGraph nodes that we haven't
   *        implement in the LLVM backend. It prints a message
   *        and exits
   */
void HGraphToLLVM::VisitInstruction(HInstruction* h) {
  DIE_UNIMPLEMENTED_;
}

void HGraphToLLVM::VisitDeoptimize(HDeoptimize* h) {
  DLOG(ERROR) << "HDeoptimize: Ignoring: "
              << prt_->GetInstruction((HInstruction*)(h))
              << "\nMethod: " << GetPrettyMethod();
}

BasicBlock* HGraphToLLVM::GenerateBasicBlock(HBasicBlock* hblock) {
  D3LOG(INFO) << "GenerateBasicBlock: " << GetBasicBlockName(hblock)
              << " (hf: " << GetPrettyMethod() << ")";
  BasicBlock* lblock =
      BasicBlock::Create(
          *ctx_, GetBasicBlockName(hblock), GetInnerFunction());
  addBasicBlock(hblock, lblock);
  return lblock;
}

void HGraphToLLVM::VisitBasicBlock(HBasicBlock* hblock) {
  D3LOG(WARNING) << "BasicBlock: " << std::to_string(hblock->GetBlockId());
  // visit block's instructions
  cur_lblock_ = getBasicBlock(hblock);
  irb_->SetInsertPoint(cur_lblock_);
  if (McrDebug::VerifyBasicBlock(GetPrettyMethod())) {
    PrintBasicBlockDebug(hblock);
  }
  D3LOG(INFO) << "VisitBasicBlock: " << GetBasicBlockName(hblock);
  HGraphVisitor::VisitBasicBlock(hblock);
}

void HGraphToLLVM::VisitCurrentMethod(HCurrentMethod* h) {
  VERIFIED_;
  addValue(h, GetLoadedArtMethod());
}

/**
 * @brief Special cases:
 *
 * NOTE:
 * If h is integral, and smaller than i32, it is still handled as i32.
 * In most cases where it is used a relevant casting HInstruction
 * (maybe TypeConversion) follows, which truncates.
 *
 * But for some vector operations it does not, so we compare the llvm type
 * from the parameter (which should be i32), and truc manually (hgraph_vector)
 *
 */
void HGraphToLLVM::VisitParameterValue(HParameterValue* h) {
  D3LOG(INFO) << "VisitParameterValue: " << GetTwine(h);
  Argument* src = getArgument(h);
  Type* srcTy = src->getType();
  Type* resTy = irb_->getType(h);
  DataType::Type type = h->GetType();
  bool is_fp = DataType::IsFloatingPointType(type);

  Value* casted_src = src;
  if (srcTy != resTy) {
    D3LOG(INFO) << "Casting : " << type
                << " to: " << DataType::Kind(type);
    if (is_fp) {
      casted_src = irb_->CreateFPCast(src, resTy);
    } else {
      bool is_signed = IRBuilder::IsSigned(type);
      casted_src = irb_->CreateIntCast(src, resTy, is_signed);
    }
  }
  addValue(h, casted_src);
}

// Disabled in HGraph when generating IR for LLVM backend.
// LLVM will apply this optimization if needed.
void HGraphToLLVM::VisitDataProcWithShifterOp(
    HDataProcWithShifterOp* h) {
  DIE_TODO
      << "\nCurrent implementation is incomplete."
      << "\nBut we never let LLVM generate this optimization "
         "as it is done anyway by its assembler.";

  DataType::Type type = h->GetType();
  HInstruction::InstructionKind kind = h->GetInstrKind();
  CHECK(type == DataType::Type::kInt32 || type == DataType::Type::kInt64);

  HInstruction* hleft = h->InputAt(0);
  HInstruction* hright = h->InputAt(1);

  Value* lhs = nullptr;
  Value* rhs = getValue(hright);
  Value* res = nullptr;

  if (kind != HInstruction::kNeg) {
    lhs = getValue(hleft);
    if (type == DataType::Type::kInt64) {
      if (hleft->GetType() != DataType::Type::kInt64) {
        Value* casted = irb_->CreateIntCast(
            lhs, irb_->getJLongTy(),
            IRBuilder::IsSigned(hleft->GetType()));
        lhs = casted;
      }
      if (hright->GetType() != DataType::Type::kInt64) {
        Value* casted = irb_->CreateIntCast(
            rhs, irb_->getJLongTy(),
            IRBuilder::IsSigned(hright->GetType()));
        rhs = casted;
      }
    }
  }

  // If this `HDataProcWithShifterOp` was created by merging a type conversion as the
  // shifter operand operation, the IR generating `right_reg` (input to the type
  // conversion) can have a different type from the current h's type,
  // so we manually indicate the type.

  // Register right_reg = RegisterFrom(h->GetLocations()->InAt(1), type);
  // Operand right_operand(0);
  HDataProcWithShifterOp::OpKind op_kind = h->GetOpKind();
  if (HDataProcWithShifterOp::IsExtensionOp(op_kind)) {
    // right_operand = Operand(right_reg, helpers::ExtendFromOpKind(op_kind));
  } else {
    // Probably here I should have shifter..
    //   right_operand = Operand(right_reg,
    //       helpers::ShiftFromOpKind(op_kind),
    //       h->GetShiftAmount());
  }

  // Logical binary operations do not support extension operations in the
  // operand. Note that VIXL would still manage if it was passed by generating
  // the extension as a separate h.
  // `HNeg` also does not support extension. See comments in `ShifterOperandSupportsExtension()`.

  // should always be false (checked above)
  bool is_fp = DataType::IsFloatingPointType(h->GetType());
  bool is_signed = IRBuilder::IsSigned(type);
  switch (kind) {
    case HInstruction::kAdd:
      res = irb_->mCreateAdd(is_fp, lhs, rhs);
      // __ Add(out, left, right_operand);
      break;
    case HInstruction::kAnd:
      res = irb_->CreateAnd(lhs, rhs);
      // __ And(out, left, right_operand);
      break;
    case HInstruction::kNeg:
      CHECK(h->InputAt(0)->AsConstant()->IsArithmeticZero());
      res = irb_->mCreateNeg(is_fp, is_signed, rhs);
      // __ Neg(out, right_operand);
      break;
    case HInstruction::kOr:
      res = irb_->CreateOr(lhs, rhs);
      // __ Orr(out, left, right_operand);
      break;
    case HInstruction::kSub:
      res = irb_->mCreateSub(is_fp, lhs, rhs);
      // __ Sub(out, left, right_operand);
      break;
    case HInstruction::kXor:
      res = irb_->CreateXor(lhs, rhs);
      // __ Eor(out, left, right_operand);
      break;
    default:
      LOG(FATAL) << __func__ << "Unexpected operation kind: " << kind;
      UNREACHABLE();
  }

  CHECK(res != nullptr);
  addValue(h, res);
}

// Disabled in HGraph when generating IR for LLVM backend.
// LLVM will apply this optimization if needed.
void HGraphToLLVM::VisitMultiplyAccumulate(HMultiplyAccumulate* h) {
  // INFO this is probably NOT needed.
  // LLVM's ARM assembler will create a madd, msub, mneg where is needed!
  HInstruction* accum =
      h->InputAt(HMultiplyAccumulate::kInputAccumulatorIndex);

  DLOG(FATAL) << __func__;

  bool is_fp = DataType::IsFloatingPointType(accum->GetType());
  if (h->GetOpKind() == HInstruction::kSub &&
      accum->IsConstant() &&
      accum->AsConstant()->IsArithmeticZero()) {
    D5LOG(INFO) << "Don't allocate register for Mneg instruction.";
  } else {
    // locations->SetInAt(HMultiplyAccumulate::kInputAccumulatorIndex,
    // Location::RequiresRegister());
  }

  HInstruction* hleft = h->InputAt(HMultiplyAccumulate::kInputMulLeftIndex);
  HInstruction* hright = h->InputAt(HMultiplyAccumulate::kInputMulRightIndex);
  Value* laccum = getValue(accum);
  Value* lhs = getValue(hleft);
  Value* rhs = getValue(hright);
  Value* res = nullptr;

  Value* mul = irb_->mCreateMul(is_fp, lhs, rhs);
  if (h->GetOpKind() == HInstruction::kAdd) {
    // add(mul)
    // INFO LLVM: fmuladd if float. And this optimization is only applied for integers.
    // Also LLVM will probably do the float opt anyway..
    res = irb_->mCreateAdd(is_fp, mul, laccum);
    // __ Madd(res, mul_left, mul_right, accumulator);
  } else {
    CHECK(h->GetOpKind() == HInstruction::kSub);
    if (accum->IsConstant() && accum->AsConstant()->IsArithmeticZero()) {
      // neg(mul)
      res = irb_->mCreateNeg(is_fp, laccum, mul);
      // __ Mneg(res, mul_left, mul_right);
    } else {
      res = irb_->mCreateSub(is_fp, laccum, mul);
      // __ Msub(res, mul_left, mul_right, accumulator);
    }
  }
  addValue(h, res);
}

void HGraphToLLVM::VisitMonitorOperation(HMonitorOperation* instruction) {
  Value* lobj = getValue(instruction->InputAt(0));
  ArtCallMonitorOperation(lobj, instruction->IsEnter());
  MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
}

void HGraphToLLVM::VisitPackedSwitch(HPackedSwitch* switch_instr) {
  int32_t lower_bound = switch_instr->GetStartValue();
  uint32_t num_entries = switch_instr->GetNumEntries();
  Value* value = getValue(switch_instr->InputAt(0));
  HBasicBlock* switch_block = switch_instr->GetBlock();
  HBasicBlock* default_block = switch_instr->GetDefaultBlock();
  BasicBlock* ldefault_block = getBasicBlock(default_block);
  D3LOG(INFO) << __func__ << ": LowBound:" << lower_bound
              << " NumEntries:" << num_entries;

  SwitchInst* Switch = SwitchInst::Create(
      value, ldefault_block, num_entries, irb_->GetInsertBlock());

  // Figure out the correct compare values and jump conditions.
  // Handle the first compare/branch as a special case because it might
  // jump to the default case.
  // DCHECK_GT(num_entries, 2u);
  // Condition first_condition;
  // uint32_t index;
  int32_t switch_idx = lower_bound;
  const ArenaVector<HBasicBlock*>& successors = switch_block->GetSuccessors();
  for (HBasicBlock* successor : successors) {
    D5LOG(INFO) << __func___ << "Switch: val:block "
                << switch_idx << ":" << GetBasicBlockName(successor);
    ConstantInt *case_val =
      ConstantInt::get(irb_->getJIntTy(), switch_idx++);
    BasicBlock* lsuccessor = getBasicBlock(successor);
    Switch->addCase(case_val, lsuccessor);
  }
}

void HGraphToLLVM::VisitGoto(HGoto* got) {
#ifdef SUSPEND_CHECK_USE_METHOD
  HandleGotoMethod(got, got->GetSuccessor());
#else
  HandleGoto(got, got->GetSuccessor());
#endif
}

/**
 * @brief We follow the BB and PHI generation of HGraph for all cases,
 * by wrapping to inlinable methods any operations that require branches.
 * This was also the case with SuspendCheck (SC).
 * However, this was slower so we are adding extra BBs for that.
 *
 * @param lfrom   LLVM BB for the original HGraph BB, that is doing the SC
 *                Needed for adding to phi_sc_additions_
 * @param lfromSC LLVM BB that will goto htarget. if lfrom has an input 
 *                to a phi, then we make sure that lfromSC replicates that
 *                input
 * @param htarget the target that hfrom originally br to,
 *                 and now lfromSC will br to
 */
void HGraphToLLVM::GenerateGotoForSuspendCheck(
    BasicBlock* lfrom, BasicBlock *lfromSC, HBasicBlock* htarget) {
  BasicBlock* ltarget = getBasicBlock(htarget);
  LinkBasicBlocks(lfromSC, ltarget);

  D1LOG(INFO) << "PHI extra input: " << Pretty(lfrom)
    << " has input from: " <<  Pretty(lfromSC) ;
  phi_sc_additions_.insert(
        std::pair<BasicBlock*, BasicBlock*>(lfrom, lfromSC));
}

void HGraphToLLVM::GenerateGoto(HBasicBlock* htarget) {
  BasicBlock* ltarget = getBasicBlock(htarget);
  LinkBasicBlocks(cur_lblock_, ltarget);
}

void HGraphToLLVM::VisitTryBoundary(HTryBoundary* try_boundary) {
  D3LOG(INFO) << __func___ << prt_->GetInstruction(try_boundary);

  HBasicBlock* successor = try_boundary->GetNormalFlowSuccessor();
  if (!successor->IsExitBlock()) {
    GenerateGoto(successor);
  } else {
    // we link also the exit block, as LLVM IR verification will complain
    GenerateGoto(successor);
  }
}

void HGraphToLLVM::VisitPhi(HPhi* h ATTRIBUTE_UNUSED) {
  // Some inputs to phi might not have been defined yet,
  // so we do a separate pass with Populate PHIs once all instructions
  // were emited.
}

void HGraphToLLVM::PopulatePhi(HPhi* hphi) {
  D3LOG(INFO) << "PopulatePhi: " << prt_->GetInstruction(hphi);
  PHINode* lphi = getPhi(hphi);

  D3LOG(INFO) << "Generating phi inputs";
  for (size_t i = 0; i < hphi->InputCount(); i++) {
    D5LOG(INFO) << "Generating phi input: " << i;
    HInstruction* input = hphi->InputAt(i);
    HBasicBlock* pred = hphi->GetBlock()->GetPredecessors()[i];
    BasicBlock* lpred = getBasicBlock(pred);

    BasicBlock* lblockSC = nullptr;
    if(phi_sc_additions_.find(lpred) != phi_sc_additions_.end()) {
      lblockSC=phi_sc_additions_[lpred];
    }

    // extra inputs to PHIs from SuspendCheck blocks
    // its: original BB, that has a SuspendCheck BB
    // e.g., block5 has SuspendCheck_block5 (which is only an LLVM block)

    Value* inval = getValue(input);
    lphi->addIncoming(inval, lpred);
    if(lblockSC) {
      lphi->addIncoming(inval, lblockSC);
    }
  }
}

void HGraphToLLVM::VisitIf(HIf* h) {
  D3LOG(INFO) << __func__ << ":" << GetTwine(h);
  HInstruction* hcond = h->InputAt(0);
  HBasicBlock* htrue_block = h->GetBlock()->GetSuccessors()[0];
  HBasicBlock* hfalse_block = h->GetBlock()->GetSuccessors()[1];

  Value* lhs = getValue(hcond);
  Value* rhs_false = irb_->getJBoolean(false);
  rhs_false = irb_->CreateBitCast(rhs_false, irb_->getType(hcond->GetType()));
  Value* condition = irb_->CreateICmpNE(lhs, rhs_false);
  BasicBlock* ltrue_block = getBasicBlock(htrue_block);
  BasicBlock* lfalse_block = getBasicBlock(hfalse_block);
  irb_->CreateCondBr(condition, ltrue_block, lfalse_block);
}

void HGraphToLLVM::VisitSelect(HSelect* h) {
  D4LOG(INFO) << __func__ << ": " << GetTwine(h);
  HInstruction* hcondition = h->GetCondition();
  HInstruction* htrue_value = h->GetTrueValue();
  HInstruction* hfalse_value = h->GetFalseValue();

  Value* lcond = getValue(hcondition);

  bool is_signed = IRBuilder::IsSigned(hcondition->GetType());
  lcond = irb_->CreateIntCast(lcond, irb_->getCBoolTy(), is_signed);
  Value* ltruev = getValue(htrue_value);
  Value* lfalsev = getValue(hfalse_value);

  // CHECK_LLVM does it handle automatically float?
  const char* reason = SelectInst::areInvalidOperands(lcond, ltruev, lfalsev);
  CHECK(reason == nullptr) << __func__ << "SelectInst: " << reason;

  Value* res = irb_->CreateSelect(lcond, ltruev, lfalsev);
  addValue(h, res);
}

void HGraphToLLVM::VisitExit(HExit* h) {
  D3LOG(INFO) << "VisitExit";
  std::vector<Value*> prt_args;
#ifdef CRDEBUG3
  Value* fmt =
      irb_->mCreateGlobalStringPtr("EXITING [exit_block]\n");
  prt_args.push_back(irb_->AndroidLogSeverity(ERROR));
  prt_args.push_back(fmt);
  irb_->CreateCall(fh_->AndroidLog(), prt_args);
#endif
  fh_->CallExit(irb_, 1);
}

void HGraphToLLVM::VisitReturnVoid(HReturnVoid* h) {
  D4LOG(INFO) << __func__;
  irb_->CreateRetVoid();
}

void HGraphToLLVM::VisitReturn(HReturn* h) {
  VERIFY_LLVMD4(__func__);
  // REVIEW in nodes.h it states that it should go to exit_block.
  // However, it also state's that exit_block's only instruction is
  // HExit, which essentially kills the app. A bit confused.
  // Our approach given our calling convention works well though.
  HInstruction* hret = h->InputAt(0);
  Value* ret_value = getValue(h->InputAt(0));

  // We are using remapped types for all variables, and exact types
  // for the method's finger print. In case of an integral value,
  // smaller than 32 bits a casting (truncate actually) will be performed
  Type* lreturn_type = irb_->getType(hret);
  Type* methods_return_type = inner_func_->getReturnType();

  Value* casted_ret_value = ret_value;
  if (lreturn_type != methods_return_type) {
    DataType::Type hreturn_type = hret->GetType();
    if (DataType::IsFloatingPointType(hreturn_type)) {
      casted_ret_value = irb_->CreateFPCast(ret_value, methods_return_type);
    } else {
      bool is_signed = IRBuilder::IsSigned(hreturn_type);
      casted_ret_value = irb_->CreateIntCast(ret_value, methods_return_type, is_signed);
    }
  }

#ifdef CRDEBUG4
  irb_->AndroidLogPrintValue(INFO, "VisitReturn",
                             casted_ret_value, hret->GetType());
#endif
  irb_->CreateRet(casted_ret_value);
}

void HGraphToLLVM::VisitFloatConstant(HFloatConstant* h) {
  D4LOG(INFO) << __func__ << ": " << GetTwine(h);
  Constant* const_val = irb_->getJFloat(h->GetValue());
  GenerateConstant(h, const_val);
}
void HGraphToLLVM::VisitDoubleConstant(HDoubleConstant* h) {
  D4LOG(INFO) << __func__ << ": " << GetTwine(h);
  Constant* const_val = irb_->getJDouble(h->GetValue());
  GenerateConstant(h, const_val);
}

void HGraphToLLVM::VisitLongConstant(HLongConstant* h) {
  D4LOG(INFO) << __func__ << ": " << GetTwine(h);
  Constant* const_val = irb_->getJLong(h->GetValue());
  GenerateConstant(h, const_val);
}

void HGraphToLLVM::VisitIntConstant(HIntConstant* h) {
  D4LOG(INFO) << __func__ << ": " << GetTwine(h);
  Constant* const_val = irb_->getJInt(h->GetValue());
  GenerateConstant(h, const_val);
}

void HGraphToLLVM::VisitNullConstant(HNullConstant* h) {
  D4LOG(INFO) << __func__ << ": " << GetTwine(h);
  CHECK(h->GetType() == DataType::Type::kReference)
      << "Null constant type must be an object";
  GenerateConstant(h, irb_->getJNull());
}

void HGraphToLLVM::VisitBooleanNot(HBooleanNot* h) {
  D4LOG(INFO) << __func___ << prt_->GetInstruction(h);
  // REVIEW this could be optimized further:
  // create an i32, with value 1, and xor it w/ invalue
  // This assumes that invalue has just 0 and 1 values, which holds.
  Value* invalue = getValue(h->InputAt(0));
  Value* trunc_invalue = irb_->CreateTrunc(invalue, irb_->getInt1Ty());
  Value* invert = irb_->CreateNot(trunc_invalue);

  Value* res = irb_->CreateZExt(invert, irb_->getType(h));
  addValue(h, res);
}

void HGraphToLLVM::VisitNeg(HNeg* h) {
  D3LOG(INFO) << __func___ << prt_->GetInstruction(h);
  HInstruction* hinput = h->InputAt(0);
  Value* lvalue = getValue(hinput);
  DataType::Type type = hinput->GetType();
  bool is_fp = DataType::IsFloatingPointType(type);
  bool is_signed = IRBuilder::IsSigned(type);

  D4LOG(INFO) << __func___ << GetTwine(h)
              << " " << (is_signed ? "signed" : "UNSIGNED")
              << " llvm type: " << Pretty(lvalue->getType());

  Value* res = irb_->mCreateNeg(is_fp, is_signed, lvalue);

  bool is_char = type == DataType::Type::kUint16;
  if (is_char) {
    // special handling for unsigned neg
    // e.g. char x ='a'; x=-x;
    res = irb_->CreateTrunc(res, irb_->getTypeExact(type));
    // Type* ltype = lvalue->getType();
    // CreateTrunc getJCharTy
    // CreateNeg
    // CreateZExt
  }

  addValue(h, res);
}

void HGraphToLLVM::VisitAbs(HAbs* h) {
  D3LOG(INFO) << __func___ << prt_->GetInstruction(h);
  HInstruction* hinput = h->InputAt(0);
  DataType::Type type = hinput->GetType();
  bool is_fp = DataType::IsFloatingPointType(type);
  CHECK(IRBuilder::IsSigned(type));

  Value* res = nullptr;
  if (is_fp) {
    res = ih_->llvm_absf(this, h);
  } else {
    res = ih_->canonical_absi(this, h);
  }

  addValue(h, res);
}

void HGraphToLLVM::VisitNot(HNot* h) {
  D3LOG(INFO) << "VisitNot: " << prt_->GetInstruction(h);
  HInstruction* hinput = h->InputAt(0);
  DataType::Type type = hinput->GetType();
  CHECK(DataType::IsIntOrLongType(type)) << "Only int or long allowed";
  // if issues: not covering uint32, and uint64?

  Value* lvalue = getValue(hinput);
  Value* res = irb_->CreateNot(lvalue);
  addValue(h, res);
}

void HGraphToLLVM::VisitCompare(HCompare* h) {
  D3LOG(INFO) << "VisitCompare";
  Value* lhs = getValue(h->GetLeft());
  Value* rhs = getValue(h->GetRight());

  std::vector<Value*> args;
  args.push_back(lhs);
  args.push_back(rhs);

  CHECK(h->GetLeft()->GetType() == h->GetRight()->GetType())
      << "Compare: different types:\n"
      << "\nlhs: " << h->GetLeft()->GetType()
      << "\nrhs: " << h->GetRight()->GetType()
      << "\ninsruction type is always:  " << h->GetType()
      << " (vals: 0,1,-2\1)";

  // since lhs, rhs are quaranteed to be the same, we can use either
  Value* res = irb_->CreateCall(
      fh_->Compare(irb_, h->GetLeft()->GetType()), args);
  res->setName("cmpres");
  addValue(h, res);
}

/**
 * @brief INFO: This requires >=-O1 to get correct output on FP.
 *        Probably due to some optimization, that matches exactly
 *        what art is dong..
 *
 */
void HGraphToLLVM::VisitTypeConversion(HTypeConversion* h) {
  D4LOG(INFO) << __func__ << GetTwine(h);
  Value* linput = getValue(h->GetInput());
  DataType::Type input_type = h->GetInputType();
  DataType::Type result_type = h->GetResultType();
  Value* ext = nullptr;

  bool input_is_fp = DataType::IsFloatingPointType(input_type);
  bool result_is_fp = DataType::IsFloatingPointType(result_type);
  bool is_signed = irb_->IsSigned(result_type);
  Type* lresult_type = irb_->getType(result_type);

  // REVIEW
  if ((input_is_fp && result_is_fp) ||
      (!input_is_fp && !result_is_fp)) {
    if (result_is_fp) {
      ext = irb_->CreateFPCast(linput, lresult_type);
    } else {
      ext = irb_->CreateIntCast(linput, lresult_type, is_signed);
    }
  } else {
    if (input_is_fp) {  // fp to integral
      if (!is_signed) {
        ext = irb_->CreateFPToUI(linput, lresult_type);
      } else {
        ext = irb_->CreateFPToSI(linput, lresult_type);
      }
    } else {  // integral to fp
      ext = irb_->CreateSIToFP(linput, lresult_type);
    }
  }
  addValue(h, ext);
}

/**
 * @brief Initialized once on llvm entry block
 *
 * @param cls
 */
void HGraphToLLVM::VisitLoadClass(HLoadClass* cls) {
  Value* lart_method = GetLoadedArtMethod();
  HLoadClass::LoadKind load_kind = cls->GetLoadKind();

  Value* loaded_class = nullptr;
  const ReadBarrierOption
    read_barrier_option = cls->IsInBootImage()
    ? kWithoutReadBarrier
    : kCompilerReadBarrierOption;
  UNUSED(read_barrier_option);

  bool skip_init_check=false;
  bool generate_null_check = false;
  if (McrDebug::VerifyLoadClass()) {
    std::stringstream ss;
    ss << load_kind;
    irb_->AndroidLogPrint(WARNING, "LoadClass: " + ss.str());
  }

  D4LOG(INFO) << "LoadClass: "
    << GetTwine(cls) << ": "  << load_kind
    << ": type_idx: " << cls->GetTypeIndex().index_;
  switch (load_kind) {
    case HLoadClass::LoadKind::kReferrersClass: {  // VERIFIED
      CHECK(!cls->CanCallRuntime());
      CHECK(!cls->MustGenerateClinitCheck());
      // /* GcRoot<mirror::Class> */ out = current_method->declaring_class_
      loaded_class = GetArtMethodClass(lart_method);
    } break;
    case HLoadClass::LoadKind::kBootImageLinkTimePcRelative:
      {
        DIE_TODO << load_kind
          << ": Unimplemented: " << prt_->GetInstruction(cls) 
          << "\nMethod: " << GetPrettyMethod();

        const uint32_t boot_image_offset = GetBootImageOffset(cls);
        TODO_LLVM("kBootImageLinkTimePcRelative: 0x"
                     << std::hex << boot_image_offset
                     << std::dec << "/" << boot_image_offset);
        Value* boot_image_begin = LoadGlobalBootImageBegin();
        Value* addrInt = irb_->mCreateAdd(false, boot_image_begin,
            irb_->getJUnsignedInt(boot_image_offset));
        irb_->AndroidLogPrintHex(ERROR, "LoadClass: PcRelative TODO: VALUE:", addrInt);

        std::vector<Value*> args{ lart_method };
        loaded_class = irb_->CreateCall(
            fh_->LoadClass(this, irb_, cls, GetMethodIdx(), true), args);

        irb_->AndroidLogPrintHex(ERROR, "LoadClass: PcRelative: SlowPath:",
            loaded_class);
        VERIFY_LLVM("Are these the same? if so use image cache "
            "for  kBootImageLinkTimePcRelative");
      }
      break;
    case HLoadClass::LoadKind::kBootImageRelRo:
      {
      // Implementation:
      // adrp x0, #+0x129000 (addr 0x389000)
      // ldr w0, [x0, #312]
        uint32_t boot_image_offset = GetBootImageOffset(cls);
        // this is sent from runtime to LLVM entrypoint
        Value* boot_image_begin = LoadGlobalBootImageBegin();
        Value* addrInt = irb_->mCreateAdd(false, boot_image_begin,
            irb_->getJUnsignedInt(boot_image_offset));
        addrInt->setName("bootImgCacheAddr");

        loaded_class = irb_->CreateIntToPtr(addrInt, 
            irb_->getVoidPointerType());
        if(McrDebug::DebugLlvmCode4()) {
          irb_->AndroidLogPrintHex(WARNING, "LoadClass: RelRo",
              loaded_class);
          irb_->AndroidLogPrint(WARNING, "LoadClass RelRo [DONE]");
        }
      }
      break;
    case HLoadClass::LoadKind::kBssEntry:
      {
        // OPTIMIZE_LLVM: do it directly from bss offset
        std::vector<Value*> args{ lart_method };
        loaded_class = irb_->CreateCall(
            fh_->LoadClass(this, irb_, cls, GetMethodIdx(), true), args);
        // clinit is done in the above wrapper if needed.
        skip_init_check = true;
      }
      break;
    case HLoadClass::LoadKind::kRuntimeCall: {
      // TODO make this: LoadClassSlow
      // and for BssCheck make: LoadClassBss,
      // which will use LoadClassSlow if null
      // (so no need for generate_null_check)
      std::vector<Value*> args{ lart_method };
      loaded_class = irb_->CreateCall(
          fh_->LoadClass(this, irb_, cls, GetMethodIdx(), false), args);
    } break;
    case HLoadClass::LoadKind::kJitBootImageAddress:
      FALLTHROUGH_INTENDED;
    case HLoadClass::LoadKind::kJitTableAddress:
      FALLTHROUGH_INTENDED;
    case HLoadClass::LoadKind::kInvalid:
      LOG(FATAL) << "LoadClass: LoadKind: " << load_kind;
      UNREACHABLE();
  }

  bool do_clinit = cls->MustGenerateClinitCheck();
  if (generate_null_check || do_clinit) {
    DCHECK(cls->CanCallRuntime());
    if (generate_null_check) {
      TODO_LLVM("generate_null_check");
      // __ Cbz(out, slow_path->GetEntryLabel());
    }
    if (cls->MustGenerateClinitCheck()) {
      // irb_->AndroidLogPrint(ERROR, "XBSS class init check");
      if(!skip_init_check) {
        VERIFY_LLVM("calling class init check");
        CallClassInitializationCheck(loaded_class);
      }
    }
    MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
  }

  if (McrDebug::VerifyArtClass()) {
    VERIFY_LLVM("VerifyArtClass:");
    ArtCallVerifyArtClass(loaded_class);
  }

  addValue(cls, loaded_class);
}

// NO_THREAD_SAFETY_ANALYSIS as we manipulate handles whose internal object we know does not
// move.
void HGraphToLLVM::VisitLoadString(
    HLoadString* load) NO_THREAD_SAFETY_ANALYSIS {
  VERIFY_LLVMD3(load->GetLoadKind() << ": " << GetTwine(load));
  Value* lart_method = GetLoadedArtMethod();
  const dex::StringIndex string_index = load->GetStringIndex();
  const uint32_t string_idx = string_index.index_;
  HLoadString::LoadKind load_kind = load->GetLoadKind();
  VERIFY_LLVMD4(load_kind);

  Value* loaded_string = nullptr;
  switch (load->GetLoadKind()) {
    case HLoadString::LoadKind::kBootImageLinkTimePcRelative:
      {
        DIE_TODO 
          << load_kind
          << ": Unimplemented: " << prt_->GetInstruction(load) 
          << "\nMethod: " << GetPrettyMethod();
        // // Add ADRP with its PC-relative String patch.
        // const DexFile& dex_file = load->GetDexFile();
        // const dex::StringIndex string_index = load->GetStringIndex();
        // vixl::aarch64::Label* adrp_label = codegen_->
        // NewBootImageStringPatch(dex_file, string_index);
        // codegen_->EmitAdrpPlaceholder(adrp_label, out.X());
        // // Add ADD with its PC-relative String patch.
        // vixl::aarch64::Label* add_label =
        //     codegen_->NewBootImageStringPatch(
        //     dex_file, string_index, adrp_label);
        // codegen_->EmitAddPlaceholder(add_label, out.X(), out.X());
        // return;
      }
      break;
    case HLoadString::LoadKind::kBootImageRelRo:
      {
        VERIFY_LLVMD4(load_kind);
        uint32_t boot_image_offset = GetBootImageOffset(load);
        Value* boot_image_begin = LoadGlobalBootImageBegin();
        Value* addrInt = irb_->mCreateAdd(false, boot_image_begin,
            irb_->getJUnsignedInt(boot_image_offset));
        addrInt->setName("bootImgCacheAddr");
        loaded_string = irb_->CreateIntToPtr(addrInt, 
            irb_->getVoidPointerType());
        if(McrDebug::DebugLlvmCode4()) {
          irb_->AndroidLogPrintHex(WARNING, "LoadString:RelRo",
              loaded_string);
        }
      }
      break;
    case HLoadString::LoadKind::kBssEntry:
      {
        // // Add ADRP with its PC-relative String .bss entry patch.
        // const DexFile& dex_file = load->GetDexFile();
        // const dex::StringIndex string_index = load->GetStringIndex();
        // Register temp = XRegisterFrom(out_loc);
        // vixl::aarch64::Label* adrp_label =
        // codegen_->NewStringBssEntryPatch(dex_file, string_index);
        // codegen_->EmitAdrpPlaceholder(adrp_label, temp);
        // // Add LDR with its PC-relative String .bss entry patch.
        // vixl::aarch64::Label* ldr_label =
        //     codegen_->NewStringBssEntryPatch(dex_file, string_index, adrp_label);
        // // /* GcRoot<mirror::String> */ out = *(base_address + offset)  /* PC-relative */
        // codegen_->GenerateGcRootFieldLoad(
        //     load,
        //     out_loc,
        //     temp,
        //     /* offset placeholder */ 0u,
        //     ldr_label,
        //     kCompilerReadBarrierOption);
        // SlowPathCodeARM64* slow_path =
        //     new (codegen_->GetScopedAllocator()) LoadStringSlowPathARM64(load);
        // codegen_->AddSlowPath(slow_path);
        // __ Cbz(out.X(), slow_path->GetEntryLabel());
        // __ Bind(slow_path->GetExitLabel());
        // codegen_->MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
        // return;
        constexpr bool use_cache=true;
        std::vector<Value*> args{ lart_method };
        loaded_string = irb_->CreateCall(fh_->LoadString(
              this, irb_, load, string_idx, use_cache), args);
        if(McrDebug::DebugLlvmCode3()) {
          irb_->AndroidLogPrintHex(WARNING, "LoadString: BSS", loaded_string);
        }
      }
      break;
    case HLoadString::LoadKind::kRuntimeCall:
      {
        constexpr bool use_cache=false;
        std::vector<Value*> args{ lart_method };
        loaded_string = irb_->CreateCall(fh_->LoadString(
              this, irb_, load, string_idx, use_cache), args);
        if(McrDebug::DebugLlvmCode()) {  // CLR_LLVM
          irb_->AndroidLogPrintHex(WARNING, "LoadString:RT", loaded_string);
        }
      }
      break;
    case HLoadString::LoadKind::kJitBootImageAddress:
    case HLoadString::LoadKind::kJitTableAddress: 
      LOG(FATAL) << "LoadClass: LoadKind: " << load_kind;
      UNREACHABLE();
    default:
      break;
  }

  if(McrDebug::DebugLlvmCode4()) {
    // Print out the  resolved string
    VERIFY_LLVM("calling VerifyString");
    std::vector<Value*> args {loaded_string};
    irb_->CreateCall(fh_->__VerifyString(), args);
  }

  addValue(load, loaded_string);

  // TODO: Re-add the compiler code to do string dex cache lookup again.
  // __ Mov(calling_convention.GetRegisterAt(0).W(),
  // load->GetStringIndex().index_);
  // codegen_->InvokeRuntime(kQuickResolveString, load, load->GetDexPc());
  MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
}

void HGraphToLLVM::VisitInstanceFieldGet(HInstanceFieldGet* h) {
  HandleFieldGet(h);
}

void HGraphToLLVM::VisitStaticFieldGet(HStaticFieldGet* h) {
  HandleFieldGet(h);
}

void HGraphToLLVM::VisitInstanceFieldSet(HInstanceFieldSet* h) {
  HandleFieldSet(h);
}

void HGraphToLLVM::VisitStaticFieldSet(HStaticFieldSet* h) {
  HandleFieldSet(h);
}

void HGraphToLLVM::VisitArrayLength(HArrayLength* h) {
  VERIFIED_;
  MemberOffset offset(mirror::Array::LengthOffset().Uint32Value());
  HInstruction* obj = h->InputAt(0);
  // REVIEW arrays can't be volatile? (same goes for ArrayGet/ArraySet)
  bool is_volatile = false;

  // CHECK_LLVM generating new HInstruction here as a helper..
  // Create a FieldGet instruction, and expand it to get the length
  HInstanceFieldGet get_length(
      obj, nullptr, h->GetType(), offset, is_volatile,
      kUnknownFieldIndex, kUnknownClassDefIndex,
      GetGraph()->GetDexFile(), 0);
  get_length.SetId(h->GetId());

  VisitInstanceFieldGet(&get_length);
  replaceRegisterTmpInstruction(&get_length, h);

  // Mask out compression flag from String's array length.
  if (mirror::kUseStringCompression && h->IsStringLength()) {
    Value* length = getValue(h);
    if (McrDebug::DebugLlvmCode2()) {
      std::stringstream ss;
      ss << __func___ << "StringCompression: masked flag. Length";
      irb_->AndroidLogPrintInt(INFO, ss.str(), length);
    }
    // __ Lsr(out.W(), out.W(), 1u);
    Value* shr = irb_->CreateLShr(length, 1u, "shrStrLen");
    updateRegister(h, shr);
  }
}

void HGraphToLLVM::VisitArrayGet(HArrayGet* h) {
  DataType::Type array_type = h->GetType();
  HInstruction* array_obj = h->GetArray();
  HInstruction* index = GetArrayIndex(h);
  Value* larray_obj = getValue(array_obj);
  uint32_t offset = CodeGenerator::GetArrayDataOffset(h);
  constexpr bool is_volatile = false;
  const bool maybe_compressed_char_at =
    mirror::kUseStringCompression && h->IsStringCharAt();
  CHECK_NO_INTERMEDIATE_ACCESS(h);

  Value* lindex = nullptr;
  if (!index->IsConstant()) lindex = getValue(index);

  if (maybe_compressed_char_at) {
    VERIFY_LLVMD2("compressed_char_at");

    std::vector<Value*> args{ larray_obj };
    if (!index->IsConstant()) { args.push_back(lindex); }
    Value* loadedCompressedCharAt = irb_->CreateCall(
        fh_->ArrayGetMaybeCompressedChar(this, irb_, h), args);
    addValue(h, loadedCompressedCharAt);
    return;
  }

  CHECK(!maybe_compressed_char_at) << "must have been handled with FH call";
  size_t array_shift = DataType::SizeShift(array_type);

  Value* loaded = nullptr;
  if (array_type == DataType::Type::kReference &&
      kEmitCompilerReadBarrier && kUseBakerReadBarrier) {

    // Object ArrayGet with Baker's read barrier case.
    // Note that a potential implicit null check is handled in the
    // GenerateArrayLoadWithBakerReadBarrier call.
    if (index->IsConstant()) {
      // Array load with a constant index can be treated as a field load.
      // GenerateFieldLoadWithBakerReadBarrier(
      //     h, out, obj.W(), offset, maybe_temp,
      //     /* needs_null_check= */ false,
      //     /* use_load_acquire= */ false);
      // offset += Int64FromLocation(index) << DataType::SizeShift(type);
      offset += Int64FromLocation(index) << array_shift;
      loaded = fh_->GenerateFieldLoadWithBakerReadBarrier(
          this, irb_, h, larray_obj, offset, false, false);
      loaded->setName("arrayGetRefFieldLoadBakerRead");
    } else {
      // GenerateArrayLoadWithBakerReadBarrier(
      //     h, out, obj.W(), offset, index, /* needs_null_check= */ false);
      loaded = fh_->GenerateArrayLoadWithBakerReadBarrier(
          this, irb_, h, larray_obj, offset, lindex, false);
      loaded->setName("arrayGetRefArrayLoadBakerRead");
    }
  } else {
    // General case.
    Value* loffset = nullptr;
    if (index->IsConstant()) {
      if (maybe_compressed_char_at) {
        // Covered by: FH->ArrayGetMaybeCompressedChar
      } else {
        VERIFY_LLVMD3("NormalConstant");
        offset += Int64FromLocation(index) << array_shift;
        // source = HeapOperand(obj, offset);
        loffset = irb_->getInt32(offset); 
        loffset->setName("offsetStatic");
      }
    } else {
      // __ Add(temp, obj, offset);

      if (maybe_compressed_char_at) {
        // Covered by: FH->ArrayGetMaybeCompressedChar
      } else {
        // source = HeapOperand(temp, XRegisterFrom(index), LSL, DataType::SizeShift(type));
        VERIFY_LLVMD4("NormalDynamic");
        loffset = GetDynamicOffset(index, array_shift, offset);
        loffset->setName("offsetDynamic");
      }
    }

    if (!maybe_compressed_char_at) {
      // codegen_->Load(type, OutputCPURegister(instruction), source);
      loaded=_LoadForFieldGet(h, larray_obj, loffset, is_volatile); 
      Arm64::MaybeRecordImplicitNullCheck(h);
    }

    if (array_type == DataType::Type::kReference) {
      Value* nullref = irb_->getJNull();
      if (index->IsConstant()) {
        VERIFY_LLVM("NormalObjectStatic");
        // codegen_->MaybeGenerateReadBarrierSlow(instruction, out, out, obj_loc, offset);
        loaded= MaybeGenerateReadBarrierSlow(
            h, nullref, larray_obj, loffset, nullptr, is_volatile);
      } else {
        VERIFY_LLVM("NormalObjectDynamic");
        loaded= MaybeGenerateReadBarrierSlow(
            h, nullref, larray_obj, loffset, lindex, is_volatile);
        // codegen_->MaybeGenerateReadBarrierSlow(instruction, out, out, obj_loc, offset, index);
      }
    }
  }

  addValue(h, loaded);
}

void HGraphToLLVM::VisitArraySet(HArraySet* h) {
  DataType::Type value_type = h->GetComponentType();
  bool needs_write_barrier =
    CodeGenerator::StoreNeedsWriteBarrier(value_type, h->GetValue());
  CHECK_NO_INTERMEDIATE_ACCESS(h);
  HInstruction* index = GetArrayIndex(h);
  Value* larray = getValue(h->InputAt(0));
  Value* lindex = getValue(h->InputAt(1));
  CHECK(h->InputAt(2) != nullptr) << __func___ << " value null";
  Value* lvalue = InputAtOrZero(h, 2);
  bool is_zero = InputAtIsZero(h, 2);

  size_t offset = mirror::Array::DataOffset(DataType::Size(value_type)).Uint32Value();
  if (!needs_write_barrier) {
    Value* loffset = nullptr;
    if (index->IsConstant()) {
      VERIFY_LLVMD4("NoWriteBarrier: Constant: " << offset);
      // destination = HeapOperand(array, offset);
      offset += Int64FromLocation(index) << DataType::SizeShift(value_type);
      loffset = irb_->getJInt(offset);
      loffset->setName("offsetStatic");
    } else {
      VERIFY_LLVMD4("NoWriteBarrier: Dynamic");
      // __ Add(temp, array, offset);
      // destination = HeapOperand( temp, XRegisterFrom(index),
      //     LSL, DataType::SizeShift(value_type));
      loffset = GetDynamicOffset(
          lindex, DataType::SizeShift(value_type), offset);
      loffset->setName("offsetDynamic");
    }
    // codegen_->Store(value_type, value, destination);
    std::vector<Value*> __ofst{loffset};
    Value* gep = irb_->CreateInBoundsGEP(larray, __ofst);
    Value* casted_value = lvalue;
    Type* setTy = nullptr;
      
    if (value_type== DataType::Type::kReference) {
      if(is_zero) { // storing null reference
        // Special case.: lvalue is already loaded with zero
        // It is an i32 0 (and not null reference),
        // which forces the use of a w register (wzr)
        setTy = irb_->getJIntTy();
        casted_value = CastForStorage(lvalue, value_type, setTy);
      } else {
        casted_value = GetHandleFromPointer(lvalue);
        casted_value->setName("arraySetval");
        // BUGFIX storing a i32 instead of i64
        setTy = irb_->getJIntTy();
      }
    } else {
      setTy = irb_->getTypeExact(value_type);
      casted_value = CastForStorage(casted_value, value_type, setTy);
    }
    Value* casted_gep = irb_->CreateBitCast(gep, setTy->getPointerTo());
    irb_->mCreateStore(casted_value, casted_gep, false);
    Arm64::MaybeRecordImplicitNullCheck(h);
  } else {
    std::vector<Value*> args {larray, lvalue};
    args.push_back(lindex);
    irb_->CreateCall(
        fh_->ArraySetWriteBarrier(this, irb_, h, offset), args);
  }
}

/**
 * @brief Initialized once on the llvm entry block
 *
 * @param h
 */
void HGraphToLLVM::VisitClinitCheck(HClinitCheck* h) {
  Value* klass = getValue(h->InputAt(0));
  CallClassInitializationCheck(klass);
  addValue(h, klass);
}

/**
 * @brief CHECK putting this in entry_llvm_ basic block might cause issues
 *
 * @param h
 */
void HGraphToLLVM::VisitNullCheck(HNullCheck* h) {
  VERIFIED(prt_->GetInstruction(h));
  D3LOG(INFO) << "VisitNullCheck: " << prt_->GetInstruction(h);
  CHECK(h->GetType() == DataType::Type::kReference)
      << "Null check type should be an Object";

  HInstruction* hreceiver = h->InputAt(0);
  Value* receiver = getValue(hreceiver);

  std::vector<Value*> null_check_args;
  Value* dex_pc = irb_->getJUnsignedInt(h->GetDexPc());
  null_check_args.push_back(receiver);
  null_check_args.push_back(dex_pc);

  irb_->CreateCall(
      fh_->NullCheck(irb_, GetCompilerOptions().GetImplicitNullChecks()),
      null_check_args);
  addValue(h, receiver);
}

void HGraphToLLVM::VisitBoundsCheck(HBoundsCheck* h) {
  D3LOG(INFO) << "VisitBoundsCheck";
  HInstruction* index_location = h->InputAt(0);
  HInstruction* length_location = h->InputAt(1);
  Value* dex_pc = irb_->getJUnsignedInt(h->GetDexPc());

  Value* lindex = getValue(index_location);

  // cast to int
  if (!DataType::IsInt32Any(index_location->GetType())) {
    lindex = irb_->CreateIntCast(lindex, irb_->getJIntTy(),
                                 IRBuilder::IsSigned(index_location->GetType()));
  }

  CHECK(DataType::IsInt32Any(length_location->GetType()))
      << "BoundsCheck: length is not an Integer:"
      << "\n Length type: " << length_location->GetType();

  std::vector<Value*> bounds_check_args;
  bounds_check_args.push_back(lindex);
  bounds_check_args.push_back(getValue(length_location));
  bounds_check_args.push_back(dex_pc);

  irb_->CreateCall(fh_->BoundsCheck(irb_, this), bounds_check_args);
  addValue(h, lindex);
}

void HGraphToLLVM::VisitSuspendCheck(HSuspendCheck* instruction) {
  HBasicBlock* block = instruction->GetBlock();
  if (block->GetLoopInformation() != nullptr) {
    D5LOG(INFO) << "Backloop will generate suspend check: " 
      << prt_->GetInstruction(instruction);
    // DCHECK(block->GetLoopInformation()->GetSuspendCheck() == instruction);
    // The back edge will generate the suspend check.
    return;
  }
  if (block->IsEntryBlock() && instruction->GetNext()->IsGoto()) {
    D5LOG(INFO) << "Backloop will generate suspend check: " 
      << prt_->GetInstruction(instruction);
    // The goto will generate the suspend check.
    return;
  }
  DLOG(INFO) << "GenerateSuspendCheck from VisitSuspendCheck";
#ifdef SUSPEND_CHECK_USE_METHOD
  GenerateSuspendCheckMethod(instruction, nullptr);
#else
  GenerateSuspendCheck(instruction, nullptr);
#endif
  // codegen_->MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
}

void HGraphToLLVM::VisitDivZeroCheck(HDivZeroCheck* h) {  // VIsStringLengthERIFIED_LLVM
  HInstruction* hdivisor = h->InputAt(0);
  Value* dex_pc = irb_->getJUnsignedInt(h->GetDexPc());
  Value* ldivisor = getValue(hdivisor);
  DataType::Type type = h->GetType();

  ldivisor = irb_->UpcastInt(ldivisor, type);

  std::vector<Value*> args;
  if (hdivisor->IsConstant() &&
      hdivisor->AsConstant()->IsArithmeticZero()) {
    args.push_back(dex_pc);
    irb_->CreateCall(fh_->DivZeroFailedStatic(irb_), args);
  } else {
    args.push_back(ldivisor);
    args.push_back(dex_pc);
    irb_->CreateCall(fh_->DivZeroCheck(irb_, type), args);
  }
  addValue(h, ldivisor);
}

void HGraphToLLVM::VisitInstanceOf(HInstanceOf* h) {
  DLOG(INFO) << __func__;
  Value* lobj = getValue(h->InputAt(0));
  HLoadClass* hload_class = h->InputAt(1)->AsLoadClass();
  Value* lclass = getValue(hload_class);

  std::vector<Value*> args{ lobj, lclass };
  Value* res = irb_->CreateCall(fh_->InstanceOf(irb_, h, this), args);
  // casted as it returns JBoolean
  res = irb_->UpcastInt(res, h);
  addValue(h, res);
}

// There was a BUG on 2D arrays at least in some cases..
// @Android10: I think this is fixed.
void HGraphToLLVM::VisitCheckCast(HCheckCast* h) {
  D3LOG(INFO) << "VisitCheckCast";
  // call similar endpoint with InstanceOf?
  ////- slowpath:
  //      + TypeCheckSlowPathARM64
  //          * used by InstanceOf and CheckCast  (fatal for CheckCast)
  //              - instanceof:
  //                  * kQuickInstanceofNonTrivial
  //              + CheckCast
  //                  * kQuickCheckInstanceOf
  //                  * fatal
  Value* lobj = getValue(h->InputAt(0));
  HLoadClass* hload_class = h->InputAt(1)->AsLoadClass();
  Value* lclass = getValue(hload_class);
  Value* ldex_pc = irb_->getJUnsignedInt(h->GetDexPc());

  std::vector<Value*> args{ lobj, lclass, ldex_pc };
  Function* f = fh_->CheckCast(irb_, h, this);
  // Does not return any result.
  // If check fails the RT will throw an exception.
  irb_->CreateCall(f, args);
}

void HGraphToLLVM::VisitThrow(HThrow* h) {
  D3LOG(INFO) << __func___ << GetTwine(h);
  // TODO Throw/Exceptions are not supported
  // There are several problems:
  // we have to enter the runtime, which should decide whether
  // an exception can be handled. If it's handled then the code should
  // be tranfered back to LLVM, and LLVM should silent the exception,
  // and transfer execution to the appropriate catch block.
  VERIFY_LLVM(GetPrettyMethod());
  Value* lexception = getValue(h->InputAt(0));
  ArtCallDeliverException(lexception);

  // exception might be handled
  irb_->CreateUnreachable();
}

void HGraphToLLVM::VisitLoadException(HLoadException* h) {
  VERIFY_LLVM_;
  Value* thread = GetLoadedThread();
  uint32_t exception_offset = GetThreadExceptionOffset();
  Value* exception = LoadWord<true>(thread, exception_offset);
  addValue(h, exception);
  // __ Ldr(OutputRegister(h), GetExceptionTlsAddress());
}

void HGraphToLLVM::VisitClearException(HClearException* clear) {
  VERIFY_LLVM_;
  Value* thread = GetLoadedThread();
  uint32_t exception_offset = GetThreadExceptionOffset();
  StoreToObjectOffset(thread, exception_offset, irb_->getInt32(0));
  // TODO_LLVM
  // Value* exception = HL->LoadWord<false>(thread, exception_offset);
  // __ Str(wzr, GetExceptionTlsAddress());
}


/**
 * @brief 
 * 
 * INFO register allocator removes redundant HConstructorFence 
 * when it immediately follows an HNewInstance to an uninitialized class.
 * Bcz it already had run `dmb` from runtime.
 *
 * Pattern to remove is:
 * "x = HNewInstance; HConstructorFence(x)"
 *
 * @param h
 */
void HGraphToLLVM::VisitNewInstance(HNewInstance* h) {
  // CHECK_LLVM is this optimized?
  VERIFIED("Verify this: GetLoadedArtMethod");
  QuickEntrypointEnum qpoint = h->GetEntrypoint();

  // @Optimizing: uses instruction inputAt(0)/GetRegisterAt(0)
  Value* loaded_class = getValue(h->GetLoadClass());

  VERIFY_LLVMD3("AllocObject: " << qpoint);
  Value* newobj = nullptr;
  switch (qpoint) {
    case QuickEntrypointEnum::kQuickAllocObjectInitialized:
    case QuickEntrypointEnum::kQuickAllocObjectWithChecks:
    case QuickEntrypointEnum::kQuickAllocObjectResolved: {
      newobj = ArtCallAllocObject__(qpoint, loaded_class);
      if (McrDebug::VerifyArtObject()) {
        ArtCallVerifyArtObject(newobj);
      }
    } break;
    default:
      DLOG(FATAL) << __func__ << ": Unsupported qpoint: " << qpoint;
  }
  CHECK(newobj != nullptr);

  newobj->setName("newObj");
  addValue(h, newobj);
  MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
}

// There was an issue with multi-dimensional arrays @android6
void HGraphToLLVM::VisitNewArray(HNewArray* h) {
  QuickEntrypointEnum qpoint =
    CodeGenerator::GetArrayAllocationEntrypoint(h);

  Value* loaded_class = getValue(h->GetLoadClass());
  Value* length = getValue(h->GetLength());
  
  Value* newarray = ArtCallAllocArray__(qpoint, loaded_class, length);
  newarray->setName("newArray");

  addValue(h, newarray);
}

void HGraphToLLVM::VisitConstructorFence(HConstructorFence* h) {
  VERIFIED_;
  Arm64::GenerateMemoryBarrier(irb_, MemBarrierKind::kStoreStore);
}

void HGraphToLLVM::VisitBoundType(HBoundType* h) {
  // REVIEW copied from PrepareForRegisterAllocation on HGraph
  h->ReplaceWith(h->InputAt(0));
  h->GetBlock()->RemoveInstruction(h);
}

void HGraphToLLVM::VisitMemoryBarrier(HMemoryBarrier* h) {
  D3LOG(INFO) << "VisitMemoryBarrier: " << prt_->GetInstruction(h);
  InstructionSet isa =
      llcu_->GetInstructionSet();
  switch (isa) {
    case InstructionSet::kThumb2: {
      ArmThumb::GenerateMemoryBarrier(irb_, h->GetBarrierKind());
      // INFO_LLVM arm64 should be the same with Thumb
    } break;
    default:
      DIE_TODO << "VisitMemoryBarrier: Unimplemented for architecture: " << isa;
  }
}

void HGraphToLLVM::VisitBitwiseNegatedRight(HBitwiseNegatedRight* h) {
  INFO_;
  CHECK(DataType::IsIntegralType(h->GetType())) << h->GetType();
  Value* lhs = getValue(h->InputAt(0));
  Value* rhs = getValue(h->InputAt(0));

  // whether we use W or X registesr
  bool i32=!DataType::Is64BitType(h->GetType());

  Value* res=nullptr;
  switch (h->GetOpKind()) {
    case HInstruction::kAnd:
      // __ Bic(dst, lhs, rhs);
      res=Arm64::__Bic(irb_, lhs, rhs, i32);
      break;
    case HInstruction::kOr:
      // __ Orn(dst, lhs, rhs);
      res=Arm64::__Orn(irb_, lhs, rhs, i32);
      break;
    case HInstruction::kXor:
      // __ Eon(dst, lhs, rhs);
      res=Arm64::__Eon(irb_, lhs, rhs, i32);
      break;
    default:
      LOG(FATAL) << "Unreachable";
  }
  CHECK(res!=nullptr);

  addValue(h, res);
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
