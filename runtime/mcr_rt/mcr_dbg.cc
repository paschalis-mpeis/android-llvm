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
#include "mcr_rt/mcr_dbg.h"

#include "mcr_rt/art_impl.h"
#include "mcr_rt/mcr_rt.h"

#include "art_method-inl.h"
#include "art_method.h"
#include "mcr_rt/art_impl_arch-inl.h"
#include "mcr_rt/invoke.h"
#include "mirror/object-inl.h"
#include "scoped_thread_state_change.h"
#include "scoped_thread_state_change-inl.h"
#include "stack.h"
#include "thread-inl.h"

namespace art {

bool __dbg_runtime_ = false; 

namespace LLVM {

std::string PrettyClass(ObjPtr<mirror::Class> klass) {
  Thread* self = Thread::Current();
  Locks::mutator_lock_->SharedLock(self);
  mirror::Class* ptr=klass.Ptr();
  Locks::mutator_lock_->SharedUnlock(self);
  return PrettyClass(ptr);
}
std::string PrettyClass(mirror::Class* klass) {
  if(klass==nullptr) {
    return "<null class>";
  } else {
    Thread* self = Thread::Current();
    Locks::mutator_lock_->SharedLock(self);
    std::string pretty = std::to_string(klass->GetDexTypeIndex().index_)
      + ":" + klass->PrettyClass();
    Locks::mutator_lock_->SharedUnlock(self);
    if(pretty.find("<") == std::string::npos) {
      return pretty;
    }

    // remove: 'java.lang.Class<'
    pretty.erase(0, pretty.find("<")+1);
    // remove last character: '>'
    pretty.pop_back();

    return pretty;
  }
}

void EnableDebugLLVM() {
#ifdef CRDEBUG2
    dbg_llvm_code_ = true;
    DLOG(WARNING) << "===== WARNING: ENABLED LLVMDBG code in RT =====";
#endif
  }

  void* GetDeclaringClass(void* vmethod) {
    DLOG(WARNING) << __func__ << ": SLOW!";
    ArtMethod* method = static_cast<ArtMethod*>(vmethod);
    return method->GetDeclaringClass().Ptr();
  }

  void VerifyThread(void* vself)
    REQUIRES_SHARED(Locks::mutator_lock_) {
      Thread* selfLLVM = reinterpret_cast<Thread*>(vself);
      Thread* selfxTR = GetLLVMThreadPointer();
      std::string tn1, tn2;

      DLOG(INFO) << "VerifyThread:      vself: " << std::hex << vself;
      DLOG(INFO) << "VerifyThread:    selfxTR: " << std::hex << selfxTR;
      selfLLVM->GetThreadName(tn1);
      selfLLVM->GetThreadName(tn2);
      DLOG(INFO) << "VerifyThread:   vself:ID: "
        << tn1 << " " << selfLLVM->GetTid();
      DLOG(INFO) << "VerifyThread: selfxTR:ID: "
        << tn2 << " " << selfLLVM->GetTid();
    }

  void VerifyCurrentThreadMethod() {
      // CHECK_LLVM it crashes? we need Runtime* here
      Thread* self = GetLLVMThreadPointer();
      ArtMethod* method = self->GetCurrentMethod(nullptr);
      DLOG(INFO) << __func__ << ": " << method->PrettyMethod();
    }

  void VerifyString(void* vstr) {
    mirror::String *string = static_cast<mirror::String*>(vstr);
    DLOG(INFO) << __func__ << ": '"
      << string->ToModifiedUtf8() << "'";
  }

  void VerifyBssObject(void* bss_slot) {
    // https://godbolt.org/z/MeGzfj
    if(bss_slot!=nullptr) {
      DLOG(ERROR) << __func__ << ": BSS Obj: "
        << std::hex << (void*)(*(void**)(bss_slot))
        << "(slot: " << bss_slot << ")";
    } else {
      DLOG(ERROR) << __func__ << ": BSS Slot: <null>";
    }
  }

/**
 * UNUSED, slow.
 *  Call interpterter/quick methods
 *
 */
void InvokeMethodSLOW(void* vresolved_art_method, void* vreceiver, JValue* result, ...) {
  Thread* self = Thread::Current();
  ArtMethod* art_method = static_cast<ArtMethod*>(vresolved_art_method);
  mirror::Object* receiver = static_cast<mirror::Object*>(vreceiver);
  D3LOG(WARNING) << "InvokeMethodSLOW:" << art_method->GetDexMethodIndex()
                  << ":" << art_method->PrettyMethod()
                  << "/" << art_method->GetDexFile()->GetLocation();

  D3CHECK(art_method->IsStatic() ||
          (!art_method->IsStatic() && receiver != nullptr))
      << "receiver can't be null on instance methods: "
      << art_method->PrettyMethod();

  va_list args;
  va_start(args, result);

  const dex::CodeItem* code_item = art_method->GetCodeItem();
  D3CHECK(code_item != nullptr) << "InvokeMethod: empty code_item for: " << art_method->PrettyMethod();

  ManagedStack fragment;
  self->PushManagedStackFragment(&fragment);

  uint32_t shorty_len = 0;
  const char* shorty = art_method->GetInterfaceMethodIfProxy(kRuntimePointerSize)
                           ->GetShorty(&shorty_len);

  ScopedObjectAccessUnchecked soa(self);
  mcr::ArgArray arg_array(shorty, shorty_len);
  arg_array.BuildArgArrayFromVarArgs(soa, receiver, args);

  arg_array.Invoke(soa, art_method, result, shorty);
  va_end(args);

  D3LOG(INFO) << "InvokeMethod: [returned]: " << art_method->PrettyMethod();
  self->PopManagedStackFragment(fragment);
}

