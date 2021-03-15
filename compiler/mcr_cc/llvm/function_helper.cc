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
#include "intrinsic_helper.h"
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

#include "llvm_macros_irb.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

std::string FunctionHelper::GetCompareName(DataType::Type type) {
  std::stringstream ss;
  ss << type;
  std::string stype = ss.str();
  return "Compare" + stype;
}

ALWAYS_INLINE std::string __GetTypeBitsStr(DataType::Type type) {
  switch (type) {
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kInt8:
    case DataType::Type::kUint16:
    case DataType::Type::kInt16:
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      return "32";
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      return "64";
    default:
      DLOG(FATAL) << "Wrong type: " << type;
      UNREACHABLE();
  }
}

std::string FunctionHelper::GetDivZeroCheckName(DataType::Type type) {
  return "DivZeroCheck" + __GetTypeBitsStr(type);
}

FunctionType* FunctionHelper::GetCompareTy(
    IRBuilder* irb, DataType::Type type) {
  Type* ret_type = irb->getJIntTy();
  std::vector<Type*> args_type;

  Type* arg_type = nullptr;

  switch (type) {
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      arg_type = irb->getJIntTy();
      break;
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      arg_type = irb->getJLongTy();
      break;
    case DataType::Type::kFloat64:
      arg_type = irb->getJDoubleTy();
      break;
    case DataType::Type::kFloat32:
      arg_type = irb->getJFloatTy();
      break;
    default:
      DLOG(FATAL) << "Wrong type: " << type;
      UNREACHABLE();
  }

  args_type.push_back(arg_type);
  args_type.push_back(arg_type);
  return FunctionType::get(ret_type, args_type, false);
}

FunctionType* FunctionHelper::GetDivZeroCheckTy(
    IRBuilder* irb, DataType::Type type) {
  Type* ret_type = irb->getJVoidTy();
  std::vector<Type*> args_type;

  Type* arg_type = nullptr;

  switch (type) {
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kInt8:
    case DataType::Type::kUint16:
    case DataType::Type::kInt16:
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      arg_type = irb->getJIntTy();
      break;
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      arg_type = irb->getJLongTy();
      break;
    default:
      DLOG(FATAL) << "Wrong type: " << type;
      UNREACHABLE();
  }

  args_type.push_back(arg_type);
  args_type.push_back(irb->getJIntTy());  // DexPC
  return FunctionType::get(ret_type, args_type, false);
}

void FunctionHelper::StoreCompareFunction(
    Function* compare_function, DataType::Type type) {
  switch (type) {
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      compare_int_ = compare_function;
      break;
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      compare_long_ = compare_function;
      break;
    case DataType::Type::kFloat64:
      compare_double_ = compare_function;
      break;
    case DataType::Type::kFloat32:
      compare_float_ = compare_function;
      break;
    default:
      DLOG(FATAL) << "Wrong type: " << type;
      UNREACHABLE();
  }
}

void FunctionHelper::StoreDivZeroCheckFunction(
    Function* div_zero_check_function, DataType::Type type) {
  switch (type) {
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kInt8:
    case DataType::Type::kUint16:
    case DataType::Type::kInt16:
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      div_zero_check_int_ = div_zero_check_function;
      break;
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      div_zero_check_long_ = div_zero_check_function;
      break;
    default:
      DLOG(FATAL) << "Wrong type: " << type;
      UNREACHABLE();
  }
}

Function* FunctionHelper::GetCompareFunction(DataType::Type type) {
  LOG(FATAL) << "GetCompareFunction:";
  switch (type) {
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      return compare_int_;
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      return compare_long_;
    case DataType::Type::kFloat64:
      return compare_double_;
    case DataType::Type::kFloat32:
      return compare_float_;
    default:
      DLOG(FATAL) << "Wrong type: " << type;
      UNREACHABLE();
  }
}

Function* FunctionHelper::GetDivZeroCheckFunction(DataType::Type type) {
  switch (type) {
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kInt8:
    case DataType::Type::kUint16:
    case DataType::Type::kInt16:
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      return div_zero_check_int_;
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      return div_zero_check_long_;
    default:
      DLOG(FATAL) << "Wrong type: " << type;
      UNREACHABLE();
  }
}

/**
 * @brief Returns the compare function requested according to type.
 *        It creates the function if it does not exist.
 *        It 'pushes' and 'pops' the current basic block that IRBuilder
 *        is working on, so Visitors will remain unafffected.
 */
Function* FunctionHelper::Compare(
    IRBuilder* irb, DataType::Type type) {
  Function* compare_function = GetCompareFunction(type);
  if (compare_function != nullptr) {
    return compare_function;
  }

  D3LOG(INFO) << "Creating function: Compare for:" << type;
  // backup insertion point (Visitors are recursive)
  BasicBlock* pinsert_point = irb->GetInsertBlock();
  FunctionType* compare_ty = GetCompareTy(irb, type);
  compare_function = Function::Create(
      compare_ty, Function::LinkOnceODRLinkage,
      GetCompareName(type), irb->getModule());
  // compare_function->addFnAttr(Attribute::AlwaysInline);
  compare_function->setDSOLocal(true);
  AddAttributesFastASM(compare_function);

  Function::arg_iterator arg_iter(compare_function->arg_begin());
  Argument* lhs = &*arg_iter++;
  Argument* rhs = &*arg_iter++;
  lhs->setName("lhs");
  rhs->setName("rhs");

  BasicBlock* entry_block =
      BasicBlock::Create(irb->getContext(), "entry", compare_function);
  BasicBlock* equal_block =
      BasicBlock::Create(irb->getContext(), "equal", compare_function);
  BasicBlock* nequal_block =
      BasicBlock::Create(irb->getContext(), "nequal", compare_function);

  // restore insertion point
  irb->SetInsertPoint(entry_block);
  bool is_fp = DataType::IsFloatingPointType(type);
  bool is_signed = IRBuilder::IsSigned(type);
  Value* cond_eq = irb->mCreateCmpEQ(is_fp, lhs, rhs);
  irb->CreateCondBr(cond_eq, equal_block, nequal_block);

  // equal: ret 0
  irb->SetInsertPoint(equal_block);
  irb->CreateRet(irb->getJInt(0));

  // greater: ret 1, smaller: ret -1
  irb->SetInsertPoint(nequal_block);
  Value* cond_gt = irb->mCreateCmpGT(is_fp, is_signed, lhs, rhs);
  Value* select = irb->CreateSelect(cond_gt, irb->getJInt(1), irb->getJInt(-1));
  irb->CreateRet(select);

  irb->SetInsertPoint(pinsert_point);
  StoreCompareFunction(compare_function, type);

  return compare_function;
}

