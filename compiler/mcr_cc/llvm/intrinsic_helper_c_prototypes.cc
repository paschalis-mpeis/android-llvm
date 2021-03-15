/**
 * These are not intrinsics supported officially in LLVM IR, but
 * are instead ones that LLVM can call from other C libraries it
 * links against to. Still, these are faster than relying to the runtime.
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

#include "ir_builder.h"

using namespace ::llvm;
namespace art {
namespace LLVM {
#define mod mod_

Function* IntrinsicHelper::MathAbsDouble() {
  Function* f = mod_->getFunction("llvm.fabs.f64");
  if (!f) {
    Type* doubleTy =
        Type::getDoubleTy(mod_->getContext());
    std::vector<Type*> args;
    args.push_back(doubleTy);
    FunctionType* fTy = FunctionType::get(
        doubleTy, args, false);

    f = Function::Create(fTy, GlobalValue::ExternalLinkage,
                                 "llvm.fabs.f64", mod_);
    f->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwindReadNone(f);
  return f;
}

Function* IntrinsicHelper::MathAbsFloat() {
  Function* f = mod_->getFunction("llvm.fabs.f32");
  if (!f) {
    std::vector<Type*> args;
    Type* floatTy =
        Type::getFloatTy(mod_->getContext());
    args.push_back(floatTy);
    FunctionType* fTy = FunctionType::get(
        floatTy, args, false);

    f = Function::Create(
        fTy, GlobalValue::ExternalLinkage,
        "llvm.fabs.f32", mod_);
    f->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwindReadNone(f);
  return f;
}

void IntrinsicHelper::SetAttributesNoUnwindReadNone(Function* f) {
  AttrBuilder B;
  B.addAttribute(Attribute::NoUnwind);
  B.addAttribute(Attribute::ReadNone);
  f->setAttributes(AttributeList::get(mod_->getContext(), ~0U, B));
}


void IntrinsicHelper::SetAttributesNoUnwind(Function* f) {
  AttrBuilder B;
  B.addAttribute(Attribute::NoUnwind);
  f->setAttributes(AttributeList::get(mod_->getContext(), ~0U, B));
}

Function* IntrinsicHelper::C_MathRoundFloat() {
  Function* func_roundf = mod_->getFunction("roundf");
  Type* floatTy = Type::getFloatTy(mod_->getContext());

  if (!func_roundf) {
    std::vector<Type*> args;
    args.push_back(floatTy);

    FunctionType* funcTy =
        FunctionType::get(floatTy, args, false);

    func_roundf = Function::Create(
        funcTy,
        GlobalValue::ExternalLinkage,
        "roundf", mod_);
    func_roundf->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwindReadNone(func_roundf);
  return func_roundf;
}

Function* IntrinsicHelper::C_MathRoundDouble() {
  Function* func_round = mod_->getFunction("round");

  if (!func_round) {
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    std::vector<Type*> args;
    args.push_back(doubleTy);
    FunctionType* funcTy = FunctionType::get(
        doubleTy, args, false);

    func_round = Function::Create(
        funcTy, GlobalValue::ExternalLinkage,
        "round", mod_);
    func_round->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwindReadNone(func_round);
  return func_round;
}

Function* IntrinsicHelper::MathCeil() {
  Function* func_ = mod_->getFunction("ceil");

  if (!func_) {
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    std::vector<Type*> args;
    args.push_back(doubleTy);
    FunctionType* funcTy = FunctionType::get(
        doubleTy, args, false);

    func_ = Function::Create(
        funcTy, GlobalValue::ExternalLinkage,
        "ceil", mod_);
    func_->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwindReadNone(func_);
  return func_;
}

Function* IntrinsicHelper::MathFloor() {
  Function* func_ = mod_->getFunction("floor");

  if (!func_) {
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    std::vector<Type*> args;
    args.push_back(doubleTy);
    FunctionType* funcTy = FunctionType::get(
        doubleTy, args, false);

    func_ = Function::Create(
        funcTy, GlobalValue::ExternalLinkage,
        "floor", mod_);
    func_->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwindReadNone(func_);
  return func_;
}

Function* IntrinsicHelper::MathSin() {
  Function* func_sin = mod_->getFunction("sin");

  if (!func_sin) {
    std::vector<Type*> args;
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    args.push_back(doubleTy);
    FunctionType* funcTy = FunctionType::get(
        doubleTy, args, false);

    func_sin = Function::Create(
        funcTy,
        GlobalValue::ExternalLinkage,
        "sin", mod_);
    func_sin->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwind(func_sin);
  return func_sin;
}

Function* IntrinsicHelper::MathPow() {
  Function* func_pow = mod_->getFunction("pow");

  if (!func_pow) {
    std::vector<Type*> args;
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    args.push_back(doubleTy);
    args.push_back(doubleTy);
    FunctionType* funcTy = FunctionType::get(
        doubleTy, args, false);

    func_pow = Function::Create(
        funcTy,
        GlobalValue::ExternalLinkage,
        "pow", mod_);
    func_pow->setCallingConv(CallingConv::C);
  }

  SetAttributesNoUnwind(func_pow);
  return func_pow;
}

Function* IntrinsicHelper::MathTan() {
  Function* F= mod_->getFunction("tan");
  if (!F) {
    std::vector<Type*> args;
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    args.push_back(doubleTy);
    FunctionType* fTy= FunctionType::get(doubleTy, args, false);
    F=Function::Create(fTy,GlobalValue::ExternalLinkage,"tan", mod_);
    F->setCallingConv(CallingConv::C);
  }
  SetAttributesNoUnwind(F);
  return F;
}

Function* IntrinsicHelper::isinf(bool is64) {
  // USE GenIsInfinite
  // DOES NOT WORK
  Function* F= mod_->getFunction("isinf");
  if (!F) {
    Type* ty;
    Type* retTy;
    if(is64) {
      ty=Type::getDoubleTy(mod_->getContext());
    } else {
      ty=Type::getFloatTy(mod_->getContext());
    }
    retTy=Type::getInt32Ty(mod_->getContext());
    std::vector<Type*> args{ty};
    FunctionType* fTy= FunctionType::get(retTy, args, false);
    F=Function::Create(fTy,GlobalValue::ExternalLinkage,"isinf", mod_);
    F->setCallingConv(CallingConv::C);
  }
  SetAttributesNoUnwind(F);
  return F;
}

Function* IntrinsicHelper::MathATan2() {
  Function* F= mod_->getFunction("atan2");
  if (!F) {
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    std::vector<Type*> args {2, doubleTy};
    FunctionType* fTy= FunctionType::get(doubleTy, args, false);
    F=Function::Create(fTy,GlobalValue::ExternalLinkage,"atan2", mod_);
    F->setCallingConv(CallingConv::C);
  }
  SetAttributesNoUnwind(F);
  return F;
}

Function* IntrinsicHelper::MathAsin() {
  const char* name="asin";
  Function* F= mod_->getFunction(name);
  if (!F) {
    Type* doubleTy = Type::getDoubleTy(mod_->getContext());
    std::vector<Type*> args{doubleTy};
    FunctionType* fTy= FunctionType::get(doubleTy, args, false);
    F=Function::Create(fTy,GlobalValue::ExternalLinkage, name, mod_);
    F->setCallingConv(CallingConv::C);
  }
  SetAttributesNoUnwind(F);
  return F;
}

Function* IntrinsicHelper::MathSqrtDouble() {
  DIE_ANDROID10();
  Function* func_sqrt = mod->getFunction("sqrt");
  if (!func_sqrt) {
    std::vector<Type*> args;
    args.push_back(Type::getDoubleTy(mod->getContext()));
    FunctionType* FuncTy_0 = FunctionType::get(
        Type::getDoubleTy(mod->getContext()),
        args, false);

    func_sqrt = Function::Create(
        /*Type=*/FuncTy_0,
        /*Linkage=*/GlobalValue::ExternalLinkage,
        /*Name=*/"sqrt", mod);  // (external, no body)
    func_sqrt->setCallingConv(CallingConv::C);
  }

  AttributeList func_sqrt_PAL;
  {
    SmallVector<AttributeList, 4> Attrs;
    AttributeList PAS;
    {
      AttrBuilder B;
      B.addAttribute(Attribute::NoUnwind);
      PAS = AttributeList::get(mod->getContext(), ~0U, B);
      // AttributeList::get(mod_->getContext(), ~0U, B)
    }

    Attrs.push_back(PAS);
    func_sqrt_PAL = AttributeList::get(mod->getContext(), Attrs);
  }
  func_sqrt->setAttributes(func_sqrt_PAL);
  return func_sqrt;
}

#undef mod
// TODO replace all mod_

}  // namespace LLVM
}  // namespace art
