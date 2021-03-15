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
#ifndef ART_COMPILER_MCR_LLC_INTERFACE_H_
#define ART_COMPILER_MCR_LLC_INTERFACE_H_

#include <string>
#include "mcr_cc/compiler_interface.h"

#ifndef TARGET_DEVICE
#error TARGET_DEVICE is not set
#elif TARGET_DEVICE == sailfish
#define DEVICE_FLAGS "-mcpu=kryo"
#elif TARGET_DEVICE == flame
#define DEVICE_FLAGS "-mcpu=kryo"
// #define DEVICE_FLAGS "-mcpu=kryo" TARGET_CPU_VARIANT
#elif TARGET_DEVICE == aosp_shamu
#endif

// INFO thumb arch is buggy!
#ifdef ART_MCR_ANDROID_10
// Thread register: @Android10: x19 (for interpreter it's x22)
// Prior to Android10: x18 was used for xSELF.
// x18 in android10 is referred to as the 'platform register' in:
// quick_entrypoints_arm64.S
//
// x19 should be recerved anyway by this:
// https://github.com/llvm/llvm-project/blob/master/clang/lib/Driver/ToolChains/Arch/AArch64.cpp#L382-L386
// +reserve-x19,
#define ARCH_FLAGS "-march=arm64 -enable-implicit-null-checks -mattr=+reserve-x20 " DEVICE_FLAGS

#else
#define ARCH_FLAGS "-march=arm -arm-reserve-r9 " DEVICE_FLAGS
#endif

#define RELOCATION_FLAGS "-relocation-model=pic"
#define LLC_FLAGS "-filetype=obj " RELOCATION_FLAGS
#define OPT_FLAGS "-disable-debug-info-print"

namespace art {
namespace mcr {

class LlcInterface : public CompilerInterface {
 public:
  static bool CleanupBeforeCompilation();
  static bool CleanupAfterCompilation();
  static bool Compile(bool emit_llvm, bool emit_asm, std::string extraFlags="");
  static bool GenerateSharedObject(std::string hf);

 private:
  static const std::string LLC;
  static const std::string OPT;
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_LLC_INTERFACE_H_
