/**
 * There are cases in this instruction that are not fully implemented
 * and resort to the slower runtime.
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

#include "llvm_macros_irb.h"


using namespace ::llvm;

namespace art {
namespace LLVM {

// INFO see notes in LLVM.bakerread.md before implementing stuff here
Function* FunctionHelper::InstanceOf(
    IRBuilder* irb, HInstanceOf* h, HGraphToLLVM* HL) {
  TypeCheckKind type_check_kind = h->GetTypeCheckKind();
  HLoadClass* hload_class = h->InputAt(1)->AsLoadClass();
  uint32_t class_offset = mirror::Object::ClassOffset().Int32Value();
  uint32_t super_offset = mirror::Class::SuperClassOffset().Int32Value();
  uint32_t component_offset = mirror::Class::ComponentTypeOffset().Int32Value();
  uint32_t primitive_offset = mirror::Class::PrimitiveTypeOffset().Int32Value();
  UNUSED(class_offset, super_offset, component_offset, primitive_offset);

  bool null_check = h->MustDoNullCheck();
  uint16_t type_idx = hload_class->GetTypeIndex().index_;

  std::string name = "InstanceOf" + std::to_string(type_idx);
  {
    std::stringstream ss;
    ss << type_check_kind;
    name+=ss.str();
  }

  if (null_check) name += "WithNullCheck";

  if (instance_of_.find(name) != instance_of_.end()) {
    return instance_of_[name];
  }

  D3LOG(WARNING) << "Creating function: " + name;
  BasicBlock* pinsert_point = irb->GetInsertBlock();

  std::vector<Type*> argsTy {2, irb->getVoidPointerType()};
  Type* retTy = irb->getJBooleanTy();
  FunctionType* ty = FunctionType::get(retTy, argsTy, false);

  Function* f = Function::Create(
      ty, Function::LinkOnceODRLinkage,
      name, irb->getModule());
  // f->addFnAttr(Attribute::AlwaysInline);
  f->setDSOLocal(true);
  HL->GetFunctionHelper()->AddAttributesCommon(f);

  Function::arg_iterator arg_iter(f->arg_begin());
  Argument* lobj = &*arg_iter++;
  Argument* lclass = nullptr;
  lobj->setName("obj");
  lclass = &*arg_iter++;
  lclass->setName("cls");

  BasicBlock* entry_block =
      BasicBlock::Create(irb->getContext(), "entry", f);
  irb->SetInsertPoint(entry_block);

  LOGLLVM3(WARNING, type_check_kind);

  if (null_check) {
    _doNullCheck(ctx_, irb, f, lobj);  
  }

  switch (type_check_kind) {
    case TypeCheckKind::kExactCheck: {  // VERIFIED
      ReadBarrierOption RBO =
          CodeGenerator::ReadBarrierOptionForInstanceOf(h);
      // /* HeapReference<Class> */ out = obj->klass_
      _exact_match(HL, this, h, ctx_, irb, f, lobj, lclass, nullptr, RBO, false);
      break;
    }

    case TypeCheckKind::kAbstractClassCheck: {
      ReadBarrierOption RBO =
          CodeGenerator::ReadBarrierOptionForInstanceOf(h);
      UNUSED(RBO);
      TODO_LLVM("Optimize: " << type_check_kind);
      // TODO_LLVM Essentially make something like: GenerateReferenceObjectClass,
      // but for different offsets, and pas it through the GenerateReferenceLoad
      //
      // Instead: keep fetching in loop the superclasses, and compare those
      // BUT KEEP THIS IN MIND:
      // do a check, and IF we will go to SlowPath for fetching a class
      // (GenerateReferenceLoad -> GenerateReadBarrierSlow)
      // then might be better to go direclty to SLowPath
      _SlowPathInstanceOfNonTrivial(HL, irb, lobj, lclass);
      // /* HeapReference<Class> */ out = obj->klass_
      // GenerateReferenceLoadTwoRegisters(h,
      //                                   out_loc,
      //                                   obj_loc,
      //                                   class_offset,
      //                                   maybe_temp_loc,
      //                                   RBO);
      // If the class is abstract, we eagerly fetch the super class of the
      // object to avoid doing a comparison we know will fail.
      // vixl::aarch64::Label loop, success;
      // __ Bind(&loop);
      // /* HeapReference<Class> */ out = out->super_class_
      // GenerateReferenceLoadOneRegister(h,
      //                                  out_loc,
      //                                  super_offset,
      //                                  maybe_temp_loc,
      //                                  RBO);
      // If `out` is null, we use it for the result, and jump to `done`.
      // INFO not sure but there is a loop here.. TODO
      // maybe is like:
      //  - if out is null we end (cbz)
      //  - if not but is not  the same (cmp), goto loop
      //  - otherwise store result (mov)
      // __ Cbz(out, &done);
      // __ Cmp(out, cls);
      // __ B(ne, &loop);
      // __ Mov(out, 1);
      //
      // if (zero.IsLinked()) {
      //   __ B(&done); TODO
      // }
      break;
    }

    case TypeCheckKind::kClassHierarchyCheck: {
      ReadBarrierOption RBO =
          CodeGenerator::ReadBarrierOptionForInstanceOf(h);
      // SIMILAR TO ABSTRACT (but does the initial check..)
      OPTIMIZE_LLVM(type_check_kind << ": walk over super classes");
      _cmpObjClassWithClass_SlowPathOnFalse(
          HL, this, h, ctx_, irb, f, lobj, lclass,
          nullptr, RBO, false);
      // obj->klass, then compare. if true returns (up to here done)
      // if not: keep fetching super_class_ until we match)
      // /* HeapReference<Class> */ out = obj->klass_
      // GenerateReferenceLoadTwoRegisters(h,
      //                                   out_loc,
      //                                   obj_loc,
      //                                   class_offset,
      //                                   maybe_temp_loc,
      //                                   RBO);
      // Walk over the class hierarchy to find a match.
      // vixl::aarch64::Label loop, success;
      // __ Bind(&loop);
      // __ Cmp(out, cls);
      // __ B(eq, &success);
      // /* HeapReference<Class> */ out = out->super_class_
      // GenerateReferenceLoadOneRegister(h,
      //                                  out_loc,
      //                                  super_offset,
      //                                  maybe_temp_loc,
      //                                  RBO);
      // __ Cbnz(out, &loop);
      // If `out` is null, we use it for the result, and jump to `done`.
      // __ B(&done);
      // __ Bind(&success);
      // __ Mov(out, 1);
      //
      // if (zero.IsLinked()) {
      //   __ B(&done);
      // }
      break;
    }

    case TypeCheckKind::kArrayObjectCheck: {
      ReadBarrierOption RBO =
          CodeGenerator::ReadBarrierOptionForInstanceOf(h);
      // DONE exact_check is OK! TODO the rest..
      TODO_LLVM(type_check_kind << ": OPTIMIZE: if is primitive check");
      _cmpObjClassWithClass_SlowPathOnFalse(
          HL, this, h, ctx_, irb, f, lobj, lclass, nullptr,
          RBO, false);
      // /* HeapReference<Class> */ out = obj->klass_
      // GenerateReferenceLoadTwoRegisters(h,
      //                                   out_loc,
      //                                   obj_loc,
      //                                   class_offset,
      //                                   maybe_temp_loc,
      //                                   RBO);
      // Do an exact check.
      // vixl::aarch64::Label exact_check;
      // __ Cmp(out, cls);
      // __ B(eq, &exact_check); INFO found. so jump to return true, ELSE:
      // INFO here it is: if it is NOT exact, then check if not primitive?
      // (their comment: next line..)
      // Otherwise, we need to check that the object's class is a non-primitive array.
      // INFO if component_type_ null means NO array..
      // /* HeapReference<Class> */ out = out->component_type_
      // GenerateReferenceLoadOneRegister(h,
      //                                  out_loc,
      //                                  component_offset,
      //                                  maybe_temp_loc,
      //                                  RBO);
      // If `out` is null, we use it for the result, and jump to `done`.
      // done: returns directly x0/w0 (what register had..)
      // __ Cbz(out, &done); NOT found. return false.
      // ELSE:
      // __ Ldrh(out, HeapOperand(out, primitive_offset));
      static_assert(Primitive::kPrimNot == 0, "Expected 0 for kPrimNot");
      // I guess the next line:
      // NotZero: a primitive (since object/primNot is 0)
      // if NotZero (a primitive) then then:
      // return zero (not found)
      // __ Cbnz(out, &zero); // NotZero: primitive: false
      // ELSE: we assume it is ok?!
      // __ Bind(&exact_check); // FOUND so return true
      // __ Mov(out, 1);
      // __ B(&done);
      break;
    }

    case TypeCheckKind::kArrayCheck: {
      // DONE should be ok: do the cmp, then SlowPath
      VERIFY_LLVM(type_check_kind);
      _cmpObjClassWithClass_SlowPathOnFalse(
          HL, this, h, ctx_, irb, f, lobj, lclass, nullptr,
          kWithReadBarrier, false);

      // No read barrier since the slow path will retry upon failure.
      // /* HeapReference<Class> */ out = obj->klass_
      // GenerateReferenceLoadTwoRegisters(h,
      //                                   out_loc,
      //                                   obj_loc,
      //                                   class_offset,
      //                                   maybe_temp_loc,
      //                                   kWithoutReadBarrier);
      // __ Cmp(out, cls);
      // slow_path = new (codegen_->GetScopedAllocator()) TypeCheckSlowPathARM64(
      //     h, /* is_fatal= */ false);
      // codegen_->AddSlowPath(slow_path);
      // __ B(ne, slow_path->GetEntryLabel());
      // __ Mov(out, 1);
      break;
    }

    case TypeCheckKind::kUnresolvedCheck:
    case TypeCheckKind::kInterfaceCheck: { // VERIFIED
      // Always go through RT (just like optimizing)
      _SlowPathInstanceOfNonTrivial(HL, irb, lobj, lclass);
      break;
    }

    case TypeCheckKind::kBitstringCheck: {
      DIE_TODO << type_check_kind;  // Implementation should be easy..
      // /* HeapReference<Class> */ temp = obj->klass_
      // GenerateReferenceLoadTwoRegisters(h,
      //                                   out_loc,
      //                                   obj_loc,
      //                                   class_offset,
      //                                   maybe_temp_loc,
      //                                   kWithoutReadBarrier);
      // GenerateBitstringTypeCheckCompare(h, out); TODO
      // __ Cset(out, eq);
      // if (zero.IsLinked()) {
      //   __ B(&done);
      // }
      break;
    }
  }

  // store the function and return it
  instance_of_[name] = f;
  irb->SetInsertPoint(pinsert_point);
  return f;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
