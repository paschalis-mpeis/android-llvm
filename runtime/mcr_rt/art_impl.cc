/**
 * These are an additional set of (traditional C code ) entrypoints for LLVM.
 * While this worked well for Android6, for Android10 there were issues while
 * changing context.
 * For this we are using assembly stub entrypoints, just like the Quick 
 * backend. In fact in some cases we are reusing the quick backend entrypoints.
 *
 * NOTE FATAL logs are probably for traditional C entrypoints that were used
 * on Android6, and now are implemented as assembly stubs.
 * To see the assembly stub entrypoints open:
 * runtime/arch/arm64/quick_entrypoints_arm64.S
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

#include "mcr_rt/art_impl.h"
#include "mcr_rt/mcr_rt.h"

#include "base/enums.h"
#include "class_linker-inl.h"
#include "class_linker.h"
#include "dex/dex_file_loader.h"
#include "entrypoints/entrypoint_utils-inl.h"
#include "entrypoints/entrypoint_utils.h"
#include "entrypoints/quick/quick_default_externs.h"
#include "interpreter/shadow_frame.h"
#include "jvalue-inl.h"
#include "managed_stack.h"
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

uint32_t invoke_hist_llvm_cnt_ = 0;

extern "C" void art_quick_invoke_stub(
    ArtMethod*, uint32_t*, uint32_t, Thread*, JValue*, const char*);
extern "C" void art_quick_invoke_static_stub(
    ArtMethod*, uint32_t*, uint32_t, Thread*, JValue*, const char*);

namespace LLVM {

ArtMethod* jni_method_ = nullptr;

// UNUSED 
void workaround() { }

void workaroundII(int x, int y) { // UNUSED
  UNUSED(x);
  UNUSED(y);
}

/**
 * @brief Workaround for LLVMtoJNI (compiler/mcr_cc/llvm/)
 *        in RT the value is used by thread.cc:GetCurrentMethod
 *        for getting the current stack frame's method, and then
 *        resolving the JNI code address to invoke.
 */
void SetJniMethod(void* vmethod) {
  D5LOG(INFO) << __func__ << ": " << std::hex << vmethod;
  jni_method_ = static_cast<ArtMethod*>(vmethod);
}

void* ResolveString(void* vreferrer, uint32_t string_idx) {
  ArtMethod* referrer = static_cast<ArtMethod*>(vreferrer);
  ObjPtr<mirror::String> resolved_string =
      ResolveStringFromCode(referrer, dex::StringIndex(string_idx));
  return static_cast<void*>(resolved_string.Ptr());
}

jobject AddJniReference(JNIEnvExt* env, void* vobj) {
  D4LOG(INFO) << "   "<<__func__<<": " << std::hex << vobj;
  mirror::Object* obj = reinterpret_cast<mirror::Object*>(vobj);
  return env->AddLocalReference<jobject>(obj);
}

void RemoveJniReference(JNIEnvExt* env, jobject jobj) {
  D4LOG(INFO) << "   "<<__func__<<": " << std::hex << jobj;
  if (jobj != nullptr) { env->DeleteLocalRef(jobj); }
}

void* AllocObject(void* vreferrer, uint32_t type_idx) {
  Thread* self = GetLLVMThreadPointer();
  ArtMethod* referrer = static_cast<ArtMethod*>(vreferrer);
  D2LOG(INFO) << "TODO_LLVM: AllocObject: " << referrer->PrettyMethod();

  gc::AllocatorType allocator =
    Runtime::Current()->GetHeap()->GetCurrentAllocator();

  ObjPtr<mirror::Class> klass =
    ResolveVerifyAndClinit(dex::TypeIndex(type_idx),
        referrer, self, false, false);

  ObjPtr<mirror::Object> newobj = 
    AllocObjectFromCode<true>(klass.Ptr(), self, allocator);

  D2LOG(INFO) << "object allocated!";
  return static_cast<void*>(newobj.Ptr());
}

