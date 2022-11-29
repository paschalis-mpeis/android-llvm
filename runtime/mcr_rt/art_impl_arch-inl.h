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
#ifndef ART_RUNTIME_LLVM_IMPL_ARCH_INL_H_
#define ART_RUNTIME_LLVM_IMPL_ARCH_INL_H_

#include "mcr_rt/art_impl_arch_arm-inl.h"
#include "mcr_rt/mcr_rt.h"

namespace art {
namespace LLVM {

static inline Thread* GetLLVMThreadPointer() {
  return reinterpret_cast<Thread*>(Arm::GetLLVMThreadPointer());
}

}  // namespace LLVM
}  // namespace art

#endif  // ART_RUNTIME_LLVM_IMPL_ARCH_INL_H_