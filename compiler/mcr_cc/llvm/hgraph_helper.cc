/**
 * Just to make hgraph_converter.cc file a bit smaller.
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

#include <llvm/IR/CFG.h>
#include "art_method-inl.h"
#include "asm_arm_thumb.h"
#include "base/mutex-inl.h"
#include "base/mutex.h"
#include "dex/dex_file_loader.h"
#include "driver/compiler_options.h"
#include "gc/space/image_space.h"
#include "image.h"
#include "llvm_macros_irb_.h"
#include "llvm_utils.h"
#include "mcr_cc/analyser.h"
#include "mcr_cc/linker_interface.h"
#include "mcr_cc/match.h"
#include "mcr_cc/os_comp.h"
#include "mcr_rt/invoke_info.h"
#include "optimizing/data_type.h"
#include "optimizing/nodes.h"
#include "optimizing/intrinsics.h"
#include "thread.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

inline bool MethodTypeMatch(DataType::Type htype,
                            DataType::Type dtype,
                            std::string unverified_callee_name) {
  if (DataType::Kind(htype) != DataType::Kind(dtype)) {
    DLOG(ERROR) << __func__ << " not matching: "
                << unverified_callee_name
                << "\nHInvoke RET: " << htype
                << "\nDex RET: " << dtype;
    return false;
  }
  return true;
}

bool HGraphToLLVM::MethodPrototypeMatches(HInvoke* invoke,
                                          const char* shorty,
                                          bool is_static,
                                          std::string unverified_callee_name) {
  D5LOG(INFO) << __func__ << ": " << unverified_callee_name
              << " args: " << std::to_string(invoke->GetNumberOfArguments());
  for (uint32_t i = 0; i < invoke->GetNumberOfArguments(); i++) {
    HInstruction* input = invoke->InputAt(i);
    D5LOG(INFO) << "Arg:" << i << ":" << input->GetType();
  }

  // match return type
  DataType::Type htype, dtype;
  htype = invoke->GetType();
  int sh_idx = 0;
  dtype = DataType::FromShorty(shorty[sh_idx++]);

  if (!MethodTypeMatch(htype, dtype, unverified_callee_name)) {
    DLOG(ERROR) << "Dont match: " << (sh_idx - 1)
                << ": htype: " << htype << " dtype: " << dtype
                << "\nCompiling method: " << GetPrettyMethod();
    return false;
  }
  // INFO in shorty it's implicit. In Invoke it's not!
  uint32_t arg_i = 0;
  if (!is_static) {
    htype = invoke->InputAt(arg_i++)->GetType();
    if (!MethodTypeMatch(htype, DataType::Type::kReference, unverified_callee_name)) {
      DLOG(ERROR) << "Dont match: receiver: arg_i: " << (arg_i - 1)
                  << ": htype: " << htype
                  << "\nCompiling method: " << GetPrettyMethod();
    }
  }

  // verify arguments
  for (; arg_i < invoke->GetNumberOfArguments(); arg_i++) {
    htype = invoke->InputAt(arg_i)->GetType();
    dtype = DataType::FromShorty(shorty[sh_idx++]);
    if (!MethodTypeMatch(htype, dtype, unverified_callee_name)) {
      DLOG(ERROR) << "Dont match: " << (sh_idx - 1)
                  << "htype: " << htype << " dtype: " << dtype
                  << "\nCompiling method: " << GetPrettyMethod();
      return false;
    }
  }

  return true;
}

void HGraphToLLVM::GenerateConstant(HConstant* h, Constant* constant) {
  D3LOG(INFO) << "ConstantType: " << h->GetType();
  addValue(h, constant);
}

void HGraphToLLVM::HandleBinaryOp(HBinaryOperation* h) {
  Value* lhs = getValue(h->GetLeft());
  Value* rhs = getValue(h->GetRight());
  DataType::Type lhtype = h->GetLeft()->GetType();
  DataType::Type rhtype = h->GetRight()->GetType();

  // upcast int types
  if(DataType::IsIntegralType(lhtype)) {
    lhs = irb_->CreateIntCast(lhs,
        irb_->getTypeUpcast(lhtype),
        irb_->IsSigned(lhtype));
  }
  if(DataType::IsIntegralType(rhtype)) {
    rhs = irb_->CreateIntCast(rhs,
        irb_->getTypeUpcast(rhtype),
        irb_->IsSigned(rhtype));
  }

  bool is_fp = DataType::IsFloatingPointType(h->GetResultType());
  bool is_signed = IRBuilder::IsSigned(h->GetLeft()->GetType()) ||
    IRBuilder::IsSigned(h->GetRight()->GetType());

  Value* res = nullptr;

  if (h->IsAdd()) {
    res = irb_->mCreateAdd(is_fp, lhs, rhs);
  } else if (h->IsSub()) {
    res = irb_->mCreateSub(is_fp, lhs, rhs);
  } else if (h->IsMul()) {
    res = irb_->mCreateMul(is_fp, lhs, rhs);
  } else if (h->IsDiv()) {
    res = irb_->mCreateDiv(is_fp, lhs, rhs);
  } else if (h->IsAnd()) {
    res = irb_->CreateAnd(lhs, rhs);
  } else if (h->IsOr()) {
    res = irb_->CreateOr(lhs, rhs);
  } else if (h->IsRem()) {
    res = irb_->mCreateRem(is_fp, is_signed, lhs, rhs);
  } else if (h->IsXor()) {
    res = irb_->CreateXor(lhs, rhs); 
  } else if (IsShiftOperation(h)) {
    res = GetShiftOperation(h, lhs, rhs);
  } else if (h->IsMin() || h->IsMax()) {
    if(is_fp) {
      res = ih_->llvm_MaxOrMinf(this, h, lhs, rhs);
    } else {
      res = ih_->canonical_MaxOrMin(this, h, lhs, rhs);
    }
  } else {
    DLOG(FATAL) << "Unimplemented BinOp: " << prt_->GetInstruction(h);
    UNREACHABLE();
  }

  CHECK(res != nullptr);
  addValue(h, res);
}



inline bool IsSigned(IfCondition ifcond) {
  switch (ifcond) {
    // All types.
    case kCondEQ:  // ==
    case kCondNE:  // !=
      // will not be used
      return false;
      // Signed integers and floating-point numbers.
    case kCondLT:  // <
    case kCondLE:  // <=
    case kCondGT:  // >
    case kCondGE:  // >=
      return true;
      //// Unsigned integers.
    case kCondB:   // <
    case kCondBE:  // <=
    case kCondA:   // >
    case kCondAE:  // >=
      return false;
  }
}

void HGraphToLLVM::HandleCondition(HCondition* h) {
  D5LOG(INFO) << __func___  << GetTwine(h);
  Value* lhs = getValue(h->GetLeft());
  Value* rhs = getValue(h->GetRight());
  DataType::Type result_type = h->GetResultType();

  bool is_fp = DataType::IsFloatingPointType(h->GetLeft()->GetType()) ||
      DataType::IsFloatingPointType(h->GetRight()->GetType());
  bool is_signed = IsSigned(h->GetCondition());

  Value* condition = nullptr;
  if (h->IsEqual()) {
    condition = irb_->mCreateCmpEQ(is_fp, lhs, rhs);
  } else if (h->IsNotEqual()) {
    condition = irb_->mCreateCmpNE(is_fp, lhs, rhs);
  } else if (h->IsGreaterThan() || h->IsAbove()) {
    condition = irb_->mCreateCmpGT(is_fp, is_signed, lhs, rhs);
  } else if (h->IsGreaterThanOrEqual() || h->IsAboveOrEqual()) {
    condition = irb_->mCreateCmpGE(is_fp, is_signed, lhs, rhs);
  } else if (h->IsLessThan() || h->IsBelow()) {
    condition = irb_->mCreateCmpLT(is_fp, is_signed, lhs, rhs);
  } else if (h->IsLessThanOrEqual() || h->IsBelowOrEqual()) {
    condition = irb_->mCreateCmpLE(
        is_fp, irb_->IsSigned(result_type), lhs, rhs);
    condition->setName("cmp_le");
  } else {
    DIE << "Unimplemented condition: " << prt_->GetInstruction(h);
    UNREACHABLE();
  }

  CHECK(condition != nullptr) << "Failed to get condition";

  D3LOG(INFO) << "GetCondition: " << prt_->GetInstruction(h)
              << "\nresultType: " << h->GetResultType()
              << "\nltype: " << h->GetLeft()->GetType()
              << "\nrtype: " << h->GetRight()->GetType();

  Value* casted_res = irb_->CreateIntCast(condition,
      irb_->getType(result_type), irb_->IsSigned(result_type));
  addValue(h, casted_res);
}

Value* HGraphToLLVM::GetShiftOperation(
    HBinaryOperation* h, Value* lhs, Value* rhs) {
  CHECK(!DataType::IsFloatingPointType(h->GetLeft()->GetType()) &&
        !DataType::IsFloatingPointType(h->GetRight()->GetType()));

  Value* casted_lhs = lhs;
  Value* casted_rhs = rhs;
  unsigned int lhs_bits = lhs->getType()->getPrimitiveSizeInBits();
  unsigned int rhs_bits = rhs->getType()->getPrimitiveSizeInBits();
  if (lhs->getType() != rhs->getType()) {
    D3LOG(INFO) << "GetShiftOperation: types don't match: lhs:"
                << Pretty(lhs->getType())
                << "/" << std::to_string(lhs_bits)
                << " rhs:" << Pretty(rhs->getType())
                << "/" << std::to_string(rhs_bits);
    bool is_signed = !h->IsUShr();
    if (lhs_bits > rhs_bits) {
      casted_rhs = irb_->CreateIntCast(rhs, lhs->getType(), is_signed);
    } else {
      casted_lhs = irb_->CreateIntCast(lhs, rhs->getType(), is_signed);
    }
  }

  Value* shifted_val = nullptr;
  if (h->IsShl()) {
    shifted_val = irb_->CreateShl(casted_lhs, casted_rhs);
  } else if (h->IsShr()) {
    shifted_val = irb_->CreateAShr(casted_lhs, casted_rhs);
  } else if (h->IsUShr()) {
    shifted_val = irb_->CreateLShr(casted_lhs, casted_rhs);
  } else if (h->IsRor()) {

    // WORKS
    const bool i32=!DataType::Is64BitType(h->GetResultType());
    if(h->GetRight()->IsConstant()) {
      uint64_t imm2= h->GetConstantRight()->GetValueAsUint64() &
      ((DataType::Size(h->GetLeft()->GetType())*8) - 1);
      shifted_val=Arm64::__Ror(irb_, lhs, imm2, i32);
    } else {
      shifted_val=Arm64::__Ror(irb_, lhs, rhs, i32);
    }

    // CHECK: using llvm intrinsic:
    // %2 = tail call i32 @llvm.fshl.i32(i32 %0, i32 %0, i32 5)
    // Below does not work:
    // Value* rhsUsed=rhs;
    // const bool i32=!DataType::Is64BitType(h->GetResultType());
    // if(h->GetRight()->IsConstant()) {
    //   uint64_t imm= h->GetConstantRight()->GetValueAsUint64() &
    //     ((DataType::Size(h->GetLeft()->GetType())*8) - 1);
    //   rhsUsed=i32?irb_->getJInt(imm):irb_->getJLong(imm);
    // }
    // std::vector<Value*> args{lhs, lhs, rhsUsed};
    // shifted_val=ih_->llvm_fshl(h->GetResultType(), args);
  }

  return shifted_val;
}

void HGraphToLLVM::HandleFieldGet(HInstruction* h) {
  VERIFY_LLVMD4(GetTwine(h));
  HInstruction* hobj = h->InputAt(0);
  Value* lobj = getValue(hobj);
  const FieldInfo& field_info = _GetFieldInfo(h);
  const bool is_volatile = field_info.IsVolatile();
  uint32_t field_idx = field_info.GetFieldIndex();
  uint32_t offset = field_info.GetFieldOffset().Uint32Value();
  D5LOG(INFO) << __func__  << ": field_idx: " << field_idx
    << ": offset: " << offset << "/" << std::hex << offset;

  Value* loaded=nullptr;
  DataType::Type load_type = h->GetType();
  VERIFY_LLVMD4("GenerateFieldLoadWithBakerReadBarrier: " << load_type);
  if (kEmitCompilerReadBarrier && kUseBakerReadBarrier &&
      load_type == DataType::Type::kReference) {
    VERIFY_LLVMD4("GenerateFieldLoadWithBakerReadBarrier");
    // Object FieldGet with Baker's read barrier case.
    // /* HeapReference<Object> */ out = *(base + offset)
    // Note that potential implicit null checks are handled in this
    // CodeGeneratorARM64::GenerateFieldLoadWithBakerReadBarrier call.
    // codegen_->GenerateFieldLoadWithBakerReadBarrier(
    //     instruction,
    //     out,
    //     base,
    //     offset,
    //     maybe_temp,
    //     /* needs_null_check= */ true,
    //     field_info.IsVolatile());
    loaded = fh_->GenerateFieldLoadWithBakerReadBarrier(
        this, irb_, h, lobj, offset,
        true, field_info.IsVolatile());
    loaded->setName("loadedFieldBakerRead");
  } else { // General case.
    Value* loffset = irb_->getInt32(offset);
    if (load_type != DataType::Type::kReference) {
      if (is_volatile) {
        // Note that a potential implicit null check is handled in this LoadAcquire call.
        // NB: LoadAcquire will record the pc info if needed.
        // codegen_->LoadAcquire instruction,
        loaded=Arm64::LoadAcquire(irb_, h, load_type, lobj, loffset, false);
      } else {
        // codegen_->Load(load_type, OutputCPURegister(instruction), field);
        loaded= _LoadForFieldGet(h, lobj, loffset, is_volatile); 
        Arm64::MaybeRecordImplicitNullCheck(h);
      }
    }

    if (load_type == DataType::Type::kReference) {
      // If read barriers are enabled, emit read barriers other than
      // Baker's using a slow path (and also unpoison the loaded
      // reference, if heap poisoning is enabled).
      // codegen_->MaybeGenerateReadBarrierSlow(
      // instruction, out, out, base_loc, offset);
      // INFO index is used only for arrays
      Value* nullref = irb_->getJNull();
      loaded= MaybeGenerateReadBarrierSlow(
          h, nullref, lobj, loffset, nullptr, is_volatile);
      loaded->setName("fieldGetObjReadBarrier");
    }
  }

  if (load_type == DataType::Type::kReference &&
      McrDebug::VerifyArtObject()) {
    LOGLLVM2(INFO, "FieldGet: object");
    ArtCallVerifyArtObject(loaded);
  }
  addValue(h, loaded);
}

