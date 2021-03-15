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
#ifdef ART_MCR_ANDROID_6
void HGraphToLLVM::VisitLoadClass(HLoadClass* cls) {
  Value* art_method = irb_->CreateLoad(gbl_art_method_);
  Value* loaded_class = nullptr;

  if (cls->IsReferrersClass()) {
    DCHECK(!cls->CanCallRuntime());
    DCHECK(!cls->MustGenerateClinitCheck());
    loaded_class = GetArtMethodClass(art_method);
  } else {
    DCHECK(cls->CanCallRuntime());

#ifdef USE_SLOW_LOAD_CLASS
    std::vector<Value*> args_resolve_class;
    args_resolve_class.push_back(art_method);
    args_resolve_class.push_back(
        irb_->getJUnsignedInt16(cls->GetTypeIndex()));
    args_resolve_class.push_back(
        irb_->getCBool(cls->MustGenerateClinitCheck()));
    loaded_class = irb_->CreateCall(
        fh_->__ResolveClass(), args_resolve_class);
#else
    std::vector<Value*> args_load_class;
    args_load_class.push_back(art_method);
    loaded_class = irb_->CreateCall(
        fh_->LoadClass(this, irb_,
                       GetMethodIdx(), cls->GetTypeIndex().index_,
                       cls->MustGenerateClinitCheck()),
        args_load_class);
#endif
  }

  if (McrDebug::VerifyArtClass()) {
    irb_->AndroidLogPrintHex(INFO, "VisitLoadClass:", loaded_class);
    ArtCallVerifyArtClass(loaded_class);
  }

  addValue(cls, loaded_class);
}

void HGraphToLLVM::VisitClinitCheck(HClinitCheck* h) {
  Value* klass = getValue(h->InputAt(0));
#ifdef USE_SLOW_CLINIT_CHECK
  std::vector<Value*> args;
  args.push_back(klass);
  irb_->CreateCall(fh_->__ClassInit(), args);
#else
  std::vector<Value*> args;
  args.push_back(klass);
  DLOG(INFO) << "VisitClinitCheck: getting hf";
  irb_->CreateCall(fh_->GenerateClassInitializationCheck(this, irb_), args);
#endif
  addValue(h, klass);
}

void HGraphToLLVM::VisitInstanceOf(HInstanceOf* h) {
  DLOG(ERROR) << "VisitInstanceOf: " << prt_->GetInstruction(h);
  std::vector<Value*> args;

  DLOG(FATAL) << "TODO_LLVM: HInstanceOf" << prt_->GetInstruction(h);
  // INFO isClassFinal was removed and also the logic has changed a lot
  bool class_final = h->IsClassFinal();
  HLoadClass* hload_class = h->InputAt(1)->AsLoadClass();
  args.push_back(getValue(h->InputAt(0)));  // obj
  if (!class_final) {
    args.push_back(getValue(hload_class));  // class
  }
  Value* res =
      irb_->CreateCall(
          fh_->InstanceOf(irb_, this, h->MustDoNullCheck(),
                          class_final, hload_class->GetTypeIndex()),
          args);

  addValue(h, res);
}

void HGraphToLLVM::VisitLoadString(HLoadString* h) {
  D3LOG(INFO) << __func__;

  Value* art_method = irb_->CreateLoad(gbl_art_method_);
  Value* string_idx = irb_->getJUnsignedInt(h->GetStringIndex().index_);
  std::vector<Value*> args_load_string;
  args_load_string.push_back(art_method);
  args_load_string.push_back(string_idx);

  Value* loaded_string =
      irb_->CreateCall(fh_->__ResolveString(), args_load_string);
  addValue(h, loaded_string);
}

inline void HGraphToLLVM::CallVerifyArtMethod(Value* art_method) {
  std::vector<Value*> args;
  args.push_back(art_method);
  irb_->CreateCall(fh_->__VerifyArtMethod(), args);
}

inline void HGraphToLLVM::CallVerifyArtClass(Value* art_class) {
  std::vector<Value*> args;
  args.push_back(art_class);
  irb_->CreateCall(fh_->__VerifyArtClass(), args);
}

inline void HGraphToLLVM::CallVerifyArtObject(Value* art_object) {
  std::vector<Value*> args;
  args.push_back(art_object);
  irb_->CreateCall(fh_->__VerifyArtObject(), args);
}

}  // namespace LLVM
}  // namespace art
