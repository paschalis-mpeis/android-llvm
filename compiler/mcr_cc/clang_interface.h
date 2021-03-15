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
#ifndef ART_COMPILER_MCR_CLANG_INTERFACE_H_
#define ART_COMPILER_MCR_CLANG_INTERFACE_H_

#include "mcr_cc/compiler_interface.h"

// NOTES on flags:
// - libm is included anyway w/ C++. It is used internally by libstdc++.
// - hf.so must be linked against libart
//   + Android linker differs from Linux (more: https://goo.gl/bNvIDG)

#define OPT_EMIT_LLVM " -emit-llvm "
#define OPT_EMIT_ASM " -S "
#define MCR_FLAGS "-DART_MCR -DART_MCR_TARGET -DICHF"
#define DEBUG_FLAGS "  "                         // -ggdb3
#define WARNING_FLAGS "-Wno-unknown-attributes"  // ignore `alloc_size` warnings
#define CFLAGS " -std=c++11 -march=armv7-a -fPIE " WARNING_FLAGS " " MCR_FLAGS
#define HF_CC_TO_LLVM_BC "hf.cc.bc"

#define LDFLAGS " -llog -lm -lart -fpic -shared "
#ifdef ART_MCR_ANDROID_6
// linker scripts not usedNot used
#define LDFLAGS_EXTRA_SCRIPT " -Wl,-T," DIR_CONFIG "/linker.script "
#elif defined(ART_MCR_ANDROID_10)
#define NDK_SYSROOT "/system/usr/lib"
#define ARCH_FLAGS_ANDROID_10 " -ffixed-x19 -ffixed-x20 "
#define LDFLAGS_ANDROID_10 " --target=aarch64-linux-android -nodefaultlibs " 
#endif

namespace art {
namespace mcr {

class ClangInterface : public CompilerInterface {
 public:
  static bool Compile(bool emit_llvm, bool emit_asm);
  static bool GenerateSharedObject();
 private:
#ifdef ART_MCR_ANDROID_6
  static std::string GetIncludeDirClang();
  static std::string GetDirClang();
  static void setupEnvironment(); 
#endif
  static const std::string CC;
  static const std::string LD;
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_CLANG_INTERFACE_H_