void FunctionHelper::VerifyThread(HGraphToLLVM* HL,
    IRBuilder* irb) {
  Value* current_thread = HL->GetLoadedThread();
  current_thread->setName("thread");

  std::vector<Value*> args;
  args.push_back(current_thread);

  Function* f = __VerifyThread();
  irb->CreateCall(f, args);
}

#define LLVM_PRINT_BSS_INFO(BSS_SLOT, BSS_OBJ, DESC) \
  if(McrDebug::DebugLlvmCode4()) { \
      std::string msg = name + ": " DESC ": obj: 0x%lx (slot:0x%lx)"; \
      Value* fmt = irb->mCreateGlobalStringPtr(msg); \
      std::vector<Value*> prt_args { \
        irb->AndroidLogSeverity(INFO),fmt, BSS_OBJ, BSS_SLOT}; \
      irb->CreateCall(AndroidLog(), prt_args); \
    }

/**
 * @brief   
 *         OPTIMIZE_LLVM: currently it goes through RT for every call.
 *        
 *          I could pass some offsets at entry of LLVM and utilize the
 *          BSS optimization...
 *
 */
Function* FunctionHelper::LoadClass(HGraphToLLVM* HL, IRBuilder* irb,
    HLoadClass* cls, uint32_t caller_didx, bool use_cache) {
  D3LOG(INFO) << __func__;
  uint32_t class_idx = cls->GetTypeIndex().index_;

  // OPTIMIZE_LLVM: find a way to access object bss
  HLoadClass::LoadKind load_kind = cls->GetLoadKind();
  QuickEntrypointEnum qpoint = QuickEntrypointEnum::kQuickLLVMResolveType;
  switch(load_kind) {
    case HLoadClass::LoadKind::kBootImageLinkTimePcRelative:
      DIE << "Must be handled without RT call";
      qpoint = QuickEntrypointEnum::kQuickLLVMResolveTypeInternal;
      break;
    default:
      ;
  }

  // INFO removing caller_didx might case issues on multi-dex
  // Solution: get multi-dex ID (e.g. number of the dex file)
  // HL->GetDexFile->getDexFileId() ?
  std::stringstream ss;
  ss << "Class" << load_kind
    // CHECK does this affect correctness?
    << "ClsIdx" << std::to_string(class_idx);
  if(use_cache) ss <<"Cached";
  // include in the function name the caller, so the appropriate
  // art method
  std::string name = "Load"+ ss.str();//+"From"+std::to_string(caller_didx);

  if (load_class_[name] != nullptr) return load_class_[name];

  BasicBlock* pinsert_point = irb->GetInsertBlock();
  std::vector<Type*> argsTy {irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), argsTy, false);
  Function* func = Function::Create(
      ty, Function::LinkOnceODRLinkage, name, irb->getModule());
  load_class_[name] = func;
  // func->addFnAttr(Attribute::AlwaysInline);
  func->setDSOLocal(true);
  AddAttributesCommon(func);

  // process arguments
  Function::arg_iterator arg_iter(func->arg_begin());
  Argument* arg_art_method = &*arg_iter;
  arg_art_method->setName("callee_art_method");

  LLVMContext& ctx = irb->getContext();
  BasicBlock* bbLlvmCache = use_cache?
    BasicBlock::Create(ctx, "llvm_cache", func):nullptr;
  BasicBlock* bbHit = use_cache?
    BasicBlock::Create(ctx, "cache_hit", func):nullptr;
  BasicBlock* bbMiss = BasicBlock::Create(ctx, "slow_path", func);

  Value* globalBssSlot = irb->getJNull();
  Value* bssSlot = nullptr;
  Value* bssObj = nullptr;

  if(use_cache) {
    // Create global variable
    globalBssSlot = HL->GetGlobalBssSlot(ss.str());

    irb->SetInsertPoint(bbLlvmCache);
    
    bssSlot = HL->LoadGlobalBssSlot(ss.str());
    Value* cache_null = irb->CreateCmpIsNull(bssSlot);

    MDNode *N=HL->MDB()->createBranchWeights(0, MAX_BRWEIGHT);
    irb->CreateCondBr(cache_null, bbMiss, bbHit, N);

    // if not null: dereference and return
    irb->SetInsertPoint(bbHit);

    // dereference bss slot to get the obj (might change by RT)
    bssObj= irb->CreateBitCast(bssSlot,
        irb->getVoidPointerType()->getPointerTo());

    bssObj = HL->LoadWord<true>(bssObj, static_cast<uint32_t>(0));
    bssObj->setName("bssObj");

    if(McrDebug::DebugLlvmCode2() &&
        McrDebug::VerifyArtClass()) {
      LLVM_PRINT_BSS_INFO(bssSlot, bssObj, "HIT");
      HL->ArtCallVerifyArtClass(bssObj);
    }
    irb->CreateRet(bssObj);
  }

  // else: resolve through Slow path
  // get the object directly from RT,
  // but also send the globalBssSlot as a reference
  // to be written by the RT
  irb->SetInsertPoint(bbMiss);

  // CHECK why this and not GenerateGcRootFieldLoad ?
  Value* resolved_class = HL->ArtCallResolveType__(
      qpoint, arg_art_method, class_idx, globalBssSlot);
  resolved_class->setName("resolvedClass");

  if(use_cache) {
    if (cls->MustGenerateClinitCheck()) {
      VERIFY_LLVMD("Generated clinit");
      HL->CallClassInitializationCheck(resolved_class);
    }

    if(McrDebug::DebugLlvmCode3()) {
      bssSlot= HL->LoadGlobalBssSlot(ss.str());
      bssObj = irb->CreateBitCast(bssSlot,
          irb->getVoidPointerType()->getPointerTo());
      bssObj = HL->LoadWord<true>(bssObj, static_cast<uint32_t>(0));
      LLVM_PRINT_BSS_INFO(bssSlot, bssObj, "MISS");

      std::vector<Value*> args {bssSlot};
      irb->CreateCall(__VerifyBssObject(), args);
      HL->ArtCallVerifyArtClass(resolved_class);
    }
  }
  irb->CreateRet(resolved_class);

  irb->SetInsertPoint(pinsert_point);
  return func;
}

