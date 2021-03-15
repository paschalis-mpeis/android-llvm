/**
 * Supporting ARM64/ARM architecture.
 * Please note that while ARM worked on Android6, it wont work now
 * as the runtime and the compiler have changed heavily.
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
#ifndef ART_RUNTIME_LLVM_IMPL_ARCH_ARM_THUMB_INL_H_
#define ART_RUNTIME_LLVM_IMPL_ARCH_ARM_THUMB_INL_H_

#include <stdint.h>
#include <string>

namespace art {
namespace LLVM {
namespace Arm {

#define LLVM_THREAD_REG_ARM "r9"
#define LLVM_MR_REG_ARM "r8"
// INFO in art it is x19
#define LLVM_THREAD_REG_ARM64 "x19"
#define LLVM_MR_REG_ARM64 "x20"

#if defined(__arm__)

#define REG_SET(cmd, var, reg)            \
  __asm__ __volatile__(cmd " " reg ", %0" \
                       : /*output*/       \
                       : /*input*/        \
                       "r"(var));

#define REG_GET(cmd, reg, var)         \
  __asm__ __volatile__(cmd " %0, " reg \
                       : /*output*/    \
                       "=r"(var)       \
                       : /*input*/     \
                       );

#define __LLVM_THREAD_REG LLVM_THREAD_REG_ARM
#define __LLVM_MR_REG LLVM_MR_REG_ARM
#elif defined(__aarch64__)

#define REG_SET(cmd, var, reg)            \
  __asm__ __volatile__(cmd " " reg ", %0" \
                       : /*output*/       \
                       : /*input*/        \
                       "r"(var));

#define REG_GET(cmd, reg, var)         \
  __asm__ __volatile__(cmd " %0, " reg \
                       : /*output*/    \
                       "=r"(var)       \
                       : /*input*/     \
                       );

#define __LLVM_THREAD_REG LLVM_THREAD_REG_ARM64
#define __LLVM_MR_REG LLVM_MR_REG_ARM64
#endif

#ifdef ART_MCR_RT
static inline void* GetLLVMThreadPointer() {
  uintptr_t thread_pointer;
  REG_GET("mov", __LLVM_THREAD_REG, thread_pointer);
  return reinterpret_cast<void*>(thread_pointer);
}

static inline void* GetX20() {
  uintptr_t thread_pointer;
  REG_GET("mov", "x20", thread_pointer);
  return reinterpret_cast<void*>(thread_pointer);
}
#endif

#undef __LLVM_THREAD_REG

}  // namespace Arm
}  // namespace LLVM
}  // namespace art

#endif  // ART_RUNTIME_LLVM_IMPL_ARCH_ARM_THUMB_INL_H_
