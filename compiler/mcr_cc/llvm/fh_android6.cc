/**
 * Code that was specific to Android6.
 *
 * Copyright (C) 2018  Paschalis Mpeis (paschalis.mpeis-AT-gmail.com)
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

using namespace ::llvm;

namespace art {
namespace LLVM {

#ifdef ART_MCR_ANDROID_6
Function* FunctionHelper::GenerateClassInitializationCheck(HGraphToLLVM* HL, IRBuilder* irb) {
  LLVMContext& ctx = irb->getContext();
  std::string name = "ClassInitCheck";

  if (class_init_check_ != nullptr) {
    return class_init_check_;
  }

  BasicBlock* pinsert_point = irb->GetInsertBlock();

  std::vector<Type*> argsTy;
  argsTy.push_back(irb->getVoidPointerType());
  FunctionType* ty =
      FunctionType::get(irb->getJVoidTy(), argsTy, false);

  Function* func =
      Function::Create(
          ty, Function::LinkOnceODRLinkage,
          name, irb->getModule());
  class_init_check_ = func;
  func->addFnAttr(Attribute::AlwaysInline);

  // process arguments
  Function::arg_iterator arg_iter(func->arg_begin());
  Argument* arg_class = &*arg_iter;
  arg_class->setName("class");

  // create BBs
  BasicBlock* check_init = BasicBlock::Create(ctx, "check_inited", func);
  BasicBlock* init = BasicBlock::Create(ctx, "init", func);
  BasicBlock* ok = BasicBlock::Create(ctx, "ok", func);

  irb->SetInsertPoint(check_init);
  if (McrDebug::VerifyLoadClass()) {
    irb->AndroidLogPrint(ERROR, "Check ClassInit");
  }
  Value* class_status =
      HL->LoadFromObjectOffset(arg_class,
                               mirror::Class::StatusOffset().Int32Value(),
                               irb->getJIntTy());
  class_status->setName("class_status");

  Value* is_inited = irb->mCreateCmpEQ(
      false, class_status, irb->getJInt(
        static_cast<uint32_t>(ClassStatus::kInitialized)));

  irb->CreateCondBr(is_inited, ok, init);

  // Go through RT (slow path)
  irb->SetInsertPoint(init);
  std::vector<Value*> args;
  args.push_back(arg_class);
  irb->CreateCall(__ClassInit(), args);
  irb->CreateRetVoid();

  // already inited
  irb->SetInsertPoint(ok);
  irb->CreateRetVoid();

  irb->SetInsertPoint(pinsert_point);
  return func;
}

Function* FunctionHelper::LoadClass(
    HGraphToLLVM* HL, IRBuilder* irb,
    uint32_t caller_didx, uint32_t class_idx,
    bool clinit, bool access_check) {
  LLVMContext& ctx = irb->getContext();
  std::string name = "LoadClass" + std::to_string(caller_didx) +
                     "_" + std::to_string(class_idx);

  if (load_class_[name] != nullptr) {
    return load_class_[name];
  }

  BasicBlock* pinsert_point = irb->GetInsertBlock();

  std::vector<Type*> argsTy;
  argsTy.push_back(irb->getVoidPointerType());
  FunctionType* ty =
      FunctionType::get(irb->getVoidPointerType(), argsTy, false);

  Function* func =
      Function::Create(
          ty, Function::LinkOnceODRLinkage,
          name, irb->getModule());
  load_class_[name] = func;
  func->addFnAttr(Attribute::AlwaysInline);

  // process arguments
  Function::arg_iterator arg_iter(func->arg_begin());
  Argument* arg_art_method = &*arg_iter;
  arg_art_method->setName("callee_art_method");

  // create BBs
  BasicBlock* try_dcache = BasicBlock::Create(ctx, "try_dcache", func);
  BasicBlock* miss = BasicBlock::Create(ctx, "miss", func);
  BasicBlock* hit = BasicBlock::Create(ctx, "hit", func);

  irb->SetInsertPoint(try_dcache);
  Value* cached_class;

  if (McrDebug::VerifyLoadClass()) {
    irb->AndroidLogPrint(ERROR, "LoadClass: dcache");
  }

  Value* dex_cache_resolved_types =
      HL->LoadFromObjectOffset(arg_art_method,
                               ArtMethod::DexCacheResolvedTypesOffset().Int32Value(),
                               irb->getVoidPointerType());
  dex_cache_resolved_types->setName("dex_cache_resolved_types");

  cached_class =
      HL->LoadFromObjectOffset(dex_cache_resolved_types,
                               CodeGenerator::GetCacheOffset(class_idx),
                               irb->getVoidPointerType());
  cached_class->setName("cached_class");

  Value* is_null = irb->CreateCmpIsNull(cached_class);
  irb->CreateCondBr(is_null, miss, hit);

  // Go through RT (slow path)
  irb->SetInsertPoint(miss);
  if (McrDebug::VerifyLoadClass()) {
    irb->AndroidLogPrint(ERROR, "LoadClass: cache miss");
  }

  Value* current_thread = HL->GetIntrinsicHelper()->LoadThreadCurrentThread(HL);
  current_thread->setName("thread");

  std::vector<Value*> args;
  args.push_back(arg_art_method);
  args.push_back(irb->getJInt(class_idx));
  args.push_back(current_thread);

  Function* func_resolve_class;
  if (McrDebug::VerifyLoadClass()) {
    irb->AndroidLogPrint(ERROR, "LoadClass: calling through RT");
  }

  if (do_clinit_) {
    func_resolve_class = __InitializeStaticStorageFromCode();
  } else {
    func_resolve_class = __InitializeTypeFromCode();
  }

    Value* resolved_class = irb->CreateCall(func_resolve_class, args);
  resolved_class->setName("resolved_class");

  if (McrDebug::VerifyLoadClass()) {
    irb->AndroidLogPrint(ERROR, "LoadClass: resolved. returning");
    if (McrDebug::VerifyArtClass()) {
      HL->ArtCallVerifyArtClass(resolved_class);
    }
  }
  irb->CreateRet(resolved_class);

  irb->SetInsertPoint(hit);
  if (McrDebug::VerifyLoadClass()) {
    irb->AndroidLogPrint(ERROR, "LoadClass: hit: " + std::to_string(class_idx));
  }
  irb->CreateRet(cached_class);


  irb->SetInsertPoint(pinsert_point);
  return func;
}

Function* FunctionHelper::InstanceOf(
    IRBuilder* irb, HGraphToLLVM* HL,
    bool null_check, bool class_final, uint16_t type_idx) {
  std::string name = "InstanceOf_" + std::to_string(type_idx);

  if (null_check) {
    name += "_NullCheck";
  }

  if (class_final) {
    name += "_ClassFinal";
  }

  if (instance_of_.find(name) != instance_of_.end()) {
    return instance_of_[name];
  }

  D3LOG(INFO) << "Creating function: " + name;
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  std::vector<Type*> args_type;
  args_type.push_back(irb->getVoidPointerType());
  if (!class_final) {
    args_type.push_back(irb->getVoidPointerType());
  }
  Type* retTy = irb->getType(
      DataType::Type::kBool);
  FunctionType* ty =
      FunctionType::get(retTy, args_type, false);

  Function* f = Function::Create(
      ty, Function::LinkOnceODRLinkage,
      name, irb->getModule());

  Function::arg_iterator arg_iter(f->arg_begin());
  Argument* lobj = &*arg_iter++;
  Argument* lclass = nullptr;
  lobj->setName("obj");
  if (!class_final) {
    lclass = &*arg_iter++;
    lclass->setName("cls");
  }

  BasicBlock* entry_block = BasicBlock::Create(irb->getContext(),
                                                               "entry", f);
  irb->SetInsertPoint(entry_block);

  // TODO the null check here..
  if (null_check) {
    Value* eq_null = irb->mCreateCmpEQ(false, lobj, irb->getJNull());
    BasicBlock* is_null = BasicBlock::Create(
        *ctx_, "is_null", f);
    BasicBlock* not_null = BasicBlock::Create(
        *ctx_, "not_null", f);
    irb->CreateCondBr(eq_null, is_null, not_null);

    // Return 0 if `obj` is null.
    irb->SetInsertPoint(is_null);
    irb->CreateRet(irb->getJBoolean(false));

    // if not null, continue!
    irb->SetInsertPoint(not_null);
  }

  Value* lobj_cls = HL->GetObjectClass(lobj);
  lobj_cls->setName("obj_cls");

  // Compare the class of `obj` with `cls`.
  Value* lobj_cls_idx =
      HL->GetClassTypeIdx(lobj_cls);
  lobj_cls_idx->setName("obj_cls_idx");

  BasicBlock* cls_same = BasicBlock::Create(*ctx_, "cls_same", f);
  BasicBlock* cls_diff =
      BasicBlock::Create(*ctx_, "cls_diff", f);

  Value* cls_match =
      irb->mCreateCmpEQ(false, lobj_cls_idx, irb->getJUnsignedInt(type_idx));

  irb->CreateCondBr(cls_match, cls_same, cls_diff);
  irb->SetInsertPoint(cls_same);
  irb->CreateRet(irb->getJBoolean(true));
  irb->SetInsertPoint(cls_diff);

  if (class_final) {
    irb->CreateRet(irb->getJBoolean(false));
  } else {  // If the classes are not equal, we go into a slow path.
    std::vector<Value*> args_rt;
    args_rt.push_back(lobj_cls);
    args_rt.push_back(lclass);
    Value* res = irb->CreateCall(__InstanceOf(), args_rt);

    // CHECK_LLVM that this casting here is correct
    // (to possibly usigned due to being bool)
    // it must be ok (old IsSigned did not include kPrimBoolean
    Value* casted_res =
        irb->CreateIntCast(res, retTy, 
            irb->IsSigned(DataType::Type::kBool));

    irb->CreateRet(casted_res);
  }

  instance_of_[name] = f;
  irb->SetInsertPoint(pinsert_point);
  return f;
}

#endif
}  // namespace LLVM
}  // namespace art