Function* FunctionHelper::LoadString(HGraphToLLVM* HL, IRBuilder* irb,
    HLoadString* h , uint32_t string_idx, bool use_cache) {
  D3LOG(INFO) << __func__;
  HLoadString::LoadKind load_kind = h->GetLoadKind();

  std::stringstream ss;
  ss << "String" << load_kind
    << std::to_string(string_idx)
    << HL->GetDexMethodIndex();
  if(use_cache) ss << "Cached";
  std::string name = "Load" + ss.str();

  if (load_string_[name] != nullptr) return load_string_[name];
  VERIFY_LLVMD4(name);

  BasicBlock* pinsert_point = irb->GetInsertBlock();
  std::vector<Type*> argsTy {irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), argsTy, false);
  Function* func = Function::Create(
        ty, Function::LinkOnceODRLinkage, name, irb->getModule());
  load_string_[name] = func;
  // func->addFnAttr(Attribute::AlwaysInline);
  func->setDSOLocal(true);
  AddAttributesCommon(func);

  // process arguments
  Function::arg_iterator arg_iter(func->arg_begin());
  Argument* arg_art_method = &*arg_iter;
  arg_art_method->setName("art_method");

  LLVMContext& ctx = irb->getContext();
  BasicBlock* bbLlvmCache = use_cache?
    BasicBlock::Create(ctx, "llvm_cache", func): nullptr;
  BasicBlock* bbHit = use_cache?
    BasicBlock::Create(ctx, "cache_hit", func):nullptr;
  BasicBlock* bbMiss = BasicBlock::Create(ctx, "slow_path", func);
  Value* globalBssSlot = irb->getJNull();
  irb->getJNull();
  Value* bssSlot = nullptr;
  Value* bssObj = nullptr;

  if(use_cache) {
    // Create global variable
    globalBssSlot =  HL->GetGlobalBssSlot(ss.str());

    irb->SetInsertPoint(bbLlvmCache);
    
    bssSlot = HL->LoadGlobalBssSlot(ss.str());
    Value* cache_null = irb->CreateCmpIsNull(bssSlot);
      MDNode *N=HL->MDB()->createBranchWeights(0, MAX_BRWEIGHT);
    irb->CreateCondBr(cache_null, bbMiss, bbHit, N);

    // if not null: dereference and return
    irb->SetInsertPoint(bbHit);

    // dereference bss slot to get the obj (might change by RT)
    // https://godbolt.org/z/vEjEza
    bssObj= irb->CreateBitCast(bssSlot,
        irb->getVoidPointerType()->getPointerTo());

    bssObj = HL->LoadWord<true>(bssObj, static_cast<uint32_t>(0));
    // bssObj= HL->LoadFromAddress(bssObj, 0,
    //     irb->getVoidPointerType()->getPointerTo());
    bssObj->setName("bssObj");

    if(McrDebug::DebugLlvmCode4()) {
      LLVM_PRINT_BSS_INFO(bssSlot, bssObj, "HIT");
      VERIFIED("calling VerifyString");
      std::vector<Value*> args {bssObj};
      irb->CreateCall(__VerifyString(), args);
    }
    irb->CreateRet(bssObj);
  }

  // else: resolve through Slow path
  // get the object directly from RT,
  // but also send the globalBssSlot as a reference
  // to be written by the RT
  irb->SetInsertPoint(bbMiss);
  // GenerateGcRootFieldLoad
  Value* resolved_string = HL->ArtCallResolveString(
      arg_art_method, string_idx, globalBssSlot);
  resolved_string->setName("resolvedString");

#ifdef CRDEBUG2
  if(use_cache && McrDebug::DebugLlvmCode()) {
    bssSlot= HL->LoadGlobalBssSlot(ss.str());
    bssObj = irb->CreateBitCast(bssSlot,
        irb->getVoidPointerType()->getPointerTo());

    bssObj = HL->LoadWord<true>(bssObj, static_cast<uint32_t>(0));
    LLVM_PRINT_BSS_INFO(bssSlot, bssObj, "MISS");

#ifdef CRDEBUG4
    std::vector<Value*> args {bssSlot};
    irb->CreateCall(__VerifyBssObject(), args);
#endif
  }
#endif

  irb->CreateRet(resolved_string);

  irb->SetInsertPoint(pinsert_point);
  return func;
}

