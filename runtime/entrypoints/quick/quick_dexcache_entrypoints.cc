/*
 * Copyright (C) 2021 Paschalis Mpeis
 * Copyright (C) 2012 The Android Open Source Project
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
 */

#include "mcr_rt/mcr_rt.h"

#include "art_method-inl.h"
#include "base/callee_save_type.h"
#include "callee_save_frame.h"
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
#endif

namespace art {

static void StoreObjectInBss(ArtMethod* outer_method,
                             const OatFile* oat_file,
                             size_t bss_offset,
                             ObjPtr<mirror::Object> object,
                             void** llvm_bss_slot = nullptr) REQUIRES_SHARED(Locks::mutator_lock_) {
  // Used for storing Class or String in .bss GC roots.
  static_assert(sizeof(GcRoot<mirror::Class>) == sizeof(GcRoot<mirror::Object>), "Size check.");
  static_assert(sizeof(GcRoot<mirror::String>) == sizeof(GcRoot<mirror::Object>), "Size check.");
  DCHECK_NE(bss_offset, IndexBssMappingLookup::npos);
  DCHECK_ALIGNED(bss_offset, sizeof(GcRoot<mirror::Object>));
  if (UNLIKELY(!oat_file->IsExecutable())) {
    // There are situations where we execute bytecode tied to an oat file opened
    // as non-executable (i.e. the AOT-compiled code cannot be executed) and we
    // can JIT that bytecode and get here without the .bss being mmapped.
    return;
  }

  LOGLLVM4(ERROR) << __func__ << ": from: " << outer_method->PrettyMethod();
  GcRoot<mirror::Object>* slot = reinterpret_cast<GcRoot<mirror::Object>*>(
      const_cast<uint8_t*>(oat_file->BssBegin() + bss_offset));
  DCHECK_GE(slot, oat_file->GetBssGcRoots().data());
  DCHECK_LT(slot, oat_file->GetBssGcRoots().data() + oat_file->GetBssGcRoots().size());
  if (slot->IsNull()) {
    // This may race with another thread trying to store the very same value but that's OK.
    *slot = GcRoot<mirror::Object>(object);
    // We need a write barrier for the class loader that holds the GC roots in the .bss.
    ObjPtr<mirror::ClassLoader> class_loader = outer_method->GetClassLoader();
    Runtime* runtime = Runtime::Current();
    if (kIsDebugBuild) {
      ClassTable* class_table = runtime->GetClassLinker()->ClassTableForClassLoader(class_loader);
      CHECK(class_table != nullptr && !class_table->InsertOatFile(oat_file))
          << "Oat file with .bss GC roots was not registered in class table: "
          << oat_file->GetLocation();
    }
    if (class_loader != nullptr) {
      WriteBarrier::ForEveryFieldWrite(class_loader);
    } else {
      runtime->GetClassLinker()->WriteBarrierForBootOatFileBssRoots(oat_file);
    }
  } else {
    // Each slot serves to store exactly one Class or String.
    DCHECK_EQ(object, slot->Read());
  }

  if(llvm_bss_slot != nullptr) {
    *llvm_bss_slot = reinterpret_cast<void*>(slot);
    LOGLLVM3(WARNING) << __func__ << ": BSS: slot Obj: "
      << std::hex << (void*)(*(void**)(*llvm_bss_slot))
      << " (slot: " << *llvm_bss_slot << ")";
  }
}

static inline void StoreTypeInBss(ArtMethod* outer_method,
                                  dex::TypeIndex type_idx,
                                  ObjPtr<mirror::Class> resolved_type,
                                  void** llvm_bss_slot = nullptr)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  const DexFile* dex_file = outer_method->GetDexFile();
  DCHECK(dex_file != nullptr);
  const OatDexFile* oat_dex_file = dex_file->GetOatDexFile();
  if (oat_dex_file != nullptr) {
    size_t bss_offset = IndexBssMappingLookup::GetBssOffset(oat_dex_file->GetTypeBssMapping(),
                                                            type_idx.index_,
                                                            dex_file->NumTypeIds(),
                                                            sizeof(GcRoot<mirror::Class>));
    if (bss_offset != IndexBssMappingLookup::npos) {
      StoreObjectInBss(outer_method, oat_dex_file->GetOatFile(), bss_offset, resolved_type, llvm_bss_slot);
    }
  }
}