void HGraphToLLVM::HandleFieldSet(HInstruction* h) {
  D3LOG(INFO) << __func__;
  HInstruction* hobj = h->InputAt(0);
  HInstruction* hset_val = h->InputAt(1);
  const FieldInfo& field_info = _GetFieldInfo(h);
  DataType::Type field_type = field_info.GetFieldType();
  bool is_volatile = field_info.IsVolatile();
  uint32_t offset = field_info.GetFieldOffset().Uint32Value();
  Value* loffset = irb_->getInt32(offset);

  Value* lobj = getValue(hobj);
  Value* lset_val = getValue(hset_val);
  Value* casted_set_val = lset_val;

  Type* setTy = nullptr;
  if (field_type == DataType::Type::kReference) {
    setTy = irb_->getTypeExact(field_type);
    // we wil be storing a handle
    casted_set_val = GetHandleFromPointer(lset_val);
    casted_set_val->setName("csetval");
    // BUGFIX INFO storing an i32 instead of an i64
    setTy = irb_->getJIntTy();
  } else {
    setTy = irb_->getTypeExact(field_type);
    casted_set_val = CastForStorage(lset_val, field_type, setTy);
  }
  
  if (kPoisonHeapReferences && field_type == DataType::Type::kReference) {
    LOGLLVM2(ERROR, "VERIFY_LLVM poison heap reference");
    lset_val = Arm64::PoisonHeapReference(irb_, lset_val);
  }
  
  if (field_info.IsVolatile()) {
    VERIFY_LLVMD("StoreRelease");
    // DONT use dst.. instead use base+offset (base is obj)
    // Value* dst = Arm64::wLdr(irb_, lobj, offset);  // ??
    // or pull from a w register the obj + offset
    // use directly the lset_val...
    Arm64::StoreRelease(irb_, h, field_type,
        casted_set_val, // src
        lobj, // base
        loffset, // offset
        true);
  } else {
    // codegen_->Store(field_type, source, HeapOperand(obj, offset));
    Arm64::MaybeRecordImplicitNullCheck(h);
    // ptr to store to
    std::vector<Value*> idx_list{loffset};
    Value* gep = irb_->CreateInBoundsGEP(lobj, idx_list);
    Value* casted_gep = irb_->CreateBitCast(gep, setTy->getPointerTo());
    irb_->mCreateStore(casted_set_val, casted_gep, is_volatile);
  }
  if (CodeGenerator::StoreNeedsWriteBarrier(field_type, h->InputAt(1))) {
    VERIFIED("StoreNeedsWriteBarrier");
    CallMarkGCCard(lobj, lset_val, field_type, FieldValueCanBeNull(h));
  }
}

