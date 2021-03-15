/**
 * Several cases in this instruction are not implemented and resort to the runtime.
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
// art_quick_check_instance_of
// kQuickCheckInstanceOf
Function* FunctionHelper::CheckCast(
    IRBuilder* irb, HCheckCast* h, HGraphToLLVM* HL) {
  const bool null_check = h->MustDoNullCheck();
  D3LOG(INFO) << "CheckCast: " << (null_check ? " WithNullCheck" : "");

  TypeCheckKind type_check_kind = h->GetTypeCheckKind();
  HLoadClass* hload_class = h->InputAt(1)->AsLoadClass();
  uint16_t type_idx = hload_class->GetTypeIndex().index_;
  std::string name = "CheckCast_" + std::to_string(type_idx);
  {
    std::stringstream ss;
    ss << type_check_kind;
    name+=ss.str();
  }
  if (null_check) name += "_NullCheck";

  if (check_cast_.find(name) != check_cast_.end()) {
    return check_cast_[name];
  }

  D3LOG(INFO) << "Creating function: CastCheck";
  BasicBlock* pinsert_point = irb->GetInsertBlock();
  const uint32_t class_offset = mirror::Object::ClassOffset().Int32Value();
  const uint32_t super_offset = mirror::Class::SuperClassOffset().Int32Value();
  const uint32_t component_offset = mirror::Class::ComponentTypeOffset().Int32Value();
  const uint32_t primitive_offset = mirror::Class::PrimitiveTypeOffset().Int32Value();
  const uint32_t iftable_offset = mirror::Class::IfTableOffset().Uint32Value();
  const uint32_t array_length_offset = mirror::Array::LengthOffset().Uint32Value();
  const uint32_t object_array_data_offset =
      mirror::Array::DataOffset(kHeapReferenceSize).Uint32Value();
  bool is_type_check_slow_path_fatal =
    CodeGenerator::IsTypeCheckSlowPathFatal(h);
  UNUSED(class_offset, super_offset, component_offset, primitive_offset);
  UNUSED(is_type_check_slow_path_fatal, iftable_offset,
      array_length_offset, object_array_data_offset);

  std::vector<Type*> argsTy { irb->getJObjectTy()->getPointerTo(),
    irb->getJObjectTy()->getPointerTo(), irb->getJIntTy()};
  FunctionType* ty=FunctionType::get(irb->getVoidTy(), argsTy, false);

  Function* f = Function::Create(
      ty, Function::LinkOnceODRLinkage, name, irb->getModule());
  // f->addFnAttr(Attribute::AlwaysInline);
  f->setDSOLocal(true);
  HL->GetFunctionHelper()->AddAttributesCommon(f);

  // TypeCheckKind::kBitstringCheck TODO die?
  Function::arg_iterator arg_iter(f->arg_begin());
  Argument* lobj = &*arg_iter++;
  Argument* lclass = &*arg_iter++;
  Argument* ldex_pc = &*arg_iter++;
  lobj->setName("obj");
  lclass->setName("cls");
  ldex_pc->setName("dex_pc");

  BasicBlock* entry_block =
      BasicBlock::Create(irb->getContext(), "entry", f);
  irb->SetInsertPoint(entry_block);

  // Avoid null check if we know obj is not null.
  if (null_check) {
    // if null: return
    _doNullCheck_ReturnIfNull(ctx_, irb, f, lobj);  
    // __ Cbz(obj, &done);
  }

  ReadBarrierOption RBO = kWithoutReadBarrier;
  switch (type_check_kind) {
    case TypeCheckKind::kExactCheck:
    case TypeCheckKind::kArrayCheck:
      {
        // /* HeapReference<Class> */ temp = obj->klass_
        // GenerateReferenceLoadTwoRegisters(
        //     h,
        //     temp_loc,
        //     obj_loc,
        //     class_offset,
        //     maybe_temp2_loc,
        //     kWithoutReadBarrier);
        // __ Cmp(temp, cls);
        // Jump to slow path for throwing the exception or doing a
        // more involved array check.
        // __ B(ne, type_check_slow_path->GetEntryLabel());

        // CHECK_LLVM
        _exact_match(HL, this, h, ctx_, irb, f,
            lobj, lclass, ldex_pc, RBO, true);
        break;
      }
    case TypeCheckKind::kAbstractClassCheck:
      {
        // /* HeapReference<Class> */ temp = obj->klass_
        // GenerateReferenceLoadTwoRegisters(
        //     h,
        //     temp_loc,
        //     obj_loc,
        //     class_offset,
        //     maybe_temp2_loc,
        //     kWithoutReadBarrier);
        // If the class is abstract, we eagerly fetch the super class of the
        // object to avoid doing a comparison we know will fail.
        // vixl::aarch64::Label loop;
        // __ Bind(&loop);
        // /* HeapReference<Class> */ temp = temp->super_class_
        // GenerateReferenceLoadOneRegister(
        //     h,
        //     temp_loc,
        //     super_offset,
        //     maybe_temp2_loc,
        //     kWithoutReadBarrier);

        // If the class reference currently in `temp` is null, jump to the slow path to throw the
        // exception.
        // __ Cbz(temp, type_check_slow_path->GetEntryLabel());
        // Otherwise, compare classes.
        // __ Cmp(temp, cls);
        // __ B(ne, &loop);
        TODO_LLVMD("Optimize: " << type_check_kind);
        // calling directly the runtime
        _SlowPathCheckInstanceOf(HL, irb, lobj, lclass);
        break;
      }
    case TypeCheckKind::kClassHierarchyCheck:
      {
      // /* HeapReference<Class> */ temp = obj->klass_
      // GenerateReferenceLoadTwoRegisters(
      //     h,
      //     temp_loc,
      //     obj_loc,
      //     class_offset,
      //     maybe_temp2_loc,
      //     kWithoutReadBarrier);
      // Walk over the class hierarchy to find a match.
      // vixl::aarch64::Label loop;
      // __ Bind(&loop);
      // __ Cmp(temp, cls);
      // __ B(eq, &done);
      // /* HeapReference<Class> */ temp = temp->super_class_
      // GenerateReferenceLoadOneRegister(
      //     h,
      //     temp_loc,
      //     super_offset,
      //     maybe_temp2_loc,
      //     kWithoutReadBarrier);
      // If the class reference currently in `temp` is not null, jump
      // back at the beginning of the loop.
      // __ Cbnz(temp, &loop);
      // Otherwise, jump to the slow path to throw the exception.
      // __ B(type_check_slow_path->GetEntryLabel());
      OPTIMIZE_LLVM(type_check_kind << ": walk over super classes");
      _cmpObjClassWithClass_SlowPathOnFalse(
          HL, this, h, ctx_, irb, f, lobj, lclass,
          ldex_pc, RBO, true);
      break;
    }

    case TypeCheckKind::kArrayObjectCheck:
      {
      // /* HeapReference<Class> */ temp = obj->klass_
      // GenerateReferenceLoadTwoRegisters(
      //     h,
      //     temp_loc,
      //     obj_loc,
      //     class_offset,
      //     maybe_temp2_loc,
      //     kWithoutReadBarrier);
      // Do an exact check.
      // __ Cmp(temp, cls);
      // __ B(eq, &done);
      // Otherwise, we need to check that the object's class is a non-primitive array.
      // /* HeapReference<Class> */ temp = temp->component_type_
      // GenerateReferenceLoadOneRegister(
      //     h,
      //                                  temp_loc,
      //                                  component_offset,
      //                                  maybe_temp2_loc,
      //                                  kWithoutReadBarrier);
      // If the component type is null, jump to the slow path to throw the exception.
      // __ Cbz(temp, type_check_slow_path->GetEntryLabel());
      // Otherwise, the object is indeed an array. Further check that this component type is not a
      // primitive type.
      // __ Ldrh(temp, HeapOperand(temp, primitive_offset));
      // static_assert(Primitive::kPrimNot == 0, "Expected 0 for kPrimNot");
      // __ Cbnz(temp, type_check_slow_path->GetEntryLabel());
      // DONE exact_check is OK! TODO the rest..
      TODO_LLVMD(type_check_kind << ": OPTIMIZE: if is primitive check");
      _cmpObjClassWithClass_SlowPathOnFalse(
          HL, this, h, ctx_, irb, f, lobj, lclass, ldex_pc,
          RBO, true);
      break;
    }

    case TypeCheckKind::kUnresolvedCheck:
      // We always go into the type check slow path for the unresolved check cases.
      //
      // We cannot directly call the CheckCast runtime entry point
      // without resorting to a type checking slow path here (i.e. by
      // calling InvokeRuntime directly), as it would require to
      // assign fixed registers for the inputs of this HInstanceOf
      // h (following the runtime calling convention), which
      // might be cluttered by the potential first read barrier
      // emission at the beginning of this method.
      // __ B(type_check_slow_path->GetEntryLabel());
      _SlowPathCheckInstanceOf(HL, irb, lobj, lclass);
      break;
    case TypeCheckKind::kInterfaceCheck:
      {
        // /* HeapReference<Class> */ temp = obj->klass_
        // GenerateReferenceLoadTwoRegisters(
        //     h,
        //     temp_loc,
        //     obj_loc,
        //     class_offset,
        //     maybe_temp2_loc,
        //     kWithoutReadBarrier);

        // /* HeapReference<Class> */ temp = temp->iftable_
        // GenerateReferenceLoadTwoRegisters(
        //     h,
        //     temp_loc,
        //     temp_loc,
        //     iftable_offset,
        //     maybe_temp2_loc,
        //     kWithoutReadBarrier);
        // Iftable is never null.
        // __ Ldr(WRegisterFrom(maybe_temp2_loc), HeapOperand(temp.W(), array_length_offset));
        // Loop through the iftable and check if any class matches.
        // vixl::aarch64::Label start_loop;
        // __ Bind(&start_loop);
        // __ Cbz(WRegisterFrom(maybe_temp2_loc), type_check_slow_path->GetEntryLabel());
        // __ Ldr(WRegisterFrom(maybe_temp3_loc), HeapOperand(temp.W(), object_array_data_offset));
        // GetAssembler()->MaybeUnpoisonHeapReference(WRegisterFrom(maybe_temp3_loc));
        // // Go to next interface.
        // __ Add(temp, temp, 2 * kHeapReferenceSize);
        // __ Sub(WRegisterFrom(maybe_temp2_loc), WRegisterFrom(maybe_temp2_loc), 2);
        // // Compare the classes and continue the loop if they do not match.
        // __ Cmp(cls, WRegisterFrom(maybe_temp3_loc));
        // __ B(ne, &start_loop);
        OPTIMIZE_LLVM(type_check_kind);
        _SlowPathCheckInstanceOf(HL, irb, lobj, lclass);
        break;
      }

    case TypeCheckKind::kBitstringCheck:
      {
      DIE_TODO << type_check_kind;  // Implementation should be easy..
      // /* HeapReference<Class> */ temp = obj->klass_
      // GenerateReferenceLoadTwoRegisters(
      //     h,
      //     temp_loc,
      //     obj_loc,
      //     class_offset,
      //     maybe_temp2_loc,
      //     kWithoutReadBarrier);
      // GenerateBitstringTypeCheckCompare(h, temp);
      // __ B(ne, type_check_slow_path->GetEntryLabel());
      break;
    }
  }

  check_cast_[name] = f;
  irb->SetInsertPoint(pinsert_point);

  return f;
}


#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