void* AllocObjectWithAccessChecks(void* vreferrer, uint32_t type_idx) {
  D1LOG(INFO) << __func__ << ": CHECK_LLVM";
  Thread* self = GetLLVMThreadPointer();
  ArtMethod* referrer = static_cast<ArtMethod*>(vreferrer);
  D2LOG(INFO) << __func__ << ": ref: " << std::hex << referrer;
  D2LOG(INFO) << __func__ << ": " << referrer->PrettyMethod();

  gc::AllocatorType allocator =
    Runtime::Current()->GetHeap()->GetCurrentAllocator();
  ObjPtr<mirror::Class> klass =
    ResolveVerifyAndClinit(dex::TypeIndex(type_idx),
        referrer, self, false, false);

  // kInstrumented is true, otherwise it fails
  ObjPtr<mirror::Object> newobj = 
      AllocObjectFromCode<true>(klass.Ptr(), self, allocator);

  return static_cast<void*>(newobj.Ptr());
}

void* AllocArray(void* vreferrer, uint32_t type_idx, uint32_t length) {
  D2LOG(INFO) << __func__;
  Thread* self = GetLLVMThreadPointer();
  ArtMethod* referrer = static_cast<ArtMethod*>(vreferrer);
  D2LOG(INFO) << "AllocArray: " << referrer->PrettyMethod()
              << " count: " << std::to_string(length);
  dex::TypeIndex index(static_cast<uint32_t>(type_idx));
  ObjPtr<mirror::Array> newarray =
      AllocArrayFromCode<false, true>(
          index, length, referrer, self,
          Runtime::Current()->GetHeap()->GetCurrentAllocator());

  D2LOG(INFO) << "AllocArray: " << referrer->PrettyMethod() << "[done]";
  return static_cast<void*>(newarray.Ptr());
}

void* AllocArrayWithAccessChecks(void* vreferrer, uint32_t type_idx,
                                 uint32_t length) {
  DLOG(INFO) << __func__;
  Thread* self = GetLLVMThreadPointer();
  ArtMethod* referrer = static_cast<ArtMethod*>(vreferrer);
  D2LOG(INFO) << "AllocArrayWithAccessChecks: " << referrer->PrettyMethod();
  dex::TypeIndex index(static_cast<uint32_t>(type_idx));

  ObjPtr<mirror::Array> newarray =
      AllocArrayFromCode<true, true>(
          index, length, referrer, self,
          Runtime::Current()->GetHeap()->GetCurrentAllocator());
  return static_cast<void*>(newarray.Ptr());
}

void ArrayPutObject(void* varray, int32_t index, void* vobj) {
  DLOG(FATAL) << __func__ << " use art_quick_aput_obj entrypoint";
  art::mirror::Array* array = static_cast<art::mirror::Array*>(varray);
  mirror::Object* obj = static_cast<mirror::Object*>(vobj);
  art_quick_aput_obj(array, index, obj);
}

inline bool ClassAssignable(void* vclass, void* vref_class)
  REQUIRES_SHARED(Locks::mutator_lock_) {
  DLOG(FATAL) << __func__;
    mirror::Class* klass = reinterpret_cast<mirror::Class*>(vclass);
  mirror::Class* ref_class = reinterpret_cast<mirror::Class*>(vref_class);

  return ref_class->IsAssignableFrom(klass);
}

void CheckCast(void* vclass, void* vref_class) {
  DLOG(FATAL) << __func__;
  if (!ClassAssignable(vclass, vref_class)) {
    DLOG(FATAL) << "CheckCast: FAILED!";
  }
}

bool InstanceOf(void* vclass, void* vref_class) {
  DLOG(FATAL) << __func__;
  return ClassAssignable(vclass, vref_class);
}

