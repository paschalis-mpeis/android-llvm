/**
 * These ASM entrypoints are no longer in use, as they are implemeted
 * using LLVM instructions. see quick_entrypoints.cc
 * They are here, commented out, just for reference.
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
#include "asm_arm64.h"

#include <llvm/IR/DerivedTypes.h>
#include "art_method.h"
#include "llvm_macros_irb.h"
#include "hgraph_to_llvm.h"
#include "ir_builder.h"
#include "llvm_utils.h"
#include "mcr_rt/art_impl_arch_arm-inl.h"
#include "thread.h"

using namespace ::llvm;

namespace art {
namespace LLVM {
namespace Arm64 {

// /**
//  * from TypeCheckSlowPathARM64:
//  *    called by:
//  *        - VisitInstanceOf (qpoint: InstanceofNonTrivial) 
//  *        - CheckInstanceOf (qpoint: CheckInstanceOf)
//  * */
// Value* art_quick_check_instance_of(IRBuilder* irb,
//     QuickEntrypointEnum qpoint,
//     Value* lobj, Value* lclass) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(qpoint);
//   Value* x0 = _TwoArgDowncall(irb, qoffset, lobj, lclass,
//       irb->getJIntTy());
//   return x0;
// }

// void art_quick_initialize_static_storage(IRBuilder* irb, Value* klass) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(kQuickInitializeStaticStorage);
//   _voidOneArgDowncall(irb, qoffset, klass);
// }

// Value* art_quick_alloc_object__(IRBuilder* irb,
//     QuickEntrypointEnum qpoint, Value* klass) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(qpoint);
//   Value* x0 = _OneArgDowncall(irb, qoffset, klass);
//   Value* resolved_class = irb->CreateIntToPtr(x0, irb->getVoidPointerType());
//   resolved_class->setName("resolved_class");
//   return resolved_class;
// }

// /**
//  *
//  * art_quick_get_obj_static
//  *
//  * Probably primitives do not need RT:
//  * art_quick_get_boolean_static
//  * art_quick_get_byte_static
//  * art_quick_get_char_static
//  * art_quick_get_short_static
//  * art_quick_get32_static
//  * art_quick_get64_static
//  *
//  * @param irb
//  * @param type
//  * @param jvalue
//  * @param obj
//  *
//  * @return 
//  */
// Value* art_quick_get_obj_instance(
//     IRBuilder* irb, Value* lobj, uint32_t field_idx) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(kQuickGetObjInstance);
//   return Arm64::_TwoArgDowncallI(irb, qoffset, lobj, field_idx, true);
// }

// Value* art_quick_get_obj_static(
//     IRBuilder* irb, uint32_t field_idx) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(kQuickGetObjStatic);
//   return Arm64::_OneArgDowncallI(irb, qoffset, field_idx);
// }

// void art_llvm_jvalue_setl(IRBuilder* irb, Value* jvalue, Value* obj) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(kQuickLLVMJValueSetL);
//   _voidTwoArgDowncall(irb, qoffset, jvalue, obj);
// }

// /**
//  * @brief
//  *
//  * @param irb
//  * @param referrer attribute unused in: artReadBarrierSlow (quick_field_entrypoints.cc)
//  *
//  * @param object
//  * @param member_offset
//  *
//  * @return 
//  */
// Value* art_llvm_read_barrier_slow(
//     IRBuilder* irb, Value* referrer, Value* object,
//     Value* member_offset) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(kQuickLLVMReadBarrierSlow);

//   std::vector<Type*> params{
//     irb->getVoidPointerType(),
//       irb->getVoidPointerType(),
//       irb->getJIntTy()};
//   Value* x0 = _ThreeArgDowncall(irb, qoffset, params,
//       referrer, object, member_offset);
//   return irb->CreateIntToPtr(x0, irb->getVoidPointerType());
// }

// Value* art_llvm_resolve_interface_method(IRBuilder* irb,
//     Value* receiver, Value* referrer, uint32_t imt_index) {
//   AVOID_ASM;
//   int32_t qoffset = Arm64::offset(kQuickLLVMResolveInterfaceMethod);

//   std::vector<Type*> params{
//     irb->getVoidPointerType(),
//       irb->getVoidPointerType(),
//       irb->getJIntTy()};
//   Value* x0 = _ThreeArgDowncallI(irb, qoffset, params,
//       receiver, referrer, imt_index);
//   return irb->CreateIntToPtr(x0, irb->getVoidPointerType());
// }

// Value* art_llvm_resolve_string(IRBuilder* irb,
//     Value* caller, uint32_t string_idx) {
//   AVOID_ASM;
//   QuickEntrypointEnum qpoint = kQuickLLVMResolveString;
//   int32_t qoffset = Arm64::offset(qpoint);
//   Value* x0 = _TwoArgDowncallI(irb, qoffset, caller, string_idx);
//   Value* resolved_string = irb->CreateIntToPtr(x0, irb->getVoidPointerType());
//   resolved_string->setName("resolved_string");
//   return resolved_string;
// }

#include "llvm_macros_undef.h"

}  // namespace Arm64
}  // namespace LLVM
}  // namespace art
