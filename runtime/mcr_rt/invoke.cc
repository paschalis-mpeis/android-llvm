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
#include "mcr_rt/invoke.h"
#include "mcr_rt/mcr_rt.h"

#include "art_method.h"
#include "mcr_rt/art_impl.h"
#include "stack.h"

namespace art {
namespace mcr {

void ArgArray::DebugInvoke(const ScopedObjectAccessAlreadyRunnable& soa,
                           ArtMethod* method, JValue* result,
                           const char* shorty)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  LLVM::InvokeWithInfo(method, soa.Self(), this->GetArgs(),
                       this->GetNumBytes(), result, shorty, true);
}

void ArgArray::Invoke(const ScopedObjectAccessAlreadyRunnable& soa,
                      ArtMethod* method, JValue* result,
                      const char* shorty)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  // we are sending false because if method if hot shouldn't
  // have left the LLVM code in the first place
  method->Invoke(soa.Self(), this->GetArgs(),
                 this->GetNumBytes(), result, shorty);
}

ArgArray::ArgArray(const char* shorty, uint32_t shorty_len)
    : shorty_(shorty), shorty_len_(shorty_len), num_bytes_(0) {
  size_t num_slots = shorty_len + 1;  // +1 in case of receiver.
  if (LIKELY((num_slots * 2) < kSmallArgArraySize)) {
    // We can trivially use the small arg array.
    arg_array_ = small_arg_array_;
  } else {
    // Analyze shorty to see if we need the large arg array.
    for (size_t i = 1; i < shorty_len; ++i) {
      char c = shorty[i];
      if (c == 'J' || c == 'D') {
        num_slots++;
      }
    }
    if (num_slots <= kSmallArgArraySize) {
      arg_array_ = small_arg_array_;
    } else {
      large_arg_array_.reset(new uint32_t[num_slots]);
      arg_array_ = large_arg_array_.get();
    }
  }
}

}  // namespace mcr
}  // namespace art