void HGraphToLLVM::HandleGotoMethod(HInstruction* got, HBasicBlock* successor) {
  bool successor_exit=false;
  if (successor->IsExitBlock()) {
    successor_exit = true;
    // return;  // no code needed
  }
  if(!successor_exit) {
    HBasicBlock* block = got->GetBlock();
    HInstruction* previous = got->GetPrevious();
    HLoopInformation* info = block->GetLoopInformation();
    if (info != nullptr && info->IsBackEdge(*block) &&
        info->HasSuspendCheck()) {
      GenerateSuspendCheckMethod(info->GetSuspendCheck(), successor);
      // return;
    }
    else if (block->IsEntryBlock() && (previous != nullptr) &&
        previous->IsSuspendCheck()) {
      GenerateSuspendCheckMethod(previous->AsSuspendCheck(), nullptr);
      // codegen_->MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);
    }
  }
  // We generate GoTo in all cases, since it's required by LLVM
  // and will be optimized away if was flowing to next block anyway
  // (this default behaviour of LLVM differs from ART's optimizing backed)
  // HBasicBlock* hto = got->GetBlock()->GetSuccessors()[0];
  GenerateGoto(successor);
}

void HGraphToLLVM::GenerateSuspendCheckMethod(HSuspendCheck* h,
    HBasicBlock* successor) {
  // BUGFIX for commit: b6c6af
  if (!llcu_->IsOuter() &&
      (cur_lblock_ == getBasicBlock(GetGraph()->GetEntryBlock()))) {
    DLOG(WARNING) << "Moved suspend check on outer block";
    return;
  }

  AnalyseSuspendChecks();

  if(!McrDebug::SkipSuspendCheck()) {
    Function* f =
      fh_->SuspendCheckASM(this, irb_, llcu_->GetInstructionSet(), successor);
    irb_->CreateCall(f);
  }
}