static inline void StoreStringInBss(ArtMethod* outer_method,
                                    dex::StringIndex string_idx,
                                    ObjPtr<mirror::String> resolved_string,
                                    void** llvm_bss_slot = nullptr)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  const DexFile* dex_file = outer_method->GetDexFile();
  DCHECK(dex_file != nullptr);
  const OatDexFile* oat_dex_file = dex_file->GetOatDexFile();
  if (oat_dex_file != nullptr) {
    size_t bss_offset = IndexBssMappingLookup::GetBssOffset(oat_dex_file->GetStringBssMapping(),
                                                            string_idx.index_,
                                                            dex_file->NumStringIds(),
                                                            sizeof(GcRoot<mirror::Class>));
    if (bss_offset != IndexBssMappingLookup::npos) {
      StoreObjectInBss(outer_method, oat_dex_file->GetOatFile(), bss_offset, resolved_string, llvm_bss_slot);
    }
  }
}

static ALWAYS_INLINE bool CanReferenceBss(ArtMethod* outer_method, ArtMethod* caller)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  // .bss references are used only for AOT-compiled code and only when the instruction
  // originates from the outer method's dex file and the type or string index is tied to
  // that dex file. As we do not want to check if the call is coming from AOT-compiled
  // code (that could be expensive), simply check if the caller has the same dex file.
  //
  // If we've accepted running AOT-compiled code despite the runtime class loader
  // resolving the caller to a different dex file, this check shall prevent us from
  // filling the .bss slot and we shall keep going through the slow path. This is slow
  // but correct; we do not really care that much about performance in this odd case.
  //
  // JIT can inline throwing instructions across dex files and this check prevents
  // looking up the index in the wrong dex file in that case. If the caller and outer
  // method have the same dex file, we may or may not find a .bss slot to update;
  // if we do, this can still benefit AOT-compiled code executed later.
  return outer_method->GetDexFile() == caller->GetDexFile();
}

extern "C" mirror::Class* artInitializeStaticStorageFromCode(mirror::Class* klass, Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  LOGLLVM3(INFO) << __func__;
  // Called to ensure static storage base is initialized for direct static field reads and writes.
  // A class may be accessing another class' fields when it doesn't have access, as access has been
  // given by inheritance.
  ScopedQuickEntrypointChecks sqec(self);
  DCHECK(klass != nullptr);
  ClassLinker* class_linker = Runtime::Current()->GetClassLinker();
  StackHandleScope<1> hs(self);
  Handle<mirror::Class> h_klass = hs.NewHandle(klass);
  bool success = class_linker->EnsureInitialized(
      self, h_klass, /* can_init_fields= */ true, /* can_init_parents= */ true);
  if (UNLIKELY(!success)) {
    return nullptr;
  }
  return h_klass.Get();
}

extern "C" mirror::Class* artResolveTypeFromCode(uint32_t type_idx, Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  // Called when the .bss slot was empty or for main-path runtime call.
  ScopedQuickEntrypointChecks sqec(self);
  auto caller_and_outer = GetCalleeSaveMethodCallerAndOuterMethod(
      self, CalleeSaveType::kSaveEverythingForClinit);
  ArtMethod* caller = caller_and_outer.caller;
  ObjPtr<mirror::Class> result = ResolveVerifyAndClinit(dex::TypeIndex(type_idx),
                                                        caller,
                                                        self,
                                                        /* can_run_clinit= */ false,
                                                        /* verify_access= */ false);
  if (LIKELY(result != nullptr) && CanReferenceBss(caller_and_outer.outer_method, caller)) {
    StoreTypeInBss(caller_and_outer.outer_method, dex::TypeIndex(type_idx), result);
  }
  return result.Ptr();
}

