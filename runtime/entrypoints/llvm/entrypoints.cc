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
#include "mcr_rt/mcr_rt.h"

#include "art_method-inl.h"
#include "base/callee_save_type.h"
#include "class_linker-inl.h"
#include "class_table-inl.h"
#include "dex/dex_file-inl.h"
#include "dex/dex_file_types.h"
#include "entrypoints/entrypoint_utils-inl.h"
#include "gc/heap.h"
#include "mirror/class-inl.h"
#include "mirror/class_loader.h"
#include "mirror/object-inl.h"
#include "mirror/object_array-inl.h"
#include "oat_file.h"
#include "runtime.h"

#ifdef ART_MCR
#include "jvalue-inl.h"
#include "mcr_rt/art_impl-inl.h"

namespace art {

extern "C" void art_quick_invoke_stub(ArtMethod*, uint32_t*, uint32_t, Thread*, JValue*,
                                      const char*);
extern "C" void art_quick_invoke_static_stub(ArtMethod*, uint32_t*, uint32_t, Thread*, JValue*,
                                             const char*);

extern "C" void* artResolveInternalMethodFromLLVM(
ArtMethod* referrer, uint32_t dex_method_idx,
    uint32_t iinvoke_type, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  LOGLLVM4(INFO) << __func__;
  LLVM_FRAME_FIXUP(self);
  void* vresolved_method=
    LLVM::_ResolveInternalMethod(referrer, dex_method_idx, iinvoke_type, self);
  LOGLLVM4(INFO) << __func__ << ": resolved_method: "
    << static_cast<ArtMethod*>(vresolved_method)->PrettyMethod()
    << " (" << std::hex << vresolved_method << ")";
  return vresolved_method; }

extern "C" void* artResolveExternalMethodFromLLVM(
    ArtMethod* referrer, const char* dex_filename, const char* dex_location,
    uint32_t dex_method_idx, uint32_t iinvoke_type, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  LLVM_FRAME_FIXUP(self);
  LOGLLVM4(INFO) << __func__ << ": referrer: " << referrer->PrettyMethod();

  void* vresolved_method=
    LLVM::_ResolveExternalMethod(referrer,
        dex_filename, dex_location,
        dex_method_idx, iinvoke_type, self);
  LOGLLVM3(INFO) << __func__ << ": resolved_method: "
    << static_cast<ArtMethod*>(vresolved_method)->PrettyMethod()
    << " (" << std::hex << vresolved_method << ")";
  return vresolved_method;
}

extern "C" void* artResolveVirtualMethodFromLLVM(
    mirror::Object* receiver, ArtMethod* referrer, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  LLVM_FRAME_FIXUP(self);
  D4LOG(ERROR) << __func__ << ": referrer: "
    << referrer->PrettyMethod()
    << " (" << referrer->GetMethodIndex() << "/"
    << std::hex << referrer << ")";
  D4LOG(INFO) << __func__ << ": receiver: "
    << LLVM::PrettyClass(receiver->GetClass())
    << " (" << std::hex << receiver << ")";

  void* vresolved_method=
    LLVM::_ResolveVirtualMethod(receiver, referrer);
#ifdef CRDEBUG3
  ArtMethod* method= static_cast<ArtMethod*>(vresolved_method);
  D2LOG(WARNING) << __func__ << ": "
    << method->GetDexMethodIndex() << ": "
    << method->PrettyMethod();
#endif
  return vresolved_method;
}

extern "C" void* artResolveInterfaceMethodFromLLVM(
    mirror::Object* raw_this_object,
    ArtMethod* interface_method, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  LLVM_FRAME_FIXUP(self);
  D4LOG(INFO) << __func__ << ": referrer: " << interface_method->PrettyMethod()
    << ": idx: " << interface_method->GetDexMethodIndex();
  D4LOG(INFO) << __func__ << ": obj: "
    << LLVM::PrettyClass(raw_this_object->GetClass());

  ScopedQuickEntrypointChecks sqec(self);
  StackHandleScope<2> hs(self);
  Handle<mirror::Object> this_object = hs.NewHandle(raw_this_object);
  Handle<mirror::Class> cls = hs.NewHandle(this_object->GetClass());

  ArtMethod* method = nullptr;
  ImTable* imt = cls->GetImt(kRuntimePointerSize);
  uint32_t imt_index = interface_method->GetImtIndex();
  D5LOG(INFO) << __func__ << ": imt idx: " << imt_index;
  ArtMethod* conflict_method = imt->Get(imt_index, kRuntimePointerSize);
  D2LOG(WARNING) << __func__ << ": conflict method: "
    << std::hex << conflict_method
    << (conflict_method==nullptr?"<null>":conflict_method->PrettyMethod());

  if (LIKELY(conflict_method->IsRuntimeMethod())) {
    ImtConflictTable* current_table = conflict_method->GetImtConflictTable(kRuntimePointerSize);
    method = current_table->Lookup(interface_method, kRuntimePointerSize);
    D2LOG(WARNING) << __func__ << ": RT CONFLICT: "
      << (method==nullptr?"<null>":method->PrettyMethod());
  } else {
    // It seems we aren't really a conflict method!
    method = conflict_method;
  }
  if (method != nullptr) {
    D2LOG(WARNING) << __func__ << ": resolved. returning: "
      << method->GetDexMethodIndex() << ": "
      << method->PrettyMethod();

    return (void*) method;
  }

  D5LOG(INFO) << __func__ << ": not found.. continuing..";
  // No match, use the IfTable.
  method = cls->FindVirtualMethodForInterface(interface_method, kRuntimePointerSize);
  if (UNLIKELY(method == nullptr)) {
    DLOG(FATAL) << __func__
      << ": FATAL: ThrowIncompatibleClassChangeErrorClassForInterfaceDispatch";
    // ThrowIncompatibleClassChangeErrorClassForInterfaceDispatch(
    //     interface_method, this_object.Get(), caller_method);
    return nullptr;
  }

  // We arrive here if we have found an implementation, and it is not in the ImtConflictTable.
  // We create a new table with the new pair { interface_method, method }.
  ArtMethod* new_conflict_method = Runtime::Current()->GetClassLinker()->AddMethodToConflictTable(
      cls.Get(),
      conflict_method,
      interface_method,
      method,
      /*force_new_conflict_method=*/false);
  if (new_conflict_method != conflict_method) {
    // Update the IMT if we create a new conflict method. No fence needed here, as the
    // data is consistent.
    imt->Set(imt_index, new_conflict_method, kRuntimePointerSize);
  }

  D2LOG(WARNING) << __func__ << ": "
    << method->GetDexMethodIndex() << ": " << method->PrettyMethod();

  return (void*) method;
}

extern "C" void artJValueSetLFromLLVM(
  JValue* jvalue, mirror::Object* obj, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  UNUSED(self);
  LOGLLVM4(INFO) << __func__ << ": obj: " << std::hex << obj;
  LOGLLVM4(INFO) << __func__ << ": jvalue: " << std::hex << jvalue;

  DCHECK(Thread::Current() != nullptr) << "Thread is null: " << __func__;
  DCHECK(Runtime::Current() != nullptr) << "Runtime is null: " << __func__;

  jvalue->SetL(obj);
  return;
}

extern "C" void llvmPushQuickFrame(ManagedStack *fragment, Thread* self) {
  // Push a transition back into managed code onto the linked list in thread.
  self->PushManagedStackFragment(fragment);
  // LLVM will be calling quick code, so while in quick,
  // do not apply operations like the frame fixup
  SET_LLVM_CALLED_QUICK();
  D4LOG(INFO) << __func__ << ": old Fragment: "
    << LLVM::GetManagedStackInfo(self, fragment);
}

extern "C" void llvmPopQuickFrame(ManagedStack *old_fragment, Thread* self) {
  D4LOG(INFO) << __func__;
  // Pop transition.
  self->PopManagedStackFragment(*old_fragment);
  UNSET_LLVM_CALLED_QUICK();
}

// This was a workaround for LLVM but it doesn't affect anything
extern "C" void llvmClearTopOfStack(Thread* self) {
  DLOG(FATAL) << __func__ << ": dont use this";
  self->SetTopOfStack(nullptr);
}

#endif

}  // namespace art