void AddToInvokeHistogram(mirror::Object* receiver, ArtMethod* caller,
                          uint32_t dex_pc, ArtMethod* method_resolved,
                          bool prtDebug) {
  D3CHECK(method_resolved != nullptr);
  D3CHECK(caller!= nullptr);
  const DexFile* dex_file = method_resolved->GetDexFile();

  D3CHECK(dex_file != nullptr) << ": DexFile from method is null: " << method_resolved->PrettyMethod();
  D3CHECK(receiver->GetClass() != nullptr) << ": class of receiver is null";
  D3CHECK(caller->GetDeclaringClass() != nullptr) << ": caller declaring class is null";
  std::string dex_location = dex_file->GetLocation();
  std::string dex_base_location = DexFileLoader::GetBaseLocation(dex_location);
  std::string dex_location_caller = caller->GetDexFile()->GetLocation();

#ifdef CRDEBUG3
  prtDebug = true;
#endif

  if(receiver->GetClass()->GetDexTypeIndex().index_
      != caller->GetDeclaringClass()->GetDexTypeIndex().index_) {
    if (UNLIKELY(prtDebug)) {
      D3LOG(WARNING) << "Caller and receiver have different classes. "
        << "Caller could be an interface."
        << "\n  Caller Class: "
        << LLVM::PrettyClass(caller->GetDeclaringClass().Ptr())
        << "\nReceiver Class: "
        << LLVM::PrettyClass(receiver->GetClass());
    }
  }

  if (UNLIKELY(prtDebug)) {
    std::string extra = ":" + std::to_string(method_resolved->GetDexMethodIndex());
    mcr::LogSeverity lvl = mcr::LogSeverity::INFO;
    if (method_resolved->IsNative()) {
      extra = " native";
      lvl=mcr::LogSeverity::ERROR;
    } else if (method_resolved->IsAbstract()) {
      extra = " abstract";
      lvl=mcr::LogSeverity::ERROR;
    }

    DLOG(lvl) << __func__ << ""
      << "\ncaller: " << caller->GetDexMethodIndex() << ":" << caller->PrettyMethod()
      << "\n  Spec: " << method_resolved->GetDexMethodIndex() << ":" << method_resolved->PrettyMethod()
      << "\nunique_id: " << std::to_string(caller->GetDexMethodIndex()) << "|" << std::to_string(dex_pc)
      // << "\nSpec: " << method_resolved->PrettyMethod() << extra
      << "\nClass:" << LLVM::PrettyClass(receiver->GetClass())
      << "\nInvokeType: " << method_resolved->GetInvokeType()
      << "\nDexLocation: " << dex_location
      << "\nDexLocation:Base: " << dex_base_location;
  }

  mcr::InvokeInfo info(caller->GetDexMethodIndex(),
      dex_pc,
      receiver->GetClass()->GetDexTypeIndex().index_,
      method_resolved->GetDexMethodIndex(),
      method_resolved->GetInvokeType(),
      dex_location_caller,
      dex_base_location,
      dex_location);
  mcr::InvokeInfo::AddToCache(info);
}

#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
void AddToInvokeHistogram_fromLLVM(void* vreceiver, void* vcaller_method,
                                   uint32_t dex_pc, void* vmethod_resolved) {
  D3LOG(ERROR) << __func__ << ": VERIFY_LLVM";
  mirror::Object* receiver = static_cast<mirror::Object*>(vreceiver);
  ArtMethod* caller = reinterpret_cast<ArtMethod*>(vcaller_method);
  ArtMethod* method_resolved = reinterpret_cast<ArtMethod*>(vmethod_resolved);

  // INFO intentionally show a warning.
  // Updating the histogram file is gonna be slow from LLVM anyway,
  // so we let the use know this (adb logged)..
  if(++invoke_hist_llvm_cnt_ < 10) {
    DLOG(WARNING) << __func__ << "|"
      << std::to_string(dex_pc) << ": "
      << method_resolved->GetDexMethodIndex()
      << " (" << method_resolved->PrettyMethod() << ")";
  }

  const bool dbg=false;
  AddToInvokeHistogram(receiver, caller, dex_pc, method_resolved, dbg);
}
#endif