extern "C" mirror::Class* artResolveTypeAndVerifyAccessFromCode(uint32_t type_idx, Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  // Called when caller isn't guaranteed to have access to a type.
  ScopedQuickEntrypointChecks sqec(self);
  auto caller_and_outer = GetCalleeSaveMethodCallerAndOuterMethod(self,
                                                                  CalleeSaveType::kSaveEverything);
  ArtMethod* caller = caller_and_outer.caller;
  ObjPtr<mirror::Class> result = ResolveVerifyAndClinit(dex::TypeIndex(type_idx),
                                                        caller,
                                                        self,
                                                        /* can_run_clinit= */ false,
                                                        /* verify_access= */ true);
  // Do not StoreTypeInBss(); access check entrypoint is never used together with .bss.
  return result.Ptr();
}

extern "C" mirror::MethodHandle* artResolveMethodHandleFromCode(uint32_t method_handle_idx,
                                                                Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ScopedQuickEntrypointChecks sqec(self);
  auto caller_and_outer =
      GetCalleeSaveMethodCallerAndOuterMethod(self, CalleeSaveType::kSaveEverything);
  ArtMethod* caller = caller_and_outer.caller;
  ObjPtr<mirror::MethodHandle> result = ResolveMethodHandleFromCode(caller, method_handle_idx);
  return result.Ptr();
}

extern "C" mirror::MethodType* artResolveMethodTypeFromCode(uint32_t proto_idx, Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ScopedQuickEntrypointChecks sqec(self);
  auto caller_and_outer = GetCalleeSaveMethodCallerAndOuterMethod(self,
                                                                  CalleeSaveType::kSaveEverything);
  ArtMethod* caller = caller_and_outer.caller;
  ObjPtr<mirror::MethodType> result = ResolveMethodTypeFromCode(caller, dex::ProtoIndex(proto_idx));
  return result.Ptr();
}

extern "C" mirror::String* artResolveStringFromCode(int32_t string_idx, Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ScopedQuickEntrypointChecks sqec(self);
  auto caller_and_outer = GetCalleeSaveMethodCallerAndOuterMethod(self,
                                                                  CalleeSaveType::kSaveEverything);
  ArtMethod* caller = caller_and_outer.caller;
  ObjPtr<mirror::String> result =
      Runtime::Current()->GetClassLinker()->ResolveString(dex::StringIndex(string_idx), caller);
  if (LIKELY(result != nullptr) && CanReferenceBss(caller_and_outer.outer_method, caller)) {
    StoreStringInBss(caller_and_outer.outer_method, dex::StringIndex(string_idx), result);
  }
  return result.Ptr();
}

#ifdef ART_MCR
extern "C" mirror::Class* artResolveTypeFromLLVM(
    ArtMethod* caller, uint32_t type_idx, void** llvm_bss_slot, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  LOGLLVMDRT(WARNING) << __func__ << ": " << std::to_string(type_idx);
  LLVM_FRAME_FIXUP(self);

  // on nested calls we might have issues otherwise,
  // so use just the caller..
  ArtMethod* outer_method = caller;

  // arguments
  D4LOG(INFO) << "caller: " << std::hex << caller;
  D4LOG(INFO) << "caller: " << caller->PrettyMethod();

  // Called when the .bss slot was empty or for main-path runtime call.
  ScopedQuickEntrypointChecks sqec(self);
    
  ObjPtr<mirror::Class> result =
    ResolveVerifyAndClinit(dex::TypeIndex(type_idx), caller, self,
        /* can_run_clinit= */ false, /* verify_access= */ false);

  if (LIKELY(result != nullptr) && CanReferenceBss(outer_method, caller)) {
    StoreTypeInBss(outer_method, dex::TypeIndex(type_idx), result,
        llvm_bss_slot);
  }
  LOGLLVMDRT(INFO) << __func__ << ": Class: " << result->PrettyClass()
    << ": " << std::hex << result.Ptr();

  return result.Ptr();
}