void HGraphToLLVM::GenerateSuspendCheck(HSuspendCheck* h,
                                        HBasicBlock* successor) {
  if(successor == nullptr) {  // BUGFIX for commit: b6c6af
    return;
  }

  BasicBlock* pinsert_point = irb_->GetInsertBlock();
  std::string bbprefix="SuspendCheck_"+Pretty(pinsert_point);
  Function* F= pinsert_point->getParent();

  BasicBlock* do_check=BasicBlock::Create(
      irb_->getContext(), bbprefix, F);
  Value* state_and_flags=irb_->CreateCall(
      fh_->LoadStateAndFlags(this, irb_));
  // __ Ldrh(temp, MemOperand(tr, Thread::ThreadFlagsOffset<kArm64PointerSize>().SizeValue()));
  if (successor == nullptr) { // never enters.. (disabled)
    // if not zero, do a suspend check
    Value* is_non_zero = irb_->mCreateCmpNE(
        false, state_and_flags, irb_->getJUnsignedInt16(0));
    BasicBlock* return_block=BasicBlock::Create(
        irb_->getContext(), bbprefix+"ReturnBlock", F);
    irb_->CreateCondBr(is_non_zero, do_check, return_block);
    irb_->SetInsertPoint(do_check);
    ArtCallTestSuspend();
    irb_->CreateBr(return_block); // ERROR ..
    // __ Cbnz(temp, slow_path->GetEntryLabel());
    // __ Bind(slow_path->GetReturnLabel());
  } else {
    // if zero, skip suspend check
    Value* is_zero = irb_->mCreateCmpEQ(
        false, state_and_flags, irb_->getJUnsignedInt16(0));

     MDNode *BranchWeights=mdb_->createBranchWeights(1000000, 0);
    irb_->CreateCondBr(is_zero, getBasicBlock(successor), do_check,
        BranchWeights);

    irb_->SetInsertPoint(do_check);
    ArtCallTestSuspend();
    GenerateGotoForSuspendCheck(pinsert_point, do_check, successor);
  }

  irb_->SetInsertPoint(pinsert_point);
}

void HGraphToLLVM::HandleGoto(HInstruction* got, HBasicBlock* successor) {
  if (successor->IsExitBlock()) {
    // CHECK might have to generate here
    DCHECK(got->GetPrevious()->AlwaysThrows());
    return;  // no code needed
  }

  HBasicBlock* block = got->GetBlock();
  HInstruction* previous = got->GetPrevious();
  HLoopInformation* info = block->GetLoopInformation();

  if (info != nullptr && info->IsBackEdge(*block) && info->HasSuspendCheck()) {
    GenerateSuspendCheck(info->GetSuspendCheck(), successor);
    return;
  }
  if (block->IsEntryBlock() && (previous != nullptr) && previous->IsSuspendCheck()) {
    GenerateSuspendCheck(previous->AsSuspendCheck(), nullptr);
  }

  // LLVM must always branch to the next BB even if its right after..
  GenerateGoto(successor);
}

/**
 * @brief  The flow here might be weird. We are doing a call, whether it's
 *         static/direct or not, assuming that ArtMethod is resolved.
 *         But CallHotMethod/CallColdMethod actually puts code that resolves
 *         the ArtMethod.
 *         In case of virtual/interface, a second resolution based on the
 *         receiver is done before each call (see @InvokeVirtual)
 * 
 */
