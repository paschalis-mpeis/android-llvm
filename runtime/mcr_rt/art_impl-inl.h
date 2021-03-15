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
#ifndef ART_RUNTIME_LLVM_IMPL_INL_H
#define ART_RUNTIME_LLVM_IMPL_INL_H

#include "mcr_rt/art_impl.h"
#include "mcr_rt/mcr_rt.h"

#include "art_method.h"
#include "class_linker.h"
#include "handle_scope.h"
#include "imtable.h"
#include "mirror/class.h"
#include "runtime.h"
#include "scoped_thread_state_change.h"
#include "thread.h"

namespace art {
class Thread;
namespace LLVM {

inline void* _ResolveInternalMethod(ArtMethod* referrer, uint32_t dex_method_idx,
    uint32_t iinvoke_type, Thread* self) {
  InvokeType invoke_type = static_cast<InvokeType>(iinvoke_type);
  LOGLLVM4(INFO) << __func__ << ": referrer: " << referrer->PrettyMethod();

  ClassLinker* linker = Runtime::Current()->GetClassLinker();

  ArtMethod* resolved_method =
    linker->ResolveMethod<__resolve_mode>
    (self, dex_method_idx, referrer, invoke_type);
  if (UNLIKELY(resolved_method == nullptr)) {

    if (self->IsExceptionPending()) {
      self->ClearException();
    }

    // BUGFIX: if it cannot be found and it's direct (sharpened/divirtualized),
    // then try to resolve again w/ virtual mode
    if (invoke_type == kDirect) {
      invoke_type = kVirtual;
    }

    resolved_method = linker->ResolveMethod<__resolve_mode>(self, dex_method_idx, referrer, invoke_type);
    CHECK(resolved_method != nullptr) << "Failed to resolve method: " << dex_method_idx;
  }

  LOGLLVM3(INFO) << __func__ << ": Found: " << resolved_method->PrettyMethod();

  return static_cast<void*>(resolved_method);
}

inline void* _ResolveExternalMethod(ArtMethod* caller,
                            const char* dex_filename, const char* dex_location,
                            uint32_t dex_method_idx, uint32_t iinvoke_type,
                            Thread* self) {
  ClassLinker* linker = Runtime::Current()->GetClassLinker();
  InvokeType invoke_type = static_cast<InvokeType>(iinvoke_type);
  const DexFile* dex_file = ::art::mcr::McrRT::OpenDexFileANY(dex_filename, dex_location);

  ScopedObjectAccess soa(self);
  StackHandleScope<4> hs(soa.Self());
  // INFO issues w/ framework methods when using caller's dex_cache
  ObjPtr<mirror::DexCache> dex_cache = linker->FindDexCache(self, *dex_file);
  Handle<mirror::DexCache> h_dex_cache(hs.NewHandle(dex_cache.Ptr()));
  Handle<mirror::ClassLoader> class_loader(hs.NewHandle(
      caller->GetClassLoader()));

  ArtMethod* resolved_method = linker->ResolveMethod<__resolve_mode>(
      dex_method_idx, h_dex_cache, class_loader, caller, invoke_type);

#ifdef CRDEBUG2
  if (UNLIKELY(resolved_method == nullptr)) {
    DLOG(FATAL) << "Failed to resolve method: "
      "\ncaller: " << caller->PrettyMethod()
      << "\nidx: " << dex_method_idx << " DexFile: "
      << dex_file->GetLocation() << " " << invoke_type;
  }
#endif

  D4LOG(INFO) << __func__ << ":" << invoke_type
              << " :" << resolved_method->PrettyMethod() << "[done]";
  return static_cast<void*>(resolved_method);
}

inline void* _ResolveVirtualMethod(
    mirror::Object* receiver, ArtMethod* method) {
  D4LOG(INFO) << __func__ << ": receiver:" << method;

  mirror::Class* klass = receiver->GetClass();
  ArtMethod* resolved_method =
      klass->FindVirtualMethodForVirtualOrInterface(method, kRuntimePointerSize);

  D4LOG(INFO) << __func__ << ": Found: " << resolved_method->PrettyMethod();
  return static_cast<void*>(resolved_method);
}


inline void* _ResolveInterfaceMethod(mirror::Object* receiver, ArtMethod* method, uint32_t imt_index) {
  D4LOG(INFO) << __func__ << ":" << method->PrettyMethod();

  mirror::Class* klass = receiver->GetClass();
  ArtMethod* resolved_interface = nullptr;

  ArtMethod* imt_method = klass->GetImt(kRuntimePointerSize)
    ->Get(imt_index, kRuntimePointerSize);

  if(!imt_method->IsRuntimeMethod()) {
    D3LOG(INFO) << "Found IMT METHOD: " << imt_method->PrettyMethod();
    resolved_interface = imt_method;
  } else {
    resolved_interface =
        klass->FindVirtualMethodForVirtualOrInterface(method, kRuntimePointerSize);
    if (UNLIKELY(resolved_interface == nullptr)) {
      DLOG(FATAL) << "Failed to find interface method";
    }
  }

  D3LOG(INFO) << __func__ << ":"
    << " Found: " << resolved_interface->PrettyMethod();

  return static_cast<void*>(resolved_interface);
}

}  // namespace LLVM
}  // namespace art

#endif  // ifndef ART_RUNTIME_LLVM_IMPL_INL_H
