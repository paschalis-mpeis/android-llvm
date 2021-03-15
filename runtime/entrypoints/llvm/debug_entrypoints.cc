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
#include "managed_stack.h"

#ifdef ART_MCR
#include "jvalue-inl.h"
#include "mcr_rt/art_impl-inl.h"

namespace art {

extern "C" void artVerifyArtMethodFromLLVM(
  ArtMethod* method, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  UNUSED(self);

  mcr::LogSeverity lvl = mcr::INFO;
  std::stringstream ss;
  if(method==nullptr) {
    lvl=mcr::WARNING;
    ss << "<null>";
  } else {
    ss << method->GetDexMethodIndex()
      << ":" + method->PrettyMethod()
      << " (" << std::hex << method << "/cls:"
      << method->GetDeclaringClass() << ")";
  }

  DLOG(lvl) <<  "VerifyMethod: " << ss.str();
}

extern "C" void artVerifyArtClassFromLLVM(
  mirror::Class* klass, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  UNUSED(self);
  mcr::LogSeverity lvl=mcr::INFO;
  D5LOG(INFO) << __func__;
  if(klass==nullptr) lvl=mcr::WARNING;
  std::string pretty_class = LLVM::PrettyClass(klass);
  DLOG(lvl) << "VerifyClass: " << pretty_class 
    << " (" << std::hex << klass << ")";
}

extern "C" void artVerifyArtObjectFromLLVM(
  mirror::Object* obj, Thread* self)
REQUIRES_SHARED(Locks::mutator_lock_) {
  UNUSED(self);
  D5LOG(INFO) << __func__;
  mcr::LogSeverity lvl = mcr::INFO;

  std::stringstream ss;
  if(obj==nullptr) {
    lvl=mcr::WARNING;
    ss << "<null object>";
  } else {
    mirror::Class* klass = obj->GetClass();
    std::string pretty_class = LLVM::PrettyClass(klass);

    ss << pretty_class
      << " (cls: " << std::hex << klass << "/ptr: "
      << std::hex << obj << ")";
  }

  DLOG(lvl) << "VerifyObject: " << ss.str();
}

extern "C" void VerifyCurrentStackFrame(Thread* self) {
  D4LOG(INFO) << __func__;
  Locks::mutator_lock_->SharedLock(self);
  Locks::mutator_lock_->SharedUnlock(self);
  const ManagedStack* managed_stack = self->GetManagedStack();

  DLOG(INFO) << "| CURRENT ManagedFrame:\n"
    << LLVM::GetManagedStackInfo(self, managed_stack);
}

#endif

}  // namespace art