void HGraphToLLVM::HandleInvoke(HInvoke* hinvoke) {
  uint32_t dex_method_idx = hinvoke->GetDexMethodIndex();
  uint32_t orig_dex_method_idx = dex_method_idx;
  const DexFile* dex_file = &GetGraph()->GetDexFile();
  MethodReference method_ref(dex_file, dex_method_idx);
  // Used only for intrinsics. We then resolve ArtMethod
  // and get PrettyMethod from it.
  // This is because we might get different pretty name from art method.
  // e.g. get virtual Vehicle.start(), will  return Engine.start()
  // pretty_method_wref will be the Vehicle.start()
  // (which is not explicitly defined in the file), and pretty_method
  // will be Engine.start()
  std::string pretty_method_wref = method_ref.PrettyMethod();
  std::vector<Value*> callee_args;
  for (u_int32_t i = 0; i < hinvoke->GetNumberOfArguments(); i++) {
    callee_args.push_back(getValue(hinvoke->InputAt(i)));
  }

#ifdef EXTRA_INTRINSICS
  if (ih_->GetExtraIntrinsic(pretty_method_wref) != ExtraIntrinsics::kNone) {
    ih_->HandleExtraIntrinsic(this, hinvoke, callee_args, pretty_method_wref);
    return;
  }
#endif

  if (ih_->IsSimplified(hinvoke->GetIntrinsic())) {
    if(ih_->HandleSimplified(
          this, hinvoke, callee_args, pretty_method_wref)) {
      CHECK_LLVM("Intrinsic:: Simplified: skip call of: " << pretty_method_wref);
      // intrinsic was handled. Otherwise continue with the HInvoke
      return;
    }
  } else if (ih_->MustHandle(hinvoke->GetIntrinsic())) {
    ih_->HandleIntrinsic(this, hinvoke, callee_args, pretty_method_wref);
    return;
  }

  const dex::MethodId& method_id = dex_file->GetMethodId(method_ref.index);
  const char* shorty = dex_file->GetMethodShorty(method_id);
  InvokeType invoke_type = hinvoke->GetInvokeType();
  bool is_native = false;

  D2LOG(WARNING) << " \n-> HandleInvoke: PC:" << hinvoke->GetDexPc()
    << ":" << invoke_type
    << ": " << hinvoke->GetDexMethodIndex()
    << ":" << pretty_method_wref;
  Thread* self = Thread::Current();
  Locks::mutator_lock_->SharedLock(self);
  ArtMethod* art_method = ResolveLocalMethod(dex_method_idx, invoke_type);
  CHECK(art_method != nullptr) << "Failed to resolve method. wref: " << pretty_method_wref;
  std::string pretty_method = art_method->PrettyMethod();
  ArtMethod* hinv_art_method = hinvoke->GetResolvedMethod();
  
  std::string msg;
  LogSeverity lvl;
  if(hinvoke->GetResolvedMethod()!=nullptr) {
    std::stringstream ss;
    ss
      << "HINV: Method: " << hinv_art_method->GetDexMethodIndex()
      << ": " << hinv_art_method->PrettyMethod() << "\n"
      << "LocalResolve: " << art_method->GetDexMethodIndex()
      << ": " << art_method->PrettyMethod();
    if (hinv_art_method->GetDexMethodIndex() != 
        art_method->GetDexMethodIndex()) {
      lvl=ERROR;
      msg = " WARNING: ResolveLocalMethod different than HINV Resolved!\n"
        + ss.str();
      art_method = hinv_art_method;
    } else {
      lvl=INFO;
      msg = " ResolveLocalMethod matches HINV Resolved!\n"+ss.str();
    }
  }
  D3LOG(lvl) << __func__ << msg;


  // For multi-dex support:
  // if is multi-dex: update method_ref if needed

  is_native = art_method->IsNative();
  if (is_native) {
    D2LOG(WARNING) << "NATIVE: " << pretty_method;
  }
  if (art_method->IsAbstract()) {
    D2LOG(WARNING) << "ABSTRACT: " << pretty_method
                   << "\nabstract caller: " << GetPrettyMethod();
  }

  if (art_method->GetDexMethodIndex() != method_ref.index) {
    D2LOG(WARNING) << "WARNING: different ArtMethod and MethodRef idx"
      << "\nArtMethod: " << art_method->GetDexMethodIndex() << ": "
      << art_method->PrettyMethod()
      << "\nMRef: " << method_ref.index << ": "
      << method_ref.PrettyMethod()
      << "\nFiles: ArtMethod:"
      << mcr::McrCC::PrettyDexFile(art_method->GetDexFile()->GetLocation())
      << " MRef: "
      << mcr::McrCC::PrettyDexFile(method_ref.dex_file->GetLocation());
  }

  bool is_static = invoke_type == InvokeType::kStatic;
  bool is_static_or_direct = hinvoke->IsInvokeStaticOrDirect();
  std::string signature = GetSignature(hinvoke);
  if(hinvoke->GetResolvedMethod()) {
    D3LOG(INFO) << "SIGNATURE: INV: ORIG: " << signature << "\n"
      << "SIGNATURE: INV:  NEW: "
      << GetSignature(hinvoke->GetResolvedMethod());
  }
  Locks::mutator_lock_->SharedUnlock(self);

  u_int32_t shorty_len =
      1 +  // return type is explicit in shorty
      hinvoke->GetNumberOfArguments() +
      (is_static ? 0 : -1);  // receiver is implicit in shorty, but explicit in HInvoke
  DataType::Type ret_type = hinvoke->GetType();

  bool is_hot = mcr::McrCC::isHot(method_ref);

  D3LOG(INFO)  << "HOTprof:" << is_hot << " "
    << orig_dex_method_idx << "/" << dex_method_idx
    << "\ndidx: " << hinvoke->GetDexMethodIndex()
    << "\n" << orig_dex_method_idx << "/" << dex_method_idx
    << ": Speculation:" << hinvoke->HasSpeculation();

  if (!is_hot) {
    Locks::mutator_lock_->SharedLock(self);
    bool framework_method = mcr::McrRT::IsFrameworkMethod(art_method);

    // Add framework static/direct methods to histogram
    if (framework_method && (art_method->IsStatic() || art_method->IsDirect()
          // sharpened/devirtualized by HGraph
          || hinvoke->IsInvokeStaticOrDirect())) {
      std::string dex_loc = art_method->GetDexFile()->GetLocation();
      std::string dex_base = DexFileLoader::GetBaseLocation(dex_loc);
      if (mcr::Match::ShouldAddToHistogram(pretty_method)) {
        bool exist = false;
        D3LOG(INFO) << "Histogram: entries: " << histogram_complete_->GetSize();
        for (auto it(histogram_complete_->hist_begin());
            it != histogram_complete_->hist_end(); it++) {
          mcr::InvokeInfo ii = *it;

          if (ii.GetSpecMethodIdx() == art_method->GetDexMethodIndex() &&
              ii.GetSpecInvokeType() == art_method->GetInvokeType() &&
              ii.GetCallerMethodIdx() == 0 && ii.GetDexPC() == 0 &&
              (ii.GetDexLocation().compare(dex_loc) == 0)) {
            exist = true;
          }
        }

        if (!exist) {
          D3LOG(WARNING) << "AddToHistogram: " << art_method->PrettyMethod()
            << "/" << art_method->GetDexMethodIndex()
            << "/" << art_method->GetInvokeType()
            << "\npretty_method:" << pretty_method;

          mcr::InvokeInfo info(0, 0,
              art_method->GetDeclaringClass()->GetDexTypeIndex().index_,
              art_method->GetDexMethodIndex(), art_method->GetInvokeType(),
              GetGraph()->GetDexFile().GetLocation().c_str(),
              dex_base, dex_loc);
          mcr::InvokeInfo::AddToCache(info);
          mcr::Analyser::histogram_additions_.insert(art_method->PrettyMethod());
        }
      }
    }
    Locks::mutator_lock_->SharedUnlock(self);
  }

  if (!is_static_or_direct) {
    D3LOG(INFO) << "HandleInvoke: call InvokeWrapper: " << dex_method_idx
      << "CALL METHOD: " << pretty_method
      << "\ncallee_args: sz: " << callee_args.size();
    Locks::mutator_lock_->SharedLock(self);
    Value* call_result=irb_->CreateCall(fh_->InvokeWrapper(
          art_method, is_native, this, irb_, ih_, hinvoke, signature,
          shorty, shorty_len, ret_type), callee_args);
    Locks::mutator_lock_->SharedUnlock(self);
    if (ret_type != DataType::Type::kVoid) {
      addValue(hinvoke, call_result);
    }
  } else {
    HInvokeStaticOrDirect* hinvokeStaticOrDirect =
      hinvoke->AsInvokeStaticOrDirect();
    if (hinvokeStaticOrDirect->IsStringInit()) {
      DLOG(ERROR) << "StringInit: was previously not supported in LLVM.\n"
        "Caller: " << GetPrettyMethod();
    } else if (hinvoke->AsInvokeStaticOrDirect()->IsRecursive()) {
      DLOG(WARNING) << "Recursive method: " << pretty_method;
    }

    bool call_directly = is_static_or_direct && is_hot;
    if (call_directly) {
      D3LOG(INFO) << "Method HOT and Static/Direct: call directly";
      if (MethodPrototypeMatches(hinvoke, shorty, is_static, pretty_method)) {
        D3LOG(INFO) << "HandleInvoke: call CallHotMethod: " << dex_method_idx;
        Value* call_result =
            CallHotMethod(irb_, art_method, nullptr, hinvoke, signature,
                          shorty, shorty_len, is_native,
                          is_static, ret_type, callee_args, true);  
        if (ret_type != DataType::Type::kVoid) {
          addValue(hinvoke, call_result);
        }
      } else {
        DLOG(FATAL) << "Method does not match prototype: " << pretty_method
          << "\n" << GetPrettyMethod();
        UNREACHABLE();
      }
    } else {
      Value* receiver = nullptr;
      if (is_static) {
        receiver = irb_->getJNull();
      } else {
        // pop the 1st element of arguments, which is gonna be the receiver
        // This is because mcr::InvokeMethod requires receiver passed seperately
        // from the rest of the arguments
        receiver = *callee_args.begin();
        callee_args.erase(callee_args.begin());
      }

      D3LOG(INFO) << "HandleInvoke: call CallColdMethod: " << dex_method_idx;
      Value* call_result=CallColdMethod(
          is_hot, irb_, art_method, nullptr, hinvoke, signature,
          shorty, shorty_len, is_native, is_static, ret_type,
          receiver, callee_args, true);

      if (ret_type != DataType::Type::kVoid) {
        addValue(hinvoke, call_result);
      }
    }
  }
}