#undef LLVM_PRINT_BSS_INFO

/**
 *
 */
Function* FunctionHelper::GenerateClassInitializationCheck(
    HGraphToLLVM* HL, IRBuilder* irb) {
  if (class_init_check_ != nullptr) return class_init_check_;

  LLVMContext& ctx = irb->getContext();
  std::string name = "ClassInitCheck";
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  std::vector<Type*> argsTy{irb->getVoidPointerType()};
  FunctionType* ty = FunctionType::get(irb->getJVoidTy(), argsTy, false);

  Function* func = Function::Create(
          ty, Function::LinkOnceODRLinkage, name, irb->getModule());
  class_init_check_ = func;
  func->addFnAttr(Attribute::AlwaysInline);
  func->setDSOLocal(true);
  AddAttributesCommon(func);

  // process arguments
  Function::arg_iterator arg_iter(func->arg_begin());
  Argument* arg_class = &*arg_iter;
  arg_class->setName("class");

  BasicBlock* check_init = BasicBlock::Create(ctx, "check_inited", func);
  BasicBlock* init = BasicBlock::Create(ctx, "init", func);
  BasicBlock* ok = BasicBlock::Create(ctx, "ok", func);

  irb->SetInsertPoint(check_init);

  if (McrDebug::VerifyLoadClass()) {
    VERIFY_LLVMD4("check_init");
  }

  constexpr size_t status_lsb_position = SubtypeCheckBits::BitStructSizeOf();
  const size_t status_byte_offset =
    mirror::Class::StatusOffset().SizeValue() +
    (status_lsb_position / kBitsPerByte);
  UNUSED(status_byte_offset);
  constexpr uint32_t shifted_initialized_value =
    enum_cast<uint32_t>(ClassStatus::kInitialized) << (status_lsb_position % kBitsPerByte);

  // When using assembly (not pure_llvm) it works, as long as
  // we do an AndroidLog call..
  Value* loffset = irb->getPtrEquivInt(status_byte_offset);
  Value* status_addr = irb->CreatePtrDisp(arg_class, loffset,
      irb->getJLongTy()->getPointerTo());
  status_addr->setName("status_addr");

  // INFO semi manual
  Value* cast = irb->CreateBitCast(status_addr, irb->getVoidPointerType());
  // Value* temp = new LoadInst(irb->getJByteTy(), cast, "class_status",
  //     false/*volatile*/, Align(), AtomicOrdering::Acquire,
  //     SyncScope::System, check_init);
  // __ Ldarb(temp, HeapOperand(temp));
  Value* temp=Arm64::__Ldarb(irb, cast);
  temp = irb->CreateZExt(temp, irb->getJIntTy());

  // __ Cmp(temp, shifted_initialized_value);
  Value* init_val = irb->getJInt(shifted_initialized_value);
  Value* is_inited = irb->mCreateCmpEQ(false, temp, init_val);
  is_inited->setName("is_inited");

  irb->CreateCondBr(is_inited, ok, init);

  // Go through RT (slow path)
  irb->SetInsertPoint(init);
  LOGLLVM4(INFO, "ClassInitCheck: init: call RT");

  HL->ArtCallInitializeStaticStorage(arg_class);
  irb->CreateRetVoid();

  // already inited
  irb->SetInsertPoint(ok);
  LOGLLVM4(INFO, "ClassInitCheck: ok: already inited!");
  irb->CreateRetVoid();

  irb->SetInsertPoint(pinsert_point);
  return func;
}

/**
 * @brief  Similarly with Compare, it creates 2 variants:
 *         DivZeroCheck for ints and longs, because that's
 *         what arm code generator does.
 */
Function* FunctionHelper::DivZeroCheck(
    IRBuilder* irb, DataType::Type type) {
  Function* div_zero_check_function = GetDivZeroCheckFunction(type);
  if (div_zero_check_function != nullptr) {
    return div_zero_check_function;
  }

  D3LOG(INFO) << "Creating function: DivZeroCheck for:" << type;
  // backup insertion point (Visitors are recursive)
  BasicBlock* pinsert_point = irb->GetInsertBlock();
  div_zero_check_function = Function::Create(
      GetDivZeroCheckTy(irb, type), Function::LinkOnceODRLinkage,
      GetDivZeroCheckName(type), irb->getModule());
  // div_zero_check_function->addFnAttr(Attribute::AlwaysInline);
  div_zero_check_function->setDSOLocal(true);
  AddAttributesCheckFunction(div_zero_check_function);

  Function::arg_iterator arg_iter(div_zero_check_function->arg_begin());
  Argument* divisor = &*arg_iter++;
  Argument* dex_pc = &*arg_iter++;
  divisor->setName("divisor");
  dex_pc->setName("dex_pc");

  BasicBlock* entry_block =
      BasicBlock::Create(irb->getContext(), "entry", div_zero_check_function);
  BasicBlock* div_zero_block =
      BasicBlock::Create(irb->getContext(), "div_zero", div_zero_check_function);
  BasicBlock* div_ok_block =
      BasicBlock::Create(irb->getContext(), "div_ok", div_zero_check_function);

  // restore insertion point
  irb->SetInsertPoint(entry_block);
  Value* zero = irb->getJZero(type);
  bool is_fp = DataType::IsFloatingPointType(type);
  Value* cond_eq = irb->mCreateCmpEQ(is_fp, divisor, zero);
  irb->CreateCondBr(cond_eq, div_zero_block, div_ok_block);

  irb->SetInsertPoint(div_zero_block);
  std::vector<Value*> prt_args;
  Value* fmt =
      irb->mCreateGlobalStringPtr("Exception: DYNAMIC: Division with zero: "
                                  "DexPc: %u\n");
  prt_args.push_back(irb->AndroidLogSeverity(ERROR));
  prt_args.push_back(fmt);
  prt_args.push_back(dex_pc);
  irb->CreateCall(AndroidLog(), prt_args);
  CallExit(irb, 1);

  irb->SetInsertPoint(div_ok_block);
  irb->CreateRetVoid();

  irb->SetInsertPoint(pinsert_point);
  StoreDivZeroCheckFunction(div_zero_check_function, type);

  return div_zero_check_function;
}