  /**
   * @brief UNUSED Slow way of invoking jni directly thorugh RT (using var args)
   *
   * @param vresolved_art_method
   * @param vreceiver
   * @param result
   * @param ...
   */
  void InvokeJniMethod(void* vresolved_art_method, void* vreceiver, JValue* result, ...) {
    Thread* self = GetLLVMThreadPointer();

    ArtMethod* art_method = static_cast<ArtMethod*>(vresolved_art_method);
    mirror::Object* receiver = static_cast<mirror::Object*>(vreceiver);
    D3LOG(INFO) << "InvokeJniMethod: " << art_method->PrettyMethod();

    va_list args;
    va_start(args, result);

    uint16_t num_regs = ArtMethod::NumArgRegisters(art_method->GetShorty());
    ShadowFrameAllocaUniquePtr shadow_frame_unique_ptr =
      CREATE_SHADOW_FRAME(num_regs, /* link= */ nullptr, art_method, /* dex_pc= */ 0);
    ShadowFrame* shadow_frame = shadow_frame_unique_ptr.get();


    ManagedStack fragment;
    self->PushManagedStackFragment(&fragment);
    self->PushShadowFrame(shadow_frame);

    uint32_t shorty_len = 0;
    const char* shorty = art_method->GetInterfaceMethodIfProxy(kRuntimePointerSize)
      ->GetShorty(&shorty_len);

    ScopedObjectAccessUnchecked soa(self);
    mcr::ArgArray arg_array(shorty, shorty_len);
    arg_array.BuildArgArrayFromVarArgs(soa, receiver, args);
    arg_array.Invoke(soa, art_method, result, shorty);
    va_end(args);

    D3LOG(INFO) << "InvokeJniMethod: [returned]";
    self->PopManagedStackFragment(fragment);
  }

  /**
   * @brief UNUSED Debug method
   *
   * @param vart_method
   * @param vthis_or_class
   */
  void DebugInvokeJniMethod_allocShadowFrame(void* vart_method, void* vthis_or_class) {
    DLOG(ERROR) << "DebugInvokeJniMethod_allocShadowFrame";
    ArtMethod* art_method = static_cast<ArtMethod*>(vart_method);

    Thread* self = Thread::Current();  // INFO using r9 register sometimes fail
    // Create a new ShadowFrame
    uint16_t num_regs = ArtMethod::NumArgRegisters(art_method->GetShorty());

    ShadowFrameAllocaUniquePtr shadow_frame_unique_ptr =
      CREATE_SHADOW_FRAME(num_regs, /* link= */ nullptr, art_method, /* dex_pc= */ 0);
    ShadowFrame* shadow_frame = shadow_frame_unique_ptr.get();
    shadow_frame->SetMethod(art_method);

    ManagedStack fragment;
    self->PushManagedStackFragment(&fragment);
    self->PushShadowFrame(shadow_frame);

    DebugInvokeJniMethod(vart_method, vthis_or_class);

    self->PopManagedStackFragment(fragment);
  }