Value* HGraphToLLVM::CallMethod(
    bool llvm_to_llvm, bool is_hot, IRBuilder* irb,
    ArtMethod* art_method, ArtMethod* spec_art_method,
    HInvoke* hinvoke, std::string signature, const char* shorty, uint32_t shorty_len,
    bool is_native, bool is_static, DataType::Type ret_type,
    Value* receiver, std::vector<Value*> callee_args,
    bool init_inner, Value* rt_resolved_method,
    bool in_miss_block) {
  Thread* self = Thread::Current();
  Locks::mutator_lock_->SharedLock(self);
  D2LOG(WARNING) << "CallMethod:" << (is_hot ? "HOT" : "COLD") << ":"
                 << hinvoke->GetDexMethodIndex() << "/" << art_method->GetDexMethodIndex();

  const bool isInterface = hinvoke->IsInvokeInterface();
  const bool isVirtual= hinvoke->IsInvokeVirtual();
  UNUSED(isVirtual, isInterface);

  std::string spec_method_name, orig_method_name;
  orig_method_name = spec_method_name = art_method->PrettyMethod();

  MethodReference method_ref(art_method->GetDexFile(),
                             art_method->GetDexMethodIndex());

  const bool framework_method = mcr::McrRT::IsFrameworkMethod(art_method);
  const bool multi_dex = false;  // see multi-dex notes

  if (spec_art_method == nullptr) {
    spec_art_method = art_method;
    D3LOG(WARNING) << "WARNING: spec_art_method was null!"
      << "Now set: "  << art_method->GetDexMethodIndex() << ":"
      << art_method->PrettyMethod();
    if(hinvoke->GetResolvedMethod() != nullptr) {
      D3LOG(WARNING) << "HInvoke.Resolved: "
        << hinvoke->GetDexMethodIndex() << ":"
        << hinvoke->GetResolvedMethod()->PrettyMethod();
    }
  } else {
    std::string new_signature = GetSignature(spec_art_method);
    if(new_signature.compare(signature) != 0) {
      D3LOG(WARNING) << "Updating signature to speculative one: "
        << new_signature;
      signature = new_signature;
    }
  }
  uint32_t spec_method_idx = spec_art_method->GetDexMethodIndex();
  const bool is_abstract = spec_art_method->IsAbstract();

  // update callee name
  if (hinvoke->HasSpeculation()) {
    CHECK(spec_art_method != nullptr)
        << "CallMethod: spec_art_method can't be null on speculative call!\n"
        << "Orig ArtMethod: " << orig_method_name;

    spec_method_name = spec_art_method->PrettyMethod();
    D2LOG(INFO) << "spec_method: " 
      << spec_method_idx << ":" << spec_method_name
      << "\norig_method: " << art_method->GetDexMethodIndex() 
      << ":" << orig_method_name;
  }

  std::string call_info;
  if (hinvoke->IsInvokeVirtual()) {
    call_info += "virtual";
  } else if (hinvoke->IsInvokeInterface()) {
    call_info += "interface";
  } else {
    call_info += "static";
  }

  std::string callee_uniq_name=
    GetMethodName(spec_method_name, is_static, signature);
  D3LOG(INFO) << "signature: " << signature
    << "callee_uniq_name:" << callee_uniq_name;

  uint32_t didx = hinvoke->GetDexMethodIndex();
  const DexFile* dex_file = &GetGraph()->GetDexFile();
  if (art_method->GetDexFile()->GetLocation() != dex_file->GetLocation()) {
    D4LOG(INFO) << "didx:" << didx <<
      ":Updating dex file:"
      << mcr::McrCC::PrettyDexFile(dex_file->GetLocation());
    dex_file = art_method->GetDexFile();
  }

  std::string dex_filename, dex_location;
  dex_location = dex_file->GetLocation();
  dex_filename = DexFileLoader::GetBaseLocation(dex_location);
  if (hinvoke->GetDexMethodIndex() != art_method->GetDexMethodIndex()) {
    D4LOG(WARNING) << __func__ << ": WARNING: different resolved methodIdx:"
      << "\nOrig   method: " << art_method->GetDexMethodIndex() << ": " 
      << art_method->PrettyMethod()
      << (art_method->IsAbstract() ? " :abstract" : "")
      << "\nSpec   method: " << spec_method_idx << ": "
      << spec_art_method->PrettyMethod()
      << (spec_art_method->IsAbstract() ? " :abstract" : "")
      << "\nHINV:  Resolv: "
      << hinvoke->GetResolvedMethod()->GetDexMethodIndex()
      << ": " << hinvoke->GetResolvedMethod()->PrettyMethod()
      << "\nHINV:      IDX:" << hinvoke->GetDexMethodIndex()
      << "\nHINV: orig IDX:" << hinvoke->GetOptimizingDexMethodIndex()
      << "\nHINV:     Type:" << hinvoke->GetInvokeType()
      << "\n   Speculation: " << (hinvoke->HasSpeculation() ? "YES" : "NO")
      << " IsAbstract:" << (art_method->IsAbstract() ? "YES" : "NO")
      << "\n      DexFile:" << art_method->GetDexFile()->GetLocation()
      << "\nCaller: " << GetPrettyMethod();

    if (framework_method || multi_dex) {
      if (framework_method) {
        if (!mcr::McrRT::IsFrameworkMethod(spec_art_method)) {

          didx = spec_art_method->GetDexMethodIndex();
          dex_file = spec_art_method->GetDexFile();
          std::stringstream ss;
          ss << "ERROR: App method overriding framework method: "
            << spec_art_method->PrettyMethod()
            << "GOT: didx: " << didx << std::endl
            << "dex_file: " << dex_file->GetLocation() << std::endl
            << "OTHERWISE: didx" << art_method->GetDexMethodIndex() << "\n"
            << "dex_loc: " << art_method->GetDexFile()->GetLocation()
            << "llvm_to_llvm: " << llvm_to_llvm
            << "\nFROM (make this cold):" << GetPrettyMethod();
          DLOG(ERROR) << ss.str();
          ss << "\n";
          LlvmCompiler::LogError(ss.str());

          is_hot=false;

          // CHECK:
          // framework_method = false;
        } else {
          didx = art_method->GetDexMethodIndex();
          dex_file = art_method->GetDexFile();
        }
      } else if (multi_dex) {
        // multi-dex: update dex_file and didx with the proxy reference
        // (i.e., the method that is in the same DexFile with its caller,
        // which is this method that we are currently compiling)
        // sample code:
        // MethodReference proxy_ref = mcr::Analyser::GetProxyRef(method_ref);
        // dex_file = proxy_ref.dex_file;
        // didx = proxy_ref.index;
      }

      dex_location = dex_file->GetLocation();
      dex_filename = DexFileLoader::GetBaseLocation(dex_location);

      if (!hinvoke->HasSpeculation()) {
        spec_art_method = art_method;
        mcr::InvokeInfo* info = new mcr::InvokeInfo(
            didx, 0, 0, art_method->GetDexMethodIndex(),
            art_method->GetInvokeType(),
            GetGraph()->GetDexFile().GetLocation().c_str(),
            DexFileLoader::GetBaseLocation(dex_location),
            dex_location);
        hinvoke->SetSpeculation(info);
      }
    }
  }

  D3LOG(INFO)
    << "didx: " << didx << " Spec:" << hinvoke->HasSpeculation()
    << " framework:" << framework_method << " multiDex:" << multi_dex
    << " HINV.idx:" << hinvoke->GetDexMethodIndex()
    << " ArtMethod.idx:" << art_method->GetDexMethodIndex();

  if (hinvoke->HasSpeculation()) {
    if (!hinvoke->IsSharpened() &&
        hinvoke->GetInvokeType(true) != spec_art_method->GetInvokeType()) {
      DLOG(FATAL) << "InvokeType missmatch:"
        << art_method->PrettyMethod() << ":" << didx
        << "\nSpecArt_method::InvokeType: "
        << spec_art_method->GetInvokeType()
        << "\nHInvoke::InvokeType (smart): " << hinvoke->GetInvokeType()
        << "\nHInvoke::InvokeTypeUnsharpened: "
        << hinvoke->GetInvokeType(true)
        << "\nHInvoke::nInvokeType: "
        << hinvoke->GetInvokeType(false)
        << "\nArtMethod:nInvokeType: " << art_method->GetInvokeType();
    }
  }

  D2LOG(INFO) << "Used: didx: " << didx << "/loc:" << dex_location;
  if (init_inner) {
    AddToInitInnerMethods(spec_method_name, hinvoke, didx,
        is_hot, is_native, is_abstract, signature,
        dex_filename, dex_location, framework_method);
  }
  Locks::mutator_lock_->SharedUnlock(self);

  // if it's hot, regardless whether it will be called directly,
  // we have to add it as a dependency to include it's code
  if (is_hot) {
    mcr::LinkerInterface::AddDependency(GetPrettyMethod(), spec_method_name);
  }

  Value* call_result = nullptr;
  if (llvm_to_llvm) {
    if (McrDebug::VerifyInvokeLlvm()) {
      irb->AndroidLogPrint(INFO, "-> LLVM: "+call_info+": "
          + std::to_string(spec_method_idx)+":"+spec_method_name);
    }

    FunctionType* calleeFTy =
      fh_->GetFunctionType(shorty, shorty_len, is_static, is_native, irb_);
    Function* calleeF=cast<Function>(mod_->getOrInsertFunction(
          callee_uniq_name, calleeFTy).getCallee());

    call_result = irb_->CreateCall(calleeF, callee_args);

    if(hinvoke->IsInvokeStaticOrDirect() &&
        hinvoke->AsInvokeStaticOrDirect()->IsRecursive()) {
      // D3LOG(INFO) << "XRECV: TAIL CALL!";
      CallInst* callInst = (CallInst*)call_result;
      callInst->setTailCall(true);
    }

    if (McrDebug::VerifyInvokeLlvm() && McrDebug::DebugLLVM()) {
      irb->AndroidLogPrint(INFO, "<- LLVM: "+call_info+": "
          + std::to_string(spec_method_idx)+":"+spec_method_name);
    }

  } else {
    // we may have resolved the method to call at runtime right before the call
    Value* resolved_method = rt_resolved_method;
    if (resolved_method == nullptr) {
      GlobalVariable* gbl_art_method =
          GetGlobalArtMethod(spec_method_name, hinvoke, true);
      resolved_method = irb->CreateLoad(gbl_art_method);
      resolved_method->setName("art_method");
    }

    if (is_native) {
#ifdef LLVM_TO_JNI
      call_result =
        fh_->LLVMtoJNI(this, irb_, resolved_method,
            receiver, is_static, callee_args, ret_type, didx,
            shorty, shorty_len, spec_art_method, spec_method_name);

#else
      call_result =
          fh_->InvokeThroughRT_SLOW(this, irb_, resolved_method,
              receiver, is_native, callee_args, ret_type,
              spec_method_name, call_info);
#endif

    } else {
#ifdef LLVM_TO_QUICK
      if (McrDebug::DebugInvokeQuick()) {
        DLOG(FATAL) << __func__ << ": DebugInvokeQuick";
        fh_->DebugInvoke(this, irb_, resolved_method,
                         receiver, is_native, callee_args, ret_type);
      }

      // INFO android6: in some cases InvokeThroughRT_SLOW seemed faster
      // However:
      // 1. we had verification issues on these cases
      // 2. the frequent calls from LLVM to RT are slowdowns anyway
      //    we brought into LLVM those method calls, and problem was
      //    solved!
      bool use_llvm_to_quick = !McrDebug::QuickThroughRT();

      Value* lmethod_or_idx = resolved_method;
      if (use_llvm_to_quick) {
        call_result = fh_->LLVMtoQuick(this, irb_, hinvoke, lmethod_or_idx,
                receiver, is_static, callee_args, ret_type, didx,
                shorty, shorty_len, spec_method_name, call_info);
      } else {
        call_result = fh_->InvokeThroughRT_SLOW(this, irb_, resolved_method,
                receiver, is_native, callee_args, ret_type,
                spec_method_name, call_info);
      }
#else
      // INFO this is slow in most cases
      call_result =
        fh_->InvokeThroughRT_SLOW(this, irb_, resolved_method,
            receiver, is_native, callee_args, ret_type,
            spec_method_name, call_info);
#endif
    }
  }

  hinvoke->UnsetSpeculation();
  return call_result;
}

