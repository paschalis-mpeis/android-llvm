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

#include "callee_save_frame.h"
#include "thread-inl.h"

namespace art {

extern "C" void artTestSuspendFromCode(Thread* self) REQUIRES_SHARED(Locks::mutator_lock_) {
  LLVM_FRAME_FIXUP(self);
  // Called when suspend count check value is 0 and thread->suspend_count_ != 0
  ScopedQuickEntrypointChecks sqec(self);
  self->CheckSuspend();
}

}  // namespace art
