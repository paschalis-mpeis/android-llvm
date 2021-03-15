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
#include "function_helper.h"

#include <llvm/IR/Argument.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Metadata.h>

#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"
#include "ir_builder.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

inline void _throwException(IRBuilder* irb, FunctionHelper* fh,
    std::string msg, Value* dex_pc) {
  // TODO throw exception
  msg = "TODO_LLVM Throw: CheckCast:" + msg + "DexPC:: %u\n";
  Value* fmt = irb->mCreateGlobalStringPtr(msg);
  std::vector<Value*> args {
    irb->AndroidLogSeverity(ERROR), fmt, dex_pc };
  irb->CreateCall(fh->AndroidLog(), args);
  fh->CallExit(irb, 1);
}


/**
 * @brief calls art_quick_check_instance_of (through an entrypoint) and
 *        if the check fails it dies by the runtime.
 *
 */
inline void _SlowPathCheckInstanceOf(
    HGraphToLLVM *HL, IRBuilder* irb, Value* lobj, Value* lclass) {
  HL->ArtCallCheckInstanceOf(lobj, lclass);
  irb->CreateRetVoid();
}

/**
 * @brief calls artInstanceOfFromCode (through an entrypoint) and
 *        returns a boolean
 *
 */
inline void _SlowPathInstanceOfNonTrivial(
    HGraphToLLVM *HL, IRBuilder* irb, Value* lobj, Value* lclass) {
  Value* res = HL->ArtCallInstanceOfNonTrivial(lobj, lclass);
  res->setName("isInstanceOf_SlowPath");

  // cast to boolean
  Value* casted_res = irb->CreateIntCast(res, irb->getJBooleanTy(),
        irb->IsSigned(DataType::Type::kBool));
  irb->CreateRet(casted_res);
}

/**
 * @brief Compares an object's class with the given type_idx
 *        
 *        on success returns true
 *        failure: has to be handled by the calling code
 *
 * @param die if true it dies at RT and returns void
 */
inline void _cmpObjClassWithClass(
  HGraphToLLVM* HL, FunctionHelper* fh, HInstruction* h,
  LLVMContext* ctx, IRBuilder* irb, Function* f,
  Value* lobj, Value* lclass, Value* ldex_pc,
  ReadBarrierOption read_barrier_option, bool die) {
  // VERIFY_LLVMD("GetObjectClass using: GenerateReferenceLoad");
  Value* lobj_cls =
    HL->GenerateReferenceObjectClass(h, lobj, read_barrier_option);
  lobj_cls->setName("obj_cls");

  BasicBlock* cls_same = BasicBlock::Create(*ctx, "cls_same", f);
  BasicBlock* cls_diff = BasicBlock::Create(*ctx, "cls_diff", f);

  Value* cls_match = irb->mCreateCmpEQ(false, lobj_cls, lclass);

  irb->CreateCondBr(cls_match, cls_same, cls_diff);
  irb->SetInsertPoint(cls_same);
  // VERIFY_LLVMD4("InstanceOf: true");
  if(die) {
    irb->CreateRetVoid();
  } else {
    // cast to boolean
    irb->CreateRet(irb->getJBoolean(true));
  }

  // fast check failed
  irb->SetInsertPoint(cls_diff);
  // for die: it will call at cls_diff the RT to die from it
}

/**
 * @brief _cmpObjClassWithClass and calls:
 *        slow path on false (cls_diff BasicBlock)
 *
 */
inline void _cmpObjClassWithClass_SlowPathOnFalse(
    HGraphToLLVM* HL, FunctionHelper* fh, HInstruction* h,
    LLVMContext* ctx, IRBuilder* irb, Function* f,
    Value* lobj, Value* lclass, Value* ldex_pc,
    ReadBarrierOption read_barrier_option, bool die) {

  _cmpObjClassWithClass(HL, fh, h, ctx, irb, f, lobj, lclass, ldex_pc,
      read_barrier_option, die);
  if(die) {
    _SlowPathCheckInstanceOf(HL, irb, lobj, lclass);
  } else {
    _SlowPathInstanceOfNonTrivial(HL, irb, lobj, lclass);
  }
}


/**
 * @brief compares lobl->class_ == lclass
 *
 *        if match: return true, else return false:
 *
 *        it does NOT keep trying,
 *        e.g. through RT or by following super classes
 *
 * @param die if true it dies at RT and returns void
 */
inline void _exact_match(
    HGraphToLLVM* HL, FunctionHelper* fh, HInstruction* h,
  LLVMContext* ctx, IRBuilder* irb, Function* f,
  Value* lobj, Value* lclass, Value* ldex_pc, 
  ReadBarrierOption read_barrier_option, bool die) {

  _cmpObjClassWithClass(HL, fh, h, ctx, irb, f, lobj, lclass,
      ldex_pc, read_barrier_option, die);
  if(die) {
    _SlowPathCheckInstanceOf(HL, irb, lobj, lclass);
  } else {
    irb->CreateRet(irb->getJBoolean(false));
  }
}

inline void _doNullCheck(
  LLVMContext* ctx, IRBuilder* irb, Function* f, Value* val) {
  BasicBlock* is_null =
    BasicBlock::Create(*ctx, "is_null", f);
  BasicBlock* not_null =
    BasicBlock::Create(*ctx, "not_null", f);

  Value* eq_null = irb->mCreateCmpEQ(false, val, irb->getJNull());
  irb->CreateCondBr(eq_null, is_null, not_null);

    // Return 0 if null.
    irb->SetInsertPoint(is_null);
    irb->CreateRet(irb->getJBoolean(false));

    irb->SetInsertPoint(not_null); // not null, continue
}

// Will do a null check. If obj it's null, it will return,
// and wont perform any comparisons.
// otherwise it will set the code entry to the not null BB
inline void _doNullCheck_ReturnIfNull(
    LLVMContext* ctx, IRBuilder* irb, Function* f, Value* val) {
  BasicBlock* is_null=BasicBlock::Create(*ctx, "is_null", f);
  BasicBlock* not_null=BasicBlock::Create(*ctx, "not_null", f);

  Value* eq_null = irb->mCreateCmpEQ(false, val, irb->getJNull());
  irb->CreateCondBr(eq_null, is_null, not_null);

  // simply return if null
  irb->SetInsertPoint(is_null);
  if(McrDebug::DebugLlvmCode3()) {
    irb->AndroidLogPrint(WARNING, "CheckCast: null: returning..");
  }
  irb->CreateRetVoid();

  // not null, continue
  irb->SetInsertPoint(not_null);
}

#ifdef CODE_UNUSED
inline void _doNullCheckDie(LLVMContext* ctx,FunctionHelper *fh,
    IRBuilder* irb, Function* f, Value* val, Value* dex_pc) {
  BasicBlock* is_null =
    BasicBlock::Create(*ctx, "is_null", f);
  BasicBlock* not_null =
    BasicBlock::Create(*ctx, "not_null", f);

  Value* eq_null = irb->mCreateCmpEQ(false, val, irb->getJNull());
  irb->CreateCondBr(eq_null, is_null, not_null);

  // die if null
  irb->SetInsertPoint(is_null);
  if(McrDebug::DebugLlvmCode2()) {
    irb->AndroidLogPrint(ERROR, "_doNullCheckDie: DYING!");
  }

  _throwException(irb, fh, "Null pointer exception: Object: ", dex_pc);

  irb->SetInsertPoint(not_null); // not null, continue
}
#endif

}  // namespace LLVM
}  // namespace art