Value* HGraphToLLVM::MaybeGenerateReadBarrierSlow(HInstruction* instruction,
    Value* ref, Value* obj, Value* offset, Value* index, bool is_volatile) {
  VERIFIED_;
  Value* read_obj;
  // INFO index might be null (if NOT array..)
  if (kEmitCompilerReadBarrier) {
    // Baker's read barriers shall be handled by the fast path
    // (CodeGeneratorARM64::GenerateReferenceLoadWithBakerReadBarrier).
    // If heap poisoning is enabled, unpoisoning will be taken care of
    // by the runtime within the slow path.
    read_obj = GenerateReadBarrierSlow(instruction, ref, obj, offset, index);
  } else if (kPoisonHeapReferences) { 
    // CHECK might simple read not be appropriate for this..
    Value* read_obj = _LoadForFieldGet(instruction, obj, offset, is_volatile);
    read_obj = Arm64::UnpoisonHeapReference(irb_, read_obj);
  }
  CHECK(read_obj!=nullptr);
  return read_obj;
}

void HGraphToLLVM::MaybeGenerateMarkingRegisterCheck(int code) {
  // The following condition is a compile-time one, so it does not have a run-time cost.
  if (kEmitCompilerReadBarrier && kUseBakerReadBarrier && kIsDebugBuild) {
    // The following condition is a run-time one; it is executed after the
    // previous compile-time test, to avoid penalizing non-debug builds.
    if (GetCompilerOptions().EmitRunTimeChecksInDebugMode()) {
      Arm64::GenerateMarkingRegisterCheck(irb_, code);
    }
  }
}

