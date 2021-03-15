/**
 * Code for calling Quick entrypoints. These are runtime entrypoints
 * that already existed for supporting the Quick backend.
 *
 * Here we might also call some additional entrypoints we have implemented
 * for the LLVM backend.
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

#include <llvm/Support/raw_ostream.h>
#include "art_method.h"
#include "asm_arm64.h"
#include "asm_arm_thumb.h"
#include "class_linker-inl.h"
#include "hgraph_to_llvm.h"
#include "mcr_cc/analyser.h"
#include "mcr_cc/mcr_cc.h"
#include "mirror/class_loader.h"
#include "mirror/object.h"
#include "mirror/string.h"
#include "scoped_thread_state_change.h"

#include "llvm_macros_irb_.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

// llvm_verify_stack_frame_current
void HGraphToLLVM::ArtCallVerifyStackFrameCurrent() {
  VERIFIED_;
  Type* retTy = irb_->getVoidTy();
  artCall(kQuickLLVMVerifyStackFrameCurrent, retTy);
}

void HGraphToLLVM::ArtCallVerifyArtMethod(Value* art_method) {
  VERIFIED_;
  std::vector<Value*> args{ art_method };
  std::vector<Type*> params{ irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();

  artCall(kQuickLLVMVerifyArtMethod, retTy, params, args);
}

void HGraphToLLVM::ArtCallVerifyArtClass(Value* art_class) {
  VERIFIED_;
  std::vector<Value*> args{ art_class };
  std::vector<Type*> params{ irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();

  artCall(kQuickLLVMVerifyArtClass, retTy, params, args);
}

void HGraphToLLVM::ArtCallVerifyArtObject(Value* object) {
  std::vector<Value*> args{ object };
  std::vector<Type*> params{ irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();

  artCall(kQuickLLVMVerifyArtObject, retTy, params, args);
}

/**
 * @brief Handled a bit specially.
 *        I am sending an extra parameter in comparison to quick,
 *        which is ArtMethod.
 *
 *        We could have skipped though:
 *        This is because quick has QuickFrame that contains caller and outer.
 *        We have `ShadowFrame` that contains only the caller/referrer.
 *        However, we don't need outer, as outer has different value
 *        with caller probably in inline cases. Since we have disabled inlining
 *        to the call site of LLVM (so we can actually invoke LLVM), we should
 *        be OK.
 *
 *        Still we are using different entrypoint.
 *        To use quick's entrypoint, we have to do something like:
 *        if (FROM_LLVM()) 
 *        outer = caller = LLVM::shadow_frame_->GetMethod();
 *
 *        INFO this is may now fixed, as we are setting up ShadowFrame for the LLVM call
 *
 * @param irb
 * @param qpoint
 * @param caller
 * @param type_idx
 *
 * @return 
 */
// art_llvm_resolve_type
Value* HGraphToLLVM::ArtCallResolveType__(
    QuickEntrypointEnum qpoint, Value* caller, uint32_t type_idx,
    Value* llvm_bss_slot) {
  VERIFY_LLVMD(qpoint);
  std::vector<Value*> args {
    caller, irb_->getJUnsignedInt(type_idx), llvm_bss_slot };
  std::vector<Type*> params {
    irb_->getVoidPointerType(),
      irb_->getJIntTy(),
      irb_->getVoidPointerType()->getPointerTo()};
  Type* retTy = irb_->getVoidPointerType();
  return artCall(qpoint, retTy, params, args);
}

/**
 * from TypeCheckSlowPathARM64:
 *    called by:
 *        - VisitInstanceOf (qpoint: InstanceofNonTrivial) 
 */
// artInstanceOfFromCode
Value* HGraphToLLVM::ArtCallInstanceOfNonTrivial(
    Value* lobj, Value* lclass) {
  VERIFY_LLVMD3_;
  std::vector<Value*> args{ lobj, lclass };
  std::vector<Type*> params{ 2, irb_->getVoidPointerType() };
  Type* retTy = irb_->getJIntTy();

  return artCall(kQuickInstanceofNonTrivial, retTy, params, args);
}