Function* FunctionHelper::DivZeroFailedStatic(IRBuilder* irb) {
  if (div_zero_failed_static_!= nullptr) {
    return div_zero_failed_static_;
  }

  D3LOG(INFO) << "Creating function: DivZeroFailedStaticfor";
  // backup insertion point (Visitors are recursive)
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  std::vector<Type*> argsTy;
  argsTy.push_back(irb->getJIntTy());  // DexPC
  FunctionType *ty = FunctionType::get(irb->getJVoidTy(), argsTy, false);
  div_zero_failed_static_ = Function::Create(
      ty, Function::LinkOnceODRLinkage,
      "DivZeroFailedStatic", irb->getModule());
  div_zero_failed_static_->addFnAttr(Attribute::AlwaysInline);

  Function::arg_iterator arg_iter(div_zero_failed_static_->arg_begin());
  Argument* dex_pc = &*arg_iter++;
  dex_pc->setName("dex_pc");

  BasicBlock* entry_block =
      BasicBlock::Create(irb->getContext(), "entry", div_zero_failed_static_);

  irb->SetInsertPoint(entry_block);
  std::vector<Value*> prt_args;
  Value* fmt =
      irb->mCreateGlobalStringPtr("Exception: STATIC: Division with zero: "
                                  "DexPc: %u\n");
  prt_args.push_back(irb->AndroidLogSeverity(ERROR));
  prt_args.push_back(fmt);
  prt_args.push_back(dex_pc);
  irb->CreateCall(AndroidLog(), prt_args);
  CallExit(irb, 1);

  irb->SetInsertPoint(pinsert_point);

  return div_zero_failed_static_;
}

Function* FunctionHelper::NullCheck(IRBuilder* irb, bool make_implicit) {
  if (null_check_ != nullptr) { return null_check_; }

  D3LOG(INFO) << "Creating function: NullCheck";
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  // TODO add unwind metadata: can throw exception
  // Function type
  Type* ret_type = irb->getJVoidTy();
  std::vector<Type*> args_type;
  args_type.push_back(irb->getJObjectTy()->getPointerTo());
  args_type.push_back(irb->getJIntTy());
  FunctionType* nullCheckTy =
      FunctionType::get(ret_type, args_type, false);
  null_check_ = Function::Create(
      nullCheckTy, Function::LinkOnceODRLinkage,
      "NullCheck", irb->getModule());
  null_check_->addFnAttr(Attribute::AlwaysInline);

  Function::arg_iterator arg_iter(null_check_->arg_begin());
  Value* receiver = &*arg_iter++;
  Value* dex_pc = &*arg_iter++;
  receiver->setName("receiver");
  dex_pc->setName("dex_pc");

  BasicBlock* entry_block =
      BasicBlock::Create(irb->getContext(), "entry", null_check_);
  irb->SetInsertPoint(entry_block);

  // the normal implicit check: it doesn't check whether the address is null, and
  // continues w/ with the next operation that uses it. If it's null, then a SEGV
  // will be thrown, and LLVM will handle it and jump to the null_check_failed branch.
  // We implemented this approach, however, we need llvm >= 3.8, otherwise llc will
  // ignore it. We compiled from Nougat the llvm 3.8.x, but the device for some reason
  // bootloops.
  // We can still fake it though. We won't do any check at all, so we will match the
  // performance w/ implicit of llvm3.8 and ART's Optimizing, and we don't worry
  // about catch blocks, as in the Marshmallow version the HGraph is not even produced
  // when there are any try blocks. If an error occurs, it will be unhandled anyway, so
  // replay will fail. REVISE if we update to 7.9, we will use llvm 3.8 and ignore this
  if (McrDebug::ImplicitNullChecks()) {
    irb->CreateRetVoid();
  } else {
    BasicBlock* null_check_failed =
        BasicBlock::Create(irb->getContext(), "null_check_failed", null_check_);
    BasicBlock* null_check_passed =
        BasicBlock::Create(irb->getContext(), "null_check_passed", null_check_);

    if (make_implicit) TODO_LLVM("Implicit");

    Value* cmp = irb->CreateCmpIsNull(receiver);
    BranchInst* cond = irb->CreateCondBr(cmp, null_check_failed, null_check_passed);

    if (make_implicit) {
      // INFO in llvm10 seems to produce an identical binary with this enabled
      MDNode* md_impl_null=MDNode::get(irb->getContext(),
          MDString::get(irb->getContext(), ""));
      cond->setMetadata("make.implicit", md_impl_null);
    }

    irb->SetInsertPoint(null_check_failed);
    // print error msg
    std::vector<Value*> prt_args;
    Value* fmt =
        irb->mCreateGlobalStringPtr("NullPointerException: DexPC: %u");
    prt_args.push_back(irb->AndroidLogSeverity(ERROR));
    prt_args.push_back(fmt);
    prt_args.push_back(dex_pc);
    irb->CreateCall(AndroidLog(), prt_args);

    CallExit(irb, 1);

    irb->SetInsertPoint(null_check_passed);
    irb->CreateRetVoid();
  }

  // restore insertion point
  irb->SetInsertPoint(pinsert_point);
  return null_check_;
}