  void DebugInvokeJniMethod(void* vart_method, void* vthis_or_class) {
    DLOG(ERROR) << "DebugInvokeJniMethod: DIJM";
    ArtMethod* art_method = static_cast<ArtMethod*>(vart_method);
    std::string pretty_method = art_method->PrettyMethod();
    DLOG(ERROR) << "art_method:" << pretty_method;
    DLOG(ERROR) << "DebugInvokeJniMethod: " << pretty_method;

    Thread* self = Thread::Current();  // INFO using r9 register sometimes fail
    DLOG(ERROR) << "DIJM: art_method: " << std::hex << art_method;
    DLOG(ERROR) << "DIJM: Thread: " << std::hex << self;
    DLOG(ERROR) << "DIJM: JNIEnv: " << std::hex << self->GetJniEnv();
    DLOG(ERROR) << "DIJM: fp: " << std::hex << art_method->GetEntryPointFromJni();
    DLOG(ERROR) << "DIJM: this_or_class: " << std::hex << vthis_or_class;

    const char* shorty = art_method->GetShorty();
    std::string shortystr(shorty);

    ScopedObjectAccessUnchecked soa(self);
    JNIEnv* env = self->GetJniEnv();
    mirror::Object* this_or_class = static_cast<mirror::Object*>(vthis_or_class);
    if (pretty_method.compare("int mp.paschalis.toyjni.MainActivity.CalculateStuff(int, int)") == 0) {
      typedef int(fntype)(JNIEnv*, jobject, int, int);
      fntype* const fn = reinterpret_cast<fntype*>(art_method->GetEntryPointFromJni());

      int res = 0;
      CHECK(!art_method->IsStatic());
      DLOG(INFO) << "DebugInvokeJniMethod: instance JNI call";
      jobject jreceiver = soa.AddLocalReference<jobject>(this_or_class);
      DLOG(INFO) << "DebugInvokeJniMethod: got jrececeiver, etc";
      res = fn(env, jreceiver, 10, 15);
      DLOG(INFO) << "RETURNED :" << std::to_string(res);

    } else if (pretty_method.compare("void java.lang.System.arraycopy!(java.lang.Object, int, java.lang.Object, int, int)") == 0) {
      DLOG(INFO) << "DIJM: returns..";
    } else if (pretty_method.compare("int mp.paschalis.toyjni.MainActivity.Method2(int, int, int)") == 0) {
    }

    DLOG(ERROR) << "DebugInvokeJniMethod: " << pretty_method << " [SUCCESS!]";
  }

#if defined(ART_MCR_ANDROID_6)
  void VerifyArtMethod(void* vrt, void* vmethod) 
    REQUIRES_SHARED(Locks::mutator_lock_)
    {
      ArtMethod* method = reinterpret_cast<ArtMethod*>(vmethod);

      std::string pretty_method;
      if(method==nullptr) {
        pretty_method="<null>";
      } else {
        pretty_method = method->PrettyMethod();
      }
      DLOG(INFO) << "VAM: Method: " << pretty_method
        << " (" << std::hex << vmethod << ")";
      ObjPtr<mirror::Class> klass = method->GetDeclaringClass();
      DLOG(INFO) << "VAM: Class:" << klass->PrettyClass()
      << " (" << std::hex << klass << ")";
    }

  void VerifyArtClass(void* vrt, void* vklass)
    REQUIRES_SHARED(Locks::mutator_lock_)
    {
      DLOG(INFO) << "VAC: vclass: " << std::hex << vklass;

      mirror::Class* klass = reinterpret_cast<mirror::Class*>(vklass);

      std::string pretty_class;
      if(klass==nullptr) {
        pretty_class="<null>";
      } else {
        pretty_class=klass->PrettyClass();
      }
      DLOG(INFO) << "VAC: " << pretty_class 
        << " (" << std::hex << vklass << ")";
    }