void HGraphToLLVM::ArtCallDeliverException(Value* lexception) {
  VERIFY_LLVM_;
  std::vector<Value*> args{ lexception };
  std::vector<Type*> params{ irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();

  artCall(kQuickDeliverException, retTy, params, args);
}

/**
 *    called by:
 *        - CheckInstanceOf (qpoint: CheckInstanceOf) VERIFY_LLVM 
 *
 * art_quick_check_instance_of
 */
void HGraphToLLVM::ArtCallCheckInstanceOf(Value* lobj, Value* lclass) {
  VERIFY_LLVMD2_;
  std::vector<Value*> args{ lobj, lclass };
  std::vector<Type*> params{ 2, irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();
  artCall(kQuickCheckInstanceOf, retTy, params, args);
}

// art_quick_initialize_static_storage
void HGraphToLLVM::ArtCallInitializeStaticStorage(Value* klass) {
  VERIFY_LLVMD3_;  // DONE
  std::vector<Value*> args{ klass };
  std::vector<Type*> params{ irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();

  artCall(kQuickInitializeStaticStorage, retTy, params, args);
}

// art_quick_alloc_object__
Value* HGraphToLLVM::ArtCallAllocObject__(
    QuickEntrypointEnum qpoint, Value* klass) {
  VERIFY_LLVMD3(qpoint);

  std::vector<Value*> args{ klass };
  std::vector<Type*> params{ irb_->getVoidPointerType() };
  Type* retTy = irb_->getVoidPointerType();

  return artCall(qpoint, retTy, params, args);
}
// art_quick_alloc_array_resolved (not used actually..)
// art_quick_alloc_array_resolved8
// art_quick_alloc_array_resolved16
// art_quick_alloc_array_resolved32
// art_quick_alloc_array_resolved64
Value* HGraphToLLVM::ArtCallAllocArray__(
    QuickEntrypointEnum qpoint, Value* klass, Value* length) {
  VERIFIED(qpoint);

  std::vector<Value*> args{ klass, length };
  std::vector<Type*> params{ irb_->getVoidPointerType(), irb_->getJIntTy() };
  Type* retTy = irb_->getVoidPointerType();

  return artCall(qpoint, retTy, params, args);
}

// art_quick_aput_obj INFO only this one is used
// art_quick_aput_obj_with_bound_check
// art_quick_aput_obj_with_null_and_bound_check
void HGraphToLLVM::ArtCallAputObject(
    Value* array, Value* index, Value* storeObj) {
  QuickEntrypointEnum qpoint = kQuickAputObject ;
  VERIFY_LLVMD(qpoint);

  std::vector<Value*> args{ array, index, storeObj };
  std::vector<Type*> params{ irb_->getVoidPointerType(),
    irb_->getJIntTy(),
    irb_->getVoidPointerType()
  };
  Type* retTy = irb_->getJVoidTy();
  artCall(qpoint, retTy, params, args);
}

// art_llvm_get_obj_instance
Value* HGraphToLLVM::ArtCallGetObjInstance(
    uint32_t field_idx, Value* lobj, Value* lreferrer) {
  CHECK_LLVM("Must only be used by: GenerateUnresolvedFieldAccess");
  // Which is used by HInstructions:
  // VisitUnresolved<Static/Instance>Field<Get/Set>
  std::vector<Value*> args{irb_->getJUnsignedInt(field_idx), lobj, lreferrer};
  std::vector<Type*> params{
    irb_->getJIntTy(),
    irb_->getVoidPointerType(),
    irb_->getVoidPointerType(),
  };
  Type* retTy = irb_->getVoidPointerType();

  return artCall(kQuickLLVMGetObjInstance, retTy, params, args);
}

// art_llvm_get_obj_static
Value* HGraphToLLVM::ArtCallGetObjStatic(uint32_t field_idx, Value* lreferrer) {
  CHECK_LLVM("Must only be used by: GenerateUnresolvedFieldAccess");
  // Which is used by HInstructions:
  // VisitUnresolved<Static/Instance>Field<Get/Set>
  std::vector<Value*> args{irb_->getJUnsignedInt(field_idx), lreferrer};
  std::vector<Type*> params{irb_->getJIntTy(), irb_->getVoidPointerType()};
  Type* retTy = irb_->getVoidPointerType();

  return artCall(kQuickLLVMGetObjStatic, retTy, params, args);
}

// art_llvm_jvalue_setl
void HGraphToLLVM::ArtCallJValueSetL(Value* jvalue, Value* obj) {
  XLLVM;
  jvalue = irb_->CreateBitCast(jvalue, irb_->getVoidPointerType());
  std::vector<Value*> args{ jvalue, obj };
  std::vector<Type*> params{ 2, irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();

  artCall(kQuickLLVMJValueSetL, retTy, params, args);
}

/**
 * @brief
 *
 * @param irb
 * @param referrer attribute unused in: artReadBarrierSlow (quick_field_entrypoints.cc)
 *
 * @param object
 * @param member_offset
 *
 * @return 
 */
// art_llvm_read_barrier_slow
Value* HGraphToLLVM::ArtCallReadBarrierSlow(
    Value* referrer, Value* object, Value* member_offset) {
  // VERIFY_LLVM_;

  std::vector<Value*> args{ referrer, object, member_offset };
  std::vector<Type*> params{
    irb_->getVoidPointerType(),
    irb_->getVoidPointerType(),
    irb_->getJIntTy()
  };
  Type* retTy = irb_->getVoidPointerType();

  Value* val  = artCall(kQuickLLVMReadBarrierSlow, retTy, params, args);
  return val;
}

// art_llvm_resolve_external_method
Value* HGraphToLLVM::ArtCallResolveExternalMethod(
    Value* referrer, Value* dex_filename, Value* dex_location,
    Value* ldex_method_idx, Value* linvoke_type) {
  VERIFY_LLVMD5_;

  std::vector<Value*> args{ referrer,
                            dex_filename,
                            dex_location,
                            ldex_method_idx,
                            linvoke_type };
  std::vector<Type*> params{ irb_->getVoidPointerType(),
                             irb_->getVoidPointerType(),  // const char*
                             irb_->getVoidPointerType(),  // const char*
                             irb_->getJIntTy(),
                             irb_->getJIntTy() };
  Type* retTy = irb_->getVoidPointerType();

  return artCall(kQuickLLVMResolveExternalMethod, retTy, params, args);
}

// art_llvm_resolve_external_methodII
Value* HGraphToLLVM::ArtCallResolveExternalMethod(
    Value* referrer, Value* dex_filename, Value* dex_location,
    uint32_t dex_method_idx, InvokeType invoke_type) {
  Value* ldex_method_idx = irb_->getJUnsignedInt(dex_method_idx);
  Value* linvoke_type =
      irb_->getJUnsignedInt(static_cast<uint32_t>(invoke_type));
  return ArtCallResolveExternalMethod(referrer, dex_filename,
                                      dex_location, ldex_method_idx, linvoke_type);
}

// art_llvm_resolve_internal_method
Value* HGraphToLLVM::ArtCallResolveInternalMethod(
    Value* referrer, Value* ldex_method_idx, Value* linvoke_type) {
  VERIFIED_;

  std::vector<Type*> params{ irb_->getVoidPointerType(),
                             irb_->getJIntTy(),
                             irb_->getJIntTy() };
  std::vector<Value*> args{ referrer, ldex_method_idx, linvoke_type };
  Type* retTy = irb_->getVoidPointerType();

  Value* resolved_method =
      artCall(kQuickLLVMResolveInternalMethod, retTy, params, args);

  return resolved_method;
}

// art_llvm_resolve_internal_methodII
Value* HGraphToLLVM::ArtCallResolveInternalMethod(
    Value* referrer, uint32_t dex_method_idx, InvokeType invoke_type) {
  VERIFIED_;
  Value* ldex_method_idx = irb_->getJUnsignedInt(dex_method_idx);
  Value* linvoke_type =
      irb_->getJUnsignedInt(static_cast<uint32_t>(invoke_type));

  return ArtCallResolveInternalMethod(
      referrer, ldex_method_idx, linvoke_type);
}

// art_llvm_resolve_virtual_method
Value* HGraphToLLVM::ArtCallResolveVirtualMethod(
    Value* receiver, Value* referrer) {
  VERIFIED_;

  std::vector<Type*> params{ 2, irb_->getVoidPointerType() };
  std::vector<Value*> args{ receiver, referrer };
  Type* retTy = irb_->getVoidPointerType();

  Value* resolved_method =
      artCall(kQuickLLVMResolveVirtualMethod, retTy, params, args);

  return resolved_method;
}

// do not use this!
// art_llvm_resolve_interface_method
Value* HGraphToLLVM::ArtCallResolveInterfaceMethod(
    Value* receiver, Value* referrer) {
  std::vector<Type*> params{ 2, irb_->getVoidPointerType() };
  std::vector<Value*> args { receiver, referrer };
  Type* retTy = irb_->getVoidPointerType();
  Value* resolved_method =
      artCall(kQuickLLVMResolveInterfaceMethod, retTy, params, args);

  if(McrDebug::DebugLlvmCode4()) {
    VERIFIED("Resolved");
    ArtCallVerifyArtMethod(resolved_method);
  }
  return resolved_method;
}

// art_llvm_resolve_string
Value* HGraphToLLVM::ArtCallResolveString(
    Value* caller, uint32_t string_idx, Value* llvm_bss_slot) {
  VERIFY_LLVMD3_;
  std::vector<Type*> params { irb_->getVoidPointerType(),
    irb_->getJIntTy(), irb_->getVoidPointerType()->getPointerTo()};
  std::vector<Value*> args {
    caller,
      irb_->getJUnsignedInt(string_idx),
      llvm_bss_slot};
  Type* retTy = irb_->getVoidPointerType();

  return artCall(kQuickLLVMResolveString, retTy, params, args);
}

void HGraphToLLVM::ArtCallMonitorOperation(Value* lobj, bool is_lock) {
  VERIFY_LLVMD_;
  std::vector<Value*> args{ lobj };
  std::vector<Type*> params{ irb_->getVoidPointerType() };
  Type* retTy = irb_->getJVoidTy();

  QuickEntrypointEnum qpoint =
      is_lock ? kQuickLockObject : kQuickUnlockObject;

  artCall(qpoint, retTy, params, args);
}

void HGraphToLLVM::ArtCallTestSuspend() {
  // Go through LLVM as Quick has some time issues
  // ArtCallLLVMTestSuspend();
  ArtCallQuickTestSuspend();
}

// this adds an extra unnecessary wrapper
// on top of art_quick_test_suspend
// art_llvm_test_suspend
void HGraphToLLVM::ArtCallLLVMTestSuspend() {
  DIE << "DONT USE THIS!";
  VERIFY_LLVMD5(__func__);
  Type* retTy = irb_->getVoidPointerType();
  artCall(kQuickLLVMTestSuspend, retTy);
}

// art_quick_test_suspend
void HGraphToLLVM::ArtCallQuickTestSuspend() {
  // CHECK: this might cause issues after all..
  VERIFY_LLVMD(__func__);
  Type* retTy = irb_->getVoidPointerType();
  artCall(kQuickTestSuspend, retTy);
}

// art_llvm_push_quick_frame
void HGraphToLLVM::ArtCallPushQuickFrame(Value* fragment) {
  VERIFIED_;

  fragment=irb_->CreateBitCast(fragment, irb_->getVoidPointerType());

  std::vector<Value*> args {fragment};
  std::vector<Type*> params {irb_->getVoidPointerType()};
  Type* retTy = irb_->getVoidPointerType();
  artCall(kQuickLLVMPushQuickFrame, retTy, params, args);
}

// art_llvm_pop_quick_frame
void  HGraphToLLVM::ArtCallPopQuickFrame(Value* fragment) {
  VERIFIED_;
  fragment=irb_->CreateBitCast(fragment, irb_->getVoidPointerType());

  std::vector<Value*> args {fragment};
  std::vector<Type*> params {irb_->getVoidPointerType()};
  Type* retTy = irb_->getJVoidTy();
  artCall(kQuickLLVMPopQuickFrame, retTy, params, args);
}

// art_llvm_clear_top_of_stack
void HGraphToLLVM::ArtCallClearTopOfStack() {
  DIE << "done use this";
  Type* retTy = irb_->getVoidPointerType();
  artCall(kQuickLLVMClearTopOfStack, retTy);
}

/**
 *  extern"C" void art_quick_invoke_stub(ArtMethod *method,   x0
 *                                       uint32_t  *args,     x1
 *                                       uint32_t argsize,    w2
 *                                       Thread *self,        x3
 *                                       JValue *result,      x4
 *                                       char   *shorty);     x5
 */
void HGraphToLLVM::ArtCallInvokeQuick__(
    HInvoke* hinvoke, Value* art_method, Value* qargs,
    Value* qargs_size, Value* jvalue, Value* shorty, bool is_static,
    bool use_wrapper) {
  QuickEntrypointEnum entrypoint;
  CHECK(!use_wrapper) << "don't use wrapper entrypoint.";
  entrypoint = is_static ? kQuickLLVMInvokeQuickStaticDirect
    : kQuickLLVMInvokeQuickDirect;

  // use method index instead of ArtMethod*
  bool use_method_idx = false;

  std::vector<Type*> params;
  if(use_method_idx) {
    params.push_back(irb_->getJIntTy());
  } else {
    params.push_back(irb_->getVoidPointerType());          // method
  }

  params.push_back(irb_->getJIntTy()->getPointerTo());    // args
  params.push_back(irb_->getJIntTy());                    // argssize
  params.push_back(irb_->getVoidPointerType());           // Thread
  params.push_back(irb_->getJValueTy()->getPointerTo());  // result
  params.push_back(irb_->getVoidPointerType());           // shorty

  std::vector<Value*> args{
    art_method,
    qargs,
    qargs_size,
    GetLoadedThread(),
    jvalue,
    shorty
  };

  Type* retTy = irb_->getJVoidTy();
  artCall(entrypoint, retTy, params, args);
}

}  // namespace LLVM
}  // namespace art

#include "llvm_macros_undef.h"