Function* FunctionHelper::BoundsCheck(IRBuilder* irb, HGraphToLLVM* HL) {
  if (bounds_check_ != nullptr) {
    return bounds_check_;
  }

  D3LOG(INFO) << "Creating function: BoundsCheck";
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  // TODO add unwind metdata: can throw exception
  // Function type
  Type* ret_type = irb->getJVoidTy();
  std::vector<Type*> args_type;
  args_type.push_back(irb->getJIntTy());
  args_type.push_back(irb->getJIntTy());
  args_type.push_back(irb->getJIntTy());
  FunctionType* boundsCheckTy =
      FunctionType::get(ret_type, args_type, false);
  bounds_check_ = Function::Create(
      boundsCheckTy, Function::LinkOnceODRLinkage,
      "BoundsCheck", irb->getModule());
  bounds_check_->addFnAttr(Attribute::AlwaysInline);
  bounds_check_->setDSOLocal(true);
  AddAttributesCheckFunction(bounds_check_);

  Function::arg_iterator arg_iter(bounds_check_->arg_begin());
  Value* index = &*arg_iter++;
  Value* up_bound = &*arg_iter++;
  Value* dex_pc = &*arg_iter++;
  index->setName("index");
  up_bound->setName("up_bound");
  dex_pc->setName("dex_pc");

  BasicBlock* entry_block =
      BasicBlock::Create(irb->getContext(), "entry", bounds_check_);
  BasicBlock* bounds_check_failed =
      BasicBlock::Create(irb->getContext(),
                                 "bounds_check_failed", bounds_check_);
  BasicBlock* bounds_check_passed =
      BasicBlock::Create(irb->getContext(),
                                 "bounds_check_passed", bounds_check_);

  irb->SetInsertPoint(entry_block);
// #define BOUNDS_CHECK_SLOW
#ifdef BOUNDS_CHECK_SLOW
  Value* low_bound = irb->getJZero(DataType::Type::kInt32);
  low_bound->setName("low_bound");
  Value* lhs = irb->mCreateSub(false, index, low_bound);
  Value* rhs = irb->mCreateSub(false, up_bound, low_bound);

  // INFO unsigned GE: overflow trick for faster bounds check calculation
  Value* cmp = irb->mCreateCmpGE(false, false, lhs, rhs);
  irb->CreateCondBr(cmp, bounds_check_failed, bounds_check_passed);
#else
  Value* cmp = irb->mCreateCmpLT(false, false, index, up_bound);
  MDNode *Nweight=HL->MDB()->createBranchWeights(MAX_BRWEIGHT, 0);
  irb->CreateCondBr(cmp, bounds_check_passed, bounds_check_failed, Nweight);
#endif
  irb->SetInsertPoint(bounds_check_failed);
  // print error msg
  std::vector<Value*> prt_args;
  Value* fmt =
      irb->mCreateGlobalStringPtr("java.lang.ArrayIndexOutOfBoundsException: "
                                  "length=%d; index=%d DexPc: %u\n");
  prt_args.push_back(irb->AndroidLogSeverity(ERROR));
  prt_args.push_back(fmt);
  prt_args.push_back(up_bound);
  prt_args.push_back(index);
  prt_args.push_back(dex_pc);
  irb->CreateCall(AndroidLog(), prt_args);
  CallExit(irb, 1);

  irb->SetInsertPoint(bounds_check_passed);
  irb->CreateRetVoid();

  irb->SetInsertPoint(pinsert_point);

  MDNode* N=MDNode::get(irb->getContext(), MDString::get(irb->getContext(),
        "Android Unroll optimization: BoundsCheck"));
  bounds_check_->setMetadata("android.check.bounds", N);

  return bounds_check_;
}

void FunctionHelper::AddAttributesCheckFunction(Function* F) {
  F->addFnAttr(Attribute::AlwaysInline);

  F->addFnAttr(Attribute::Naked);
  F->addFnAttr(Attribute::NoFree);
  F->addFnAttr(Attribute::NoRedZone);
  F->addFnAttr(Attribute::NoCfCheck);
  F->addFnAttr(Attribute::NoSync);
}

void FunctionHelper::AddAttributesCommon(Function* F) {
  F->addFnAttr(Attribute::AlwaysInline);

  F->addFnAttr(Attribute::Naked);
  F->addFnAttr(Attribute::NoFree);
  F->addFnAttr(Attribute::NoRedZone);
  F->addFnAttr(Attribute::WillReturn);
  F->addFnAttr(Attribute::NoSync);
  F->addFnAttr(Attribute::NoUnwind);
  F->addFnAttr(Attribute::NoCfCheck);
}

void FunctionHelper::AddAttributesFastASM(Function* F) {
  F->addFnAttr(Attribute::AlwaysInline);
  F->addFnAttr(Attribute::ReadNone);

  F->addFnAttr(Attribute::Naked);
  F->addFnAttr(Attribute::NoFree);
  F->addFnAttr(Attribute::NoRedZone);
  F->addFnAttr(Attribute::NoRecurse);
  F->addFnAttr(Attribute::WillReturn);
  F->addFnAttr(Attribute::NoSync);
  F->addFnAttr(Attribute::NoUnwind);
  F->addFnAttr(Attribute::NoCfCheck);
}

void FunctionHelper::AddAttributesSuspendCheck(Function* F) {
  F->addFnAttr(Attribute::AlwaysInline);
  F->addFnAttr(Attribute::ReadNone);

  // CHECK THESE:
  // F->addFnAttr(Attribute::Naked);
  // F->addFnAttr(Attribute::NoFree);
  // F->addFnAttr(Attribute::NoRedZone);
  // F->addFnAttr(Attribute::NoRecurse);
  // F->addFnAttr(Attribute::WillReturn);
  // F->addFnAttr(Attribute::NoSync);
  // F->addFnAttr(Attribute::NoCfCheck);
  // F->addFnAttr(Attribute::NoUnwind);
}