void AndroidLog(int iseverity, const char* fmt, ...) {
  ::android::base::LogSeverity severity = static_cast<::android::base::LogSeverity>(iseverity);
  va_list args;
  va_start(args, fmt);
  LLVM_LOG(severity, fmt, args);
  va_end(args);
}

void ThrowNullPointerException() {  // it crashes
  DLOG(INFO) << "CHECK_LLVM: " << __func__;
  ThrowNullPointerExceptionFromDexPC();
}

void DebugInvoke(void* vresolved_art_method, void* vreceiver, JValue* result, ...) {
  Thread* self = Thread::Current();
  ArtMethod* art_method = static_cast<ArtMethod*>(vresolved_art_method);
  mirror::Object* receiver = static_cast<mirror::Object*>(vreceiver);
  DLOG(ERROR) << "DebugInvoke:" << art_method->GetDexMethodIndex()
             << ":" << art_method->PrettyMethod()
             << "/" << art_method->GetDexFile()->GetLocation();

  va_list args;
  va_start(args, result);

  const dex::CodeItem* code_item = art_method->GetCodeItem();
  D3CHECK(code_item != nullptr) << "InvokeMethod: empty code_item for: "
                                << art_method->PrettyMethod();

  uint32_t shorty_len = 0;
  const char* shorty = art_method->GetInterfaceMethodIfProxy(kRuntimePointerSize)
                           ->GetShorty(&shorty_len);

  ScopedObjectAccessUnchecked soa(self);
  mcr::ArgArray arg_array(shorty, shorty_len);
  arg_array.BuildArgArrayFromVarArgs(soa, receiver, args);

  arg_array.DebugInvoke(soa, art_method, result, shorty);
  va_end(args);

  DLOG(INFO) << "DebugInvoke: [returned]: " << art_method->PrettyMethod();
}



void InvokeWithInfo(void* vart_method, Thread* self, uint32_t* args, uint32_t args_size,
                    JValue* result, const char* shorty, bool dont_invoke) {
  ArtMethod* art_method = static_cast<ArtMethod*>(vart_method);
  uint32_t args_num = strlen(shorty);
  DLOG(INFO) << "InvokeWithInfo: " << art_method->PrettyMethod()
            << (art_method->IsStatic() ? "STATIC:" : "") << ":"
            << shorty << ":ARGS:" << args_num << ":sz:" << args_size;

  bool is_static = art_method->IsStatic();
  uint32_t i = 0;

  if (!is_static) {
    uint32_t receiver = args[i];
    DLOG(INFO) << "receiver:" << receiver;
    i++;
  }

  for (; i < args_num; i++) {
    std::string val;
    uint32_t v32;

    std::stringstream ss;
    ss << "arg:" << i << ":" << Primitive::GetType(shorty[i]) << ":";

    switch (shorty[i]) {
      case 'Z':
      case 'B':
      case 'C':
      case 'S':
      case 'I':
      case 'F':
      case 'L':
        v32 = args[i];
        val = std::to_string(v32);
        break;
      case 'D':
      case 'J':
        v32 = args[i];
        val = std::to_string(v32);
        i++;
        v32 = args[i];
        val += "_" + std::to_string(v32);
        break;

      case 'V':
        continue;
    }
    DLOG(INFO) << ss.str() << val;
  }

  if (!dont_invoke) {
    art_method->Invoke(self, args, args_size, result, shorty);
    DLOG(INFO) << "[returned] InvokeWithInfo: " << art_method->PrettyMethod();
  }
}

int StringCompareTo(void* str1, void* str2) {
  D4LOG(INFO) << __func__;
  mirror::String* string1 = reinterpret_cast<mirror::String*>(str1);
  mirror::String* string2 = reinterpret_cast<mirror::String*>(str2);
#ifdef CRDEBUG2
  if(string1 == nullptr) { DLOG(ERROR) << __func__ << "str1: is null"; }
  if(string2 == nullptr) { DLOG(ERROR) << __func__ << "str2: is null"; }
#endif
  int val = string1->CompareTo(string2);
  return val;
}

}  // namespace LLVM
}  // namespace art