// boolean java.lang.Double.isInfinite(double)
Value* HGraphToLLVM::GenIsInfinite(Value* input, bool is64bit) {
  Type* ty;
  Value* linfinity; Value *zero;
  Value* one_unsigned=irb_->getJUnsignedInt(1);

  if (is64bit) {
    linfinity = irb_->getJLong(kPositiveInfinityDouble);
    ty=irb_->getJLongTy();
    zero=irb_->getJLong(0);
    one_unsigned=irb_->CreateZExt(one_unsigned, ty);
  } else {
    linfinity = irb_->getJInt(kPositiveInfinityFloat);
    zero=irb_->getJInt(0);
    ty=irb_->getJIntTy();
  }

  // MoveFPToInt(locations, is64bit, masm);
  // INFO: this does not work, so we rely to inline asm
  // Value* fpToSI= irb_->CreateFPToSI(input, ty);
  Value* fpToSI=Arm64::__Fmov(irb_, input, !is64bit);

  // __ Eor(out, out, infinity);
  Value *eor=irb_->CreateXor(fpToSI, linfinity);

  // We don't care about the sign bit, so shift left.
  // __ Cmp(zero, Operand(out, LSL, 1));
  // __ Cset(out, eq);
  Value* shifted=irb_->CreateShl(eor, one_unsigned);

  Value* lcond=irb_->CreateICmpEQ(zero, shifted);
  Value* ltruev = irb_->getJUnsignedInt(true);
  Value* lfalsev = irb_->getJUnsignedInt(false);
  Value* res = irb_->CreateSelect(lcond, ltruev, lfalsev);
  return res;
}


// OPTIMIZE_LLVM interfaces could be resolved with operations directly in
// LLVM. This method is not finished as there are cases with runtime 
// conflicts. See comments below.
Value* HGraphToLLVM::ResolveInterfaceMethod(
    HInvokeInterface *hinvoke, Value* receiver) {
  DIE_TODO << "To use this must handle runtime method conflicts.";
  // see artResolveInterfaceMethodFromLLVM to figure out how
  // conflicts for runtime methods are handled
  
  // /* HeapReference<Class> */ temp = receiver->klass_
  // codegen_->MaybeRecordImplicitNullCheck(invoke);
  ReadBarrierOption RBO = ReadBarrierOption::kWithoutReadBarrier;
  Value* obj_class =
    GenerateReferenceObjectClass(hinvoke, receiver, RBO);

  irb_->AndroidLogPrintHex(INFO, "interface: obj_class", obj_class);
  ArtCallVerifyArtClass(obj_class);

  // Instead of simply (possibly) unpoisoning `temp` here, we should
  // emit a read barrier for the previous class reference load.
  // However this is not required in practice, as this is an
  // intermediate/temporary reference and because the current
  // concurrent copying collector keeps the from-space memory
  // intact/accessible until the end of the marking phase (the
  // concurrent copying collector may not in the future).
  // GetAssembler()->MaybeUnpoisonHeapReference(temp.W());

  u_int32_t offset=
    mirror::Class::ImtPtrOffset(kArm64PointerSize).Uint32Value();
  Value* imtPtr=LoadWord<true>(obj_class, offset);
  imtPtr->setName("imtPtr");

  irb_->AndroidLogPrintHex(INFO, "interface: imtPtr", imtPtr);

  uint32_t method_offset = static_cast<uint32_t>(
      ImTable::OffsetOfElement(
        hinvoke->GetImtIndex(), kArm64PointerSize));

  // temp = temp->GetImtEntryAt(method_offset);
  // __ Ldr(temp, MemOperand(temp, method_offset));
  Value* imtEntry=LoadWord<true>(imtPtr, method_offset);
  imtEntry->setName("imtEntry");

  irb_->AndroidLogPrintHex(INFO, "interface: imtEntry", imtEntry);
  ArtCallVerifyArtMethod(imtEntry);

  // TODO here it might have to check conflicts table if it's
  // a runtime method.

  // INFO: this is for calling interface, as code was based on:
  // InstructionCodeGeneratorARM64::VisitInvokeInterface
  // we still use the llvm_to_quick for calling!
  // lr = temp->GetEntryPoint();
  // __ Ldr(lr, MemOperand(temp, entry_point.Int32Value()));
  // // lr();
  // __ blr(lr);
  // codegen_->MaybeGenerateMarkingRegisterCheck(/* code= */ __LINE__);

  return imtEntry;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