/**
 * @brief INFO do not use `casted_set_val` (that is use the i8*,
 *        instead of the i64 for objects in arm64).
 *
 *        See an example in ArraySetBarrier.
 */
Function* FunctionHelper::MarkGCCard(
    IRBuilder* irb, DataType::Type type, bool value_can_be_null) {
  TODO_LLVMD("Make it with LLVM");

  BasicBlock* pinsert_point = irb->GetInsertBlock();
  std::string name = "MarkGCCard";
  { std::stringstream ss; ss << type; name+=ss.str(); }
  if(value_can_be_null) { name+="CanBeNull"; }
  if (mark_gc_card_[name] != nullptr) return mark_gc_card_[name];
  D3LOG(INFO) << "Creating function: " << name;

  std::vector<Type*> argsTy;
  argsTy.push_back(irb->getJObjectTy()->getPointerTo());
  argsTy.push_back(irb->getType(type));
  FunctionType* fTy =
    FunctionType::get(irb->getJVoidTy(), argsTy, false);
  Function *func = Function::Create(
      fTy, Function::LinkOnceODRLinkage,
      name, irb->getModule());
  func->addFnAttr(Attribute::AlwaysInline);
  mark_gc_card_[name] = func;
  func->setDSOLocal(true);
  AddAttributesFastASM(func);

  Function::arg_iterator arg_iter(func->arg_begin());
  Argument* object = &*arg_iter++;
  Argument* value = &*arg_iter++;
  object->setName("object");
  value->setName("value");

  BasicBlock* entry_block =
    BasicBlock::Create(irb->getContext(), "entry", func);
  BasicBlock* mark = BasicBlock::Create(irb->getContext(), "mark", func);
  BasicBlock* done = BasicBlock::Create(irb->getContext(), "done", func);

  irb->SetInsertPoint(entry_block);
  Value* is_zero = irb->mCreateCmpEQ(false, value, irb->getJZero(type));
  is_zero->setName("is_zero");
  irb->CreateCondBr(is_zero, done, mark);

  irb->SetInsertPoint(mark);
  VERIFY_LLVMD4("MarkGCCard: marking..");

  /* Example:
   *
   * ldr x16, [tr, #152] ; card_table
   * lsr w17, w23, #10
   * strb w16, [x16, x17]
   */
  // Load the address of the card table into `card`.
  uint32_t offset = Thread::CardTableOffset<kArm64PointerSize>().Int32Value();
  Value* card = Arm64::Ldr(irb, LLVM_THREAD_REG_ARM64, offset);
  card->setName("card");

  // TODO in LLVM: shift left on obj
  // then strb x1, [x2, x3]
  // `object`.
  Value* temp = Arm64::wLsr(irb, object,
      gc::accounting::CardTable::kCardShift);
  // Write the `art::gc::accounting::CardTable::kCardDirty` value into the
  // `object`'s card.
  //
  // Register `card` contains the address of the card table. Note that the card
  // table's base is biased during its creation so that it always starts at an
  // address whose least-significant byte is equal to `kCardDirty` (see
  // art::gc::accounting::CardTable::Create). Therefore the STRB instruction
  // below writes the `kCardDirty` (byte) value into the `object`'s card
  // (located at `card + object >> kCardShift`).
  //
  // This dual use of the value in register `card` (1. to calculate the location
  // of the card to mark; and 2. to load the `kCardDirty` value) saves a load
  // (no need to explicitly load `kCardDirty` as an immediate value).
  // __ Strb(card, MemOperand(card, temp.X()));
  Arm64::wStrb(irb, card, card, temp);
  irb->CreateRetVoid();

  irb->SetInsertPoint(done);
  irb->CreateRetVoid();

  irb->SetInsertPoint(pinsert_point);

  return func;
}

FunctionType* FunctionHelper::GetInnerFunctionType(
    const DexCompilationUnit& dex_cu, IRBuilder* irb) {
  D4LOG(INFO) << __func__;
  uint32_t shorty_len;
  const char* shorty = dex_cu.GetShorty(&shorty_len);
  return GetFunctionType(
      shorty, shorty_len, dex_cu.IsStatic(), dex_cu.IsNative(), irb);
}

/**
 * @brief Used for the inner hf, and all other hfs that might be dependencies
 *
 */
FunctionType* FunctionHelper::GetFunctionType(
    const char* shorty, uint32_t shorty_len,
    bool is_static, bool is_native, IRBuilder* irb) {
  D4LOG(INFO) << __func__;

  Type* ret_type = irb->getTypeFromShorty(shorty[0]);
  std::vector<Type*> args_type;

  if (is_native) {
    args_type.push_back(irb->getJEnvTy());
  }

  // receiver, or class pointer in static jni calls
  if (!is_static || is_native) {
    Type* ty = nullptr;

    if (is_native) {
      ty = irb->getJniObjectTy();
    } else {
      ty = irb->getJObjectTy()->getPointerTo();
    }

    // INFO will send just object
    args_type.push_back(ty);
  }

  for (uint32_t i = 1; i < shorty_len; ++i) {  // arguments
    Type* arg_type = nullptr;
    if (is_native) {
      arg_type = irb->getTypeFromShortyJNI(shorty[i]);
    } else {
      arg_type = irb->getTypeFromShorty(shorty[i]);
    }
    args_type.push_back(arg_type);
  }
  return FunctionType::get(ret_type, args_type, false);
}