  void VerifyArtObject(void* vrt, void* vobj)
    REQUIRES_SHARED(Locks::mutator_lock_)
    {
      mirror::Object* obj = reinterpret_cast<mirror::Object*>(vobj);
      mirror::Class* klass = obj->GetClass();

      std::string pretty_class;
      if(obj==nullptr) {
        pretty_class="<obj: null>";
      } else if(klass==nullptr) {
        pretty_class="<obj class: null>";
      } else {
        pretty_class=klass->PrettyClass();
      }
      DLOG(INFO) << "VAO: " << klass->PrettyClass()
        << " (class: " << std::hex << klass << ", object: "
        << std::hex << obj << ")";
    }
#endif

inline void WalkStackInternal(Thread* self)
  REQUIRES_SHARED(Locks::mutator_lock_) {
  int depth = 0;

  DLOG(ERROR) << "|-  " << __func__<<":Inline";
  StackVisitor::WalkStack(
      [&](const StackVisitor* visitor) REQUIRES_SHARED(Locks::mutator_lock_) {

      ArtMethod* m = visitor->GetMethod();
      DLOG(INFO) << "| " << ++depth << ": " <<  m->PrettyMethod();
      if (m->IsRuntimeMethod()) {  // Continue if this is a runtime method.
        return true;
      }
      // method = m;
      // dex_pc = visitor->GetDexPc(abort_on_error);
      return false;
      },
      const_cast<Thread*>(self),
      /* context= */ nullptr,
      StackVisitor::StackWalkKind::kIncludeInlinedFrames,
      true);

  const bool walk_noinline = false;
  if((walk_noinline)) { 
    depth=0;
    DLOG(ERROR) << "|-  " << __func__<< ":NoInline";
    StackVisitor::WalkStack(
        [&](const StackVisitor* visitor)
        REQUIRES_SHARED(Locks::mutator_lock_) {
        ArtMethod* m = visitor->GetMethod();

        DLOG(INFO) << "| " << ++depth << ": " <<  m->PrettyMethod();
        if (m->IsRuntimeMethod()) { // Continue if this is a runtime method.
        return true;
        }
        // method = m;
        // dex_pc = visitor->GetDexPc(abort_on_error);
        return false;
        },
        const_cast<Thread*>(self),
        /* context= */ nullptr,
        StackVisitor::StackWalkKind::kSkipInlinedFrames,
        true);
  }
}

std::string GetManagedStackInfo(
    Thread* self, const ManagedStack* managed_stack, bool from_walk_stack) {
  std::stringstream ss;
  ArtMethod** sp = nullptr;
  ArtMethod* called = nullptr;

  const bool both_frames_set =
    managed_stack->HasTopShadowFrame() && 
    managed_stack->HasTopQuickFrame();
  const bool llvm_frames =  both_frames_set && IN_LLVM();
  const bool bug_both =  both_frames_set && !llvm_frames;

  if(bug_both) {  // Can't be both a shadow and a quick fragment.
    DLOG(ERROR) << "ERROR: has both shadow and quick frames set!";
    ss << "| ";
  }
  
  if(managed_stack->HasTopShadowFrame()) {
    ss << "SF ";
    if(!from_walk_stack) ss << " ";
    ShadowFrame* shadow_frame = managed_stack->GetTopShadowFrame();
    ss << " " << std::hex << shadow_frame;
    Locks::mutator_lock_->SharedLock(self);
    called = shadow_frame->GetMethod();
    Locks::mutator_lock_->SharedUnlock(self);
  }

  if(bug_both || llvm_frames) {
    Locks::mutator_lock_->SharedLock(self);
    ss << ": " << called->PrettyMethod();
    Locks::mutator_lock_->SharedUnlock(self);
    if(from_walk_stack) {
      ss << "\n|  ";
    } else {
      ss << "\n";
    }
  }

  if(managed_stack->HasTopQuickFrame()) {
    if(llvm_frames) {
      ss << "LLVM";
      if(from_walk_stack) ss << " ";
    } else {
      ss << "QF";
    }
    if(managed_stack->GetTopQuickFrameTag()) { // JNI tag
      ss << "j";
      sp = managed_stack->GetTopQuickFrame();
    } else {
      if(!llvm_frames) ss << " ";
      sp = managed_stack->GetTopQuickFrameKnownNotTagged();
    }
    ss << " " << std::hex << sp; 
    called = *sp;
    // uint32_t* sp32 = reinterpret_cast<uint32_t*>(sp);
  }


  if(!managed_stack->HasTopShadowFrame() && 
      !managed_stack->HasTopQuickFrame()) {
    ss << "    <no quick or shadow frame>";
    return ss.str();
  }

  // if quick or shadow
  if(called!=nullptr) {
    if(llvm_frames) {
      // if jumped to RT and switched frames
      // (e.g. called quick, and then quick has called JNI)
      // then the LLVM second frame will become invalid,
      // therefore PrettyMethod will fail
      ss << ": " << "<LLVM method><invalid managed-frame>";
    } else {
      Locks::mutator_lock_->SharedLock(self);
      ss << ": " << called->PrettyMethod();
      Locks::mutator_lock_->SharedUnlock(self);
    }
  } else {
    ss << ": called=null";
  }
  return ss.str();
}

inline void WalkStack(Thread* self, const ManagedStack* managed_stack) {
  DLOG(INFO) << "|--  CustomWalk ---";
  DLOG(INFO) << "|- ManagedStack: top: " << std::hex << managed_stack;
  Locks::mutator_lock_->SharedLock(self);
  self->VerifyStack();
  DLOG(INFO) << "|-- VERIFIED STACK --";
  Locks::mutator_lock_->SharedUnlock(self);
  int depth=0;
  while(managed_stack!= nullptr) {
    std::string info = GetManagedStackInfo(
        self, managed_stack, true);
    // returning empty string when reaching a runtime method
    if(info.size()==0) break;
    DLOG(INFO) << "| " << ++depth << ": " << info;

    // go to parent
    managed_stack=managed_stack->GetLink(); 
  }

  if((false)) { 
    Locks::mutator_lock_->SharedLock(self);
    WalkStackInternal(self);
    Locks::mutator_lock_->SharedUnlock(self);
  }

  DLOG(INFO) << "|------------------";
}

void VerifyStackFrames(Thread* self) {
  const ManagedStack* managed_stack = self->GetManagedStack();
  WalkStack(self, managed_stack);
}

}  // namespace LLVM

bool __IsInLiveLLVM() {
  return IN_LLVM();
}

bool __IsInLiveQuick() {
  return IN_QUICK();
}

bool __IsInLiveAny() {
  return _IN_ICHF();
}

}  // namespace art
