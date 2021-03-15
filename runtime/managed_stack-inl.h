/*
 * Copyright (C) 2021 Paschalis Mpeis
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_RUNTIME_MANAGED_STACK_INL_H_
#define ART_RUNTIME_MANAGED_STACK_INL_H_

#include "managed_stack.h"

#if defined(ART_MCR_TARGET) && defined(CRDEBUG4)
#include "mcr_rt/mcr_dbg.h"
#endif

#include "interpreter/shadow_frame.h"

namespace art {

inline ShadowFrame* ManagedStack::PushShadowFrame(ShadowFrame* new_top_frame) {
  DCHECK(!HasTopQuickFrame());
#if defined(ART_MCR_TARGET) && defined(CRDEBUG4)
  if(HasTopQuickFrame()) {
    if(__IsInLiveLLVM()) LOG(ERROR) << __func__ << ": Had quick frame ERROR";
  }
#endif
  ShadowFrame* old_frame = top_shadow_frame_;
  top_shadow_frame_ = new_top_frame;
  new_top_frame->SetLink(old_frame);
  return old_frame;
}

inline ShadowFrame* ManagedStack::PopShadowFrame() {
  DCHECK(!HasTopQuickFrame());
#if defined(ART_MCR_TARGET) && defined(CRDEBUG4)
  if(HasTopQuickFrame()) {
    if(__IsInLiveLLVM()) LOG(ERROR) << __func__ << ": Had quick frame ERROR";
  }
#endif
  CHECK(top_shadow_frame_ != nullptr);
  ShadowFrame* frame = top_shadow_frame_;
  top_shadow_frame_ = frame->GetLink();
  return frame;
}

}  // namespace art

#endif  // ART_RUNTIME_MANAGED_STACK_INL_H_