void FunctionHelper::VerifySpeculation(
    LogSeverity severity,
    IRBuilder* irb, Value* lclass_idx,
    std::string spec_msg, std::string pretty_method, bool die) {
  std::vector<Value*> prt_args;
  Value* fmt =
      irb->mCreateGlobalStringPtr("spec-" + spec_msg + ": %u/%s\n");
  prt_args.push_back(irb->AndroidLogSeverity(severity));
  prt_args.push_back(fmt);
  // prt_args.push_back(irb->getJUnsignedInt(class_idx));
  prt_args.push_back(lclass_idx);
  prt_args.push_back(irb->mCreateGlobalStringPtr(pretty_method));
  irb->CreateCall(AndroidLog(), prt_args);

  // CHECK (process dies anyway?)
  if (McrDebug::DieOnSpeculationMiss()) {
    irb->FH()->CallExit(irb, 1);
  }
}

void FunctionHelper::VerifySpeculation(
    LogSeverity severity,
    IRBuilder* irb, uint32_t class_idx,
    std::string spec_msg, std::string pretty_method, bool die) {
  std::vector<Value*> prt_args;
  Value* fmt =
      irb->mCreateGlobalStringPtr("spec-" + spec_msg + ": %u/%s\n");
  prt_args.push_back(irb->AndroidLogSeverity(severity));
  prt_args.push_back(fmt);
  prt_args.push_back(irb->getJUnsignedInt(class_idx));
  prt_args.push_back(irb->mCreateGlobalStringPtr(pretty_method));
  irb->CreateCall(AndroidLog(), prt_args);

  // CHECK (process dies anyway?)
  if (McrDebug::DieOnSpeculationMiss()) {
    irb->FH()->CallExit(irb, 1);
  }
}


Value* FunctionHelper::InvokeThroughRT_SLOW(
    HGraphToLLVM* HL, IRBuilder* irb, Value* art_method,
    Value* receiver, bool is_native,
    std::vector<Value*> callee_args, DataType::Type ret_type,
    std::string spec_method_name, std::string call_info) {
  DIE;
  AllocaInst* jvalue = irb->CreateAlloca(irb->getJValueTy());
  jvalue->setUsedWithInAlloca(true);  // otherwise opt tool messes things up!
  jvalue->setName("jvalue");

  // invoke through RT
  std::vector<Value*> args_invoke_through_rt;
  args_invoke_through_rt.push_back(art_method);
  args_invoke_through_rt.push_back(receiver);
  args_invoke_through_rt.push_back(jvalue);
  args_invoke_through_rt.insert(
      args_invoke_through_rt.end(),
      callee_args.begin(), callee_args.end());

  if (McrDebug::VerifyInvokeQuickThourghRT_SLOW()) {
    irb->AndroidLogPrint(INFO, "InvokeThroughRT_SLOW:" + call_info + ": " + spec_method_name + " \n");
  }

  if (!is_native) {
    irb->CreateCall(__InvokeMethodSLOW(), args_invoke_through_rt);
  } else {
    irb->CreateCall(__InvokeJniMethod(), args_invoke_through_rt);
  }

  Value* result = nullptr;
  if (ret_type != DataType::Type::kVoid) {
    result = irb->CallGetReturnValue(HL, jvalue, ret_type);
    result->setName("result");
  }
  return result;
}

void FunctionHelper::DebugInvoke(
    HGraphToLLVM* HL, IRBuilder* irb, Value* art_method,
    Value* receiver, bool is_native,
    std::vector<Value*> callee_args, DataType::Type ret_type) {
  CHECK(!is_native) << "should't be JNI method";
  UNUSED(ret_type);
  AllocaInst* jvalue =
      irb->CreateAlloca(irb->getJValueTy());
  jvalue->setUsedWithInAlloca(true);  // otherwise opt tool messes things up!
  jvalue->setName("jvalue");
  // invoke through RT
  std::vector<Value*> args_invoke_through_rt;
  args_invoke_through_rt.push_back(art_method);
  args_invoke_through_rt.push_back(receiver);
  args_invoke_through_rt.push_back(jvalue);
  args_invoke_through_rt.insert(
      args_invoke_through_rt.end(),
      callee_args.begin(), callee_args.end());

  irb->CreateCall(__DebugInvoke(), args_invoke_through_rt);
}

FunctionType* FunctionHelper::GetInitInnerFromInitTy(IRBuilder* irb) {
  std::vector<Type*> params{irb->getVoidPointerType(),
    irb->getInt32Ty(), irb->getInt32Ty()};

  return FunctionType::get(irb->getJVoidTy(), params, false);
}

Function* FunctionHelper::exit(IRBuilder *irb) {

  std::vector<Type*> params{irb->getInt32Ty()};
  FunctionType* exit_type =
    FunctionType::get(irb->getJVoidTy(), params, false);

  return cast<Function>(
      mod_->getOrInsertFunction("exit", exit_type).getCallee());
}

FunctionType* FunctionHelper::GetInitInnerFromIchfTy(IRBuilder* irb) {
    std::vector<Type*> params{irb->getVoidPointerType()};
    return FunctionType::get(irb->getJVoidTy(), params, false);
}

void FunctionHelper::CallExit(IRBuilder* irb, int exit_code, bool emit_unreachable) {
  std::vector<Value*> exit_args;
  exit_args.push_back(irb->getInt32(exit_code));
  irb->CreateCall(exit(irb), exit_args);
  if (emit_unreachable) {
    irb->CreateUnreachable();
  }
}

Function* FunctionHelper::printf(IRBuilder* irb) {
  std::vector<Type*> params{irb->getJCharTy()->getPointerTo()};
  FunctionType* printf_type =
    FunctionType::get(irb->getInt32Ty(), params, true);

  AttributeList attributes;
  {
    AttrBuilder B;
    B.addAttribute(Attribute::NoAlias);
    attributes = AttributeList::get(*ctx_, 0U, B);
  }

  return cast<Function>(mod_->getOrInsertFunction(
        "printf", printf_type, attributes).getCallee());
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