extern "C" mirror::Class* artResolveTypeAndVerifyAccessFromLLVM(
    ArtMethod* caller, uint32_t type_idx, void** llvm_bss_slot, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  UNUSED(llvm_bss_slot);
  LLVM_FRAME_FIXUP(self);
  D5LOG(ERROR) <<  __func__;

  // Called when caller isn't guaranteed to have access to a type.
  ScopedQuickEntrypointChecks sqec(self);
  ObjPtr<mirror::Class> result = ResolveVerifyAndClinit(
      dex::TypeIndex(type_idx),
      caller,
      self,
      /* can_run_clinit= */ false,
      /* verify_access= */ true);

  // Do not StoreTypeInBss(); access check entrypoint is never used together with .bss.
  D2LOG(INFO) << __func__ << ": class: " << result->PrettyClass()
    << ": " << std::hex << result.Ptr();

  return result.Ptr();
}

extern "C" mirror::Class* artResolveTypeInternalLLVM(
    ArtMethod* caller, uint32_t type_idx, void** llvm_bss_slot, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  UNUSED(llvm_bss_slot);
  D5LOG(ERROR) << "RTIVA: " << __func__;
  LLVM_FRAME_FIXUP(self);
  D5LOG(WARNING) << "RTIVA: "<< __func__ << ": " << std::to_string(type_idx);

  // Called when caller isn't guaranteed to have access to a type.
  ScopedQuickEntrypointChecks sqec(self);
  ObjPtr<mirror::Class> result = ResolveVerifyAndClinit(
      dex::TypeIndex(type_idx),
      caller,
      self,
      /* can_run_clinit= */ false,
      /* verify_access= */ true);

  D2LOG(WARNING) << __func__ << ": ResolveVerifyAndClinit returned";

  // Do not StoreTypeInBss(); access check entrypoint is never used together with .bss.

  D5LOG(INFO) << __func__ << ": class: " << result->PrettyClass()
    << ": " << std::hex << result.Ptr();

  return result.Ptr();
}

extern "C" mirror::String* artResolveStringFromLLVM(
    ArtMethod* caller, int32_t string_idx, void** llvm_bss_slot, Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  LOGLLVM3(INFO) << __func__ << ": Caller: " << caller->PrettyMethod();
  LOGLLVM4(INFO) << __func__ << ": String Idx: " << string_idx;
  LLVM_FRAME_FIXUP(self);

  // if we use the LLVM::ShadowFrame ArtMethod we might have
  // issues with nested calls (it will be the wrong outer)
  ArtMethod* outer_method = caller;

  ScopedQuickEntrypointChecks sqec(self);
  ObjPtr<mirror::String> result =
      Runtime::Current()->GetClassLinker()->ResolveString(dex::StringIndex(string_idx), caller);
  // No point of doing this additional check below:
  // && CanReferenceBss(outer_method, caller)
  // This is because we haven't setup a Quick stack frame (as we come from LLVM), so we
  // don't have an 'outer_method'. I do believe though after some digging, that caller, outer_method
  // have to do with inlining:
  // e.g., A -> B -> C, and C is inlined in B:
  // for C: caller is B, and outer_method is A
  // for B: caller is A, and outer_method is also A
  if (LIKELY(result != nullptr)) {
    // INFO must NOT be stored if it's a relro
    // (relro loads now are completely done in LLVM)
    LOGLLVM4(WARNING) << __func__ << ": Store in BSS";
    StoreStringInBss(outer_method, dex::StringIndex(string_idx), result, llvm_bss_slot);
  }
#ifdef CRDEBUG2
  else {
    LOGLLVM(ERROR) << __func__ << ": nullptr (caller:"
      << caller->PrettyMethod() << ")";
  }
#endif

  LOGLLVM4(ERROR) << __func__ << ": Resolved: '"
    <<  result.Ptr()->ToModifiedUtf8() << "'";

  return result.Ptr();
}
#endif

}  // namespace art
