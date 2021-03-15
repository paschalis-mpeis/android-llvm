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
#include "mcr_rt/art_impl.h"
#include "mcr_rt/mcr_rt.h"  // needed by art_impl_asm_*.h

#include "base/enums.h"
#include "class_linker-inl.h"
#include "class_linker.h"
#include "dex/dex_file_loader.h"
#include "entrypoints/entrypoint_utils-inl.h"
#include "entrypoints/entrypoint_utils.h"
#include "entrypoints/quick/quick_default_externs.h"
#include "interpreter/shadow_frame.h"
#include "jvalue-inl.h"
#include "mcr_rt/art_impl-inl.h"
#include "mcr_rt/art_impl_arch-inl.h"
#include "mcr_rt/invoke.h"
#include "mcr_rt/invoke_info.h"
#include "mcr_rt/mcr_dbg.h"
#include "mcr_rt/utils.h"
#include "mirror/class_loader.h"
#include "mirror/object-inl.h"
#include "nativehelper/scoped_local_ref.h"
#include "runtime.h"
#include "thread-inl.h"
#include <bionic_tls.h>  // Access to our own TLS slot.

namespace art {

namespace LLVM {
#ifdef ART_MCR_ANDROID_6
void TestSuspend() {
  DBCSC_RunCheckSuspend();
  art_quick_test_suspend();
}

inline void _ClassInit(mirror::Class* klass, Thread* self)
  REQUIRES_SHARED(Locks::mutator_lock_) {
  ClassLinker* linker = Runtime::Current()->GetClassLinker();
  if (!klass->IsInitialized()) {
    StackHandleScope<1> hs(self);
    Handle<mirror::Class> h_class(hs.NewHandle(klass));
    linker->EnsureInitialized(self, h_class, true, true);
  }
}

void ClassInit(void* vklass) {
  Thread* self = GetLLVMThreadPointer();
  mirror::Class* klass = static_cast<mirror::Class*>(vklass);
  DIE_ANDROID10_ENTRYPOINT();
  _ClassInit(klass, self);
}

void* ResolveClass(void* vmethod, uint16_t type_idx, bool clinit_check) {
  DIE_ANDROID10_ENTRYPOINT();
  Thread* self = GetLLVMThreadPointer();
  ArtMethod* referrer = static_cast<ArtMethod*>(vmethod);
  ClassLinker* linker = Runtime::Current()->GetClassLinker();
  dex::TypeIndex index(static_cast<uint32_t>(type_idx));
  ObjPtr<mirror::Class> klass = linker->ResolveType(index, referrer);

  if (UNLIKELY(clinit_check)) {
    _ClassInit(klass.Ptr(), self);
  }

  return static_cast<void*>(klass.Ptr());
}

void* InitializeStaticStorageFromCode(void* vrt, void* vcaller, uint32_t type_idx, void* vself) {
  UNUSED(vrt);
  ArtMethod* caller = static_cast<ArtMethod*>(vcaller);
  Thread* self = reinterpret_cast<Thread*>(vself);
  dex::TypeIndex index(static_cast<uint32_t>(type_idx));
  return ResolveVerifyAndClinit(index, caller, self, true, false).Ptr();
}

/**
 * @brief Based on artResolveTypeFromCode
 */
void* InitializeTypeFromCode(void* vrt, void* vcaller, uint32_t type_idx, void* vself) {
  DLOG(INFO) << "CHECK_LLVM: InitializeTypeFromCode";
  UNUSED(vrt);
  ArtMethod* caller = static_cast<ArtMethod*>(vcaller);
  Thread* self = reinterpret_cast<Thread*>(vself);
  dex::TypeIndex tidx=dex::TypeIndex(type_idx);

  ObjPtr<mirror::Class> result = ResolveVerifyAndClinit(
      tidx, caller, self,
      /* can_run_clinit= */ false,
      /* verify_access= */ false);
  return result.Ptr();
}

/**
 * @brief Resolve a method that belongs to the same dex file (classes.dex)
 *        with the method_caller
 */
void* ResolveInternalMethod(void* vreferrer, uint32_t dex_method_idx,
    uint32_t iinvoke_type) {
  D2LOG(INFO) << "ResolveInternalMethod:";
  ArtMethod* referrer = static_cast<ArtMethod*>(vreferrer);
  return _ResolveInternalMethod(referrer, dex_method_idx, iinvoke_type,
      // self: r9 had issues sometimes here..
      Thread::Current());
}

void* ResolveExternalMethod(void* method_caller,
                            const char* dex_filename, const char* dex_location,
                            uint32_t dex_method_idx, uint32_t iinvoke_type) {
  DIE_ANDROID10_ENTRYPOINT();
  Thread* self = Thread::Current();  // slower but won't timed so we don't care
  // but we don't care, because resolving methods is before we time HF execution
  ArtMethod* caller = static_cast<ArtMethod*>(method_caller);

  return _ResolveExternalMethod(caller,
      dex_filename, dex_location,
      dex_method_idx, iinvoke_type, self);
}

void* ResolveVirtualMethod(void* vreceiver, void* vmethod) {
  DIE_ANDROID10_ENTRYPOINT();
  D2LOG(INFO) << "ResolveVirtualMethod: vreceiver:" << vmethod;
  ArtMethod* method = static_cast<ArtMethod*>(vmethod);
  mirror::Object* receiver = static_cast<mirror::Object*>(vreceiver);

  return _ResolveVirtualMethod(receiver, method);
}

void* ResolveInterfaceMethod(void* vreceiver, void* vmethod, uint32_t imt_index) {
  DIE_ANDROID10_ENTRYPOINT();
  mirror::Object* receiver = static_cast<mirror::Object*>(vreceiver);
  ArtMethod* method = static_cast<ArtMethod*>(vmethod);
  D3LOG(INFO) << "ResolveInterfaceMethod:" << method->PrettyMethod();

  return _ResolveInterfaceMethod(receiver, method, imt_index);
}

/**
 * @brief 
 *
 * @param vart_method
 * @param args
 * @param args_size
 * @param result
 */
void InvokeWrapper(void* vart_method, uint32_t* args, uint32_t args_size, JValue* result) {
  Thread* self = GetLLVMThreadPointer();
#define FAST_QUICK_INVOKE
#ifdef FAST_QUICK_INVOKE
  ArtMethod* art_method = static_cast<ArtMethod*>(vart_method);
  LOGDBG(INFO) << "InvokeWrapper: " << art_method->PrettyMethod();
  DIE_ANDROID10_ENTRYPOINT();
  const char* shorty = art_method->GetShorty();

  if (!art_method->IsStatic()) {
    (*art_quick_invoke_stub)(art_method, args, args_size, self, result, shorty);
  } else {
    (*art_quick_invoke_static_stub)(art_method, args, args_size, self, result, shorty);
  }
  LOGDBG(INFO) << "InvokeWrapper: " << art_method->PrettyMethod() << "[DONE]";
#else
  DLOG(INFO) << "InvokeWrapper: InvokeWithInfo(SUPERSLOW!)";
  InvokeWithInfo(vart_method, self, args, args_size, result, shorty);
#endif
}
#endif
}  // namespace LLVM
}  // namespace art
