/**
 * This also has some verification methods implemented by mcr_dbg.cc
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
#ifndef ART_RUNTIME_LLVM_IMPL_H_
#define ART_RUNTIME_LLVM_IMPL_H_

#include "dex/invoke_type.h"
#include "jni/jni_env_ext.h"
#include "mirror/object.h"
#include "mirror/class.h"

namespace art {

union JValue;
class ShadowFrame;
class ManagedStack;

namespace mirror {
class Class;
}

extern uint32_t invoke_hist_llvm_cnt_;

namespace LLVM {

extern ArtMethod* jni_method_;

void SetJniMethod(void* vmethod);
void EnableDebugLLVM();

std::string GetManagedStackInfo(
    Thread* self, const ManagedStack* managed_stack,
    bool from_walk_stack = false);

void VerifyStackFrame(Thread* self, ManagedStack* managed_stack);
void VerifyStackFrames(Thread* self);
void VerifyThread(void* vself) REQUIRES_SHARED(Locks::mutator_lock_);
void VerifyCurrentThreadMethod() REQUIRES_SHARED(Locks::mutator_lock_);
void VerifyString(void* vstr) REQUIRES_SHARED(Locks::mutator_lock_);
void VerifyBssObject(void* bss_slot);

ALWAYS_INLINE void* _ResolveInternalMethod(ArtMethod* referrer,
    uint32_t dex_method_idx, uint32_t iinvoke_type, Thread* self)
  REQUIRES_SHARED(Locks::mutator_lock_);

ALWAYS_INLINE void* _ResolveExternalMethod(ArtMethod* caller,
    const char* dex_filename, const char* dex_location,
    uint32_t dex_method_idx, uint32_t iinvoke_type,
    Thread* self);

ALWAYS_INLINE void* _ResolveVirtualMethod(
    mirror::Object* receiver, ArtMethod* method)
  REQUIRES_SHARED(Locks::mutator_lock_);

ALWAYS_INLINE void* _ResolveInterfaceMethod(mirror::Object* receiver, ArtMethod* method, uint32_t imt_index)
  REQUIRES_SHARED(Locks::mutator_lock_);

std::string PrettyClass(ObjPtr<mirror::Class> klass);
std::string PrettyClass(mirror::Class* klass);

void workaround();
void workaroundII(int x, int y);


#if defined(ART_MCR_ANDROID_6)
void InvokeWrapper(void* vart_method, uint32_t* args, uint32_t args_size, JValue* result)
  REQUIRES_SHARED(Locks::mutator_lock_);
void ClassInit(void* vklass) REQUIRES_SHARED(Locks::mutator_lock_);

void* ResolveClass(void* vmethod, uint16_t type_idx, bool clinit_check)
  REQUIRES_SHARED(Locks::mutator_lock_);

void* InitializeStaticStorageFromCode(void* vrt, void* vcaller, uint32_t type_idx, void* vself)
    REQUIRES_SHARED(Locks::mutator_lock_);
void* InitializeTypeFromCode(void* vrt, void* vcaller, uint32_t type_idx, void* vself)
    REQUIRES_SHARED(Locks::mutator_lock_);

void* ResolveInternalMethod(void* vreferrer, uint32_t dex_method_idx,
                            uint32_t original_invoke_type_i)
  REQUIRES_SHARED(Locks::mutator_lock_);
void* ResolveExternalMethod(void* method_caller,
    const char* dex_filename, const char* dex_location,
    uint32_t dex_method_idx, uint32_t iinvoke_type);
void* ResolveVirtualMethod(void* vreceiver, void* vmethod)
  REQUIRES_SHARED(Locks::mutator_lock_);
void* ResolveInterfaceMethod(void* vreceiver, void* vmethod, uint32_t imt_index)
  REQUIRES_SHARED(Locks::mutator_lock_);

void VerifyArtMethod(void* vrt, void* art_method) REQUIRES_SHARED(Locks::mutator_lock_);
void VerifyArtObject(void* vrt, void* vobj) REQUIRES_SHARED(Locks::mutator_lock_);
void VerifyArtClass(void* vrt, void* vklass) REQUIRES_SHARED(Locks::mutator_lock_);

void TestSuspend() REQUIRES_SHARED(Locks::mutator_lock_);
#endif

void ThrowNullPointerException() REQUIRES_SHARED(Locks::mutator_lock_);

void* GetDeclaringClass(void* vmethod) REQUIRES_SHARED(Locks::mutator_lock_);

void* ResolveString(void* vreferrer, uint32_t string_idx)
    REQUIRES_SHARED(Locks::mutator_lock_);
void InvokeMethodSLOW(void* vresolved_art_method, void* vreceiver, JValue* result, ...)
    REQUIRES_SHARED(Locks::mutator_lock_);
void DebugInvoke(void* vresolved_art_method, void* vreceiver, JValue* result, ...)
    REQUIRES_SHARED(Locks::mutator_lock_);

void DebugInvokeJniMethod_allocShadowFrame(void* vart_method, void* vthis_or_class)
    REQUIRES(Locks::mutator_lock_);
void DebugInvokeJniMethod(void* vart_method, void* this_or_class)
    REQUIRES_SHARED(Locks::mutator_lock_);
void InvokeJniMethod(void* vresolved_art_method, void* vreceiver, JValue* result, ...)
    REQUIRES_SHARED(Locks::mutator_lock_);

void* AllocObject(void* vreferrer, uint32_t type_idx)
    REQUIRES_SHARED(Locks::mutator_lock_);
void* AllocObjectWithAccessChecks(void* vreferrer, uint32_t type_idx)
    REQUIRES_SHARED(Locks::mutator_lock_);
void* AllocArray(void* vreferrer, uint32_t type_idx, uint32_t length)
    REQUIRES_SHARED(Locks::mutator_lock_);
void* AllocArrayWithAccessChecks(void* vreferrer, uint32_t type_idx,
                                 uint32_t length)
    REQUIRES_SHARED(Locks::mutator_lock_);
void ArrayPutObject(void* varray, int32_t index, void* vobj)
    REQUIRES_SHARED(Locks::mutator_lock_);

void AndroidLog(int iseverity, const char* fmt, ...);
void CheckCast(void* vclass, void* vref_class)
  REQUIRES_SHARED(Locks::mutator_lock_);
bool InstanceOf(void* vclass, void* vref_class)
  REQUIRES_SHARED(Locks::mutator_lock_);
void AddToInvokeHistogram(mirror::Object* receiver, ArtMethod* caller,
                          uint32_t dex_pc, ArtMethod* method_resolved,
                          bool prtDebug = false)
  REQUIRES_SHARED(Locks::mutator_lock_);

jobject AddJniReference(JNIEnvExt* env, void* ptr)
    REQUIRES_SHARED(Locks::mutator_lock_);
void RemoveJniReference(JNIEnvExt* env, jobject jobj)
    REQUIRES_SHARED(Locks::mutator_lock_);

#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
void AddToInvokeHistogram_fromLLVM(void* vreceiver, void* vcaller_method,
    uint32_t dex_pc, void* vmethod_resolved)
  REQUIRES_SHARED(Locks::mutator_lock_);
#endif

// bool IsArtMethodHot(void* method)
//     REQUIRES_SHARED(Locks::mutator_lock_);
void InvokeWithInfo(void* vart_method, Thread* self, uint32_t* args,
    uint32_t args_size, JValue* result, const char* shorty,
    bool dont_invoke = false) REQUIRES_SHARED(Locks::mutator_lock_);
int StringCompareTo(void* str1, void* str2)
  REQUIRES_SHARED(Locks::mutator_lock_);

}  // namespace LLVM
}  // namespace art

#endif  // ART_RUNTIME_LLVM_IMPL_H_
