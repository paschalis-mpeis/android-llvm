/**
 * There used to be an Hraph to C pretty printer, that we made obsolete
 * when code became bigger and more complex operations (like HPhis) were
 * needed.
 * This class was an interface to the clang tool for compiling.
 * Now it is only used as a driver to the lld linker.
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
#include "mcr_cc/clang_interface.h"
#include "mcr_rt/mcr_rt.h"

#include "base/os.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_cc/pass_manager.h"
#include "mcr_rt/utils.h"

namespace art {
namespace mcr {

#ifdef ART_MCR_ANDROID_6
const std::string ClangInterface::CC = "clang++ ";
#elif defined(ART_MCR_ANDROID_10)
const std::string ClangInterface::CC = "clang ";
const std::string ClangInterface::LD = "ld ";
#endif

bool ClangInterface::GenerateSharedObject() {
  cdSrcDir();
  std::string ldflags=LDFLAGS;
  std::string cflags;
  const std::string debug_flags = mcr::PassManager::GetDebugFlags();
  cflags+=spaced(debug_flags);
#ifdef ART_MCR_ANDROID_6
  setupEnvironment();
#elif defined(ART_MCR_ANDROID_10)
  ldflags+=LDFLAGS_ANDROID_10;
  cflags+=ARCH_FLAGS_ANDROID_10;
#endif
  
  std::string cmd = CC + cflags + ldflags +
          " -Wl,-soname," HFso " -o " HFso " " HFo " ";
  cmd = McrCC::remove_extra_whitespaces(cmd);
  D2LOG(INFO) << "LD: " << cmd;
  bool r = EXE(cmd);
  return r;
}

#ifdef ART_MCR_ANDROID_6
/**
 * @brief For sysroot. on android 10 this is not required
 */
void ClangInterface::setupEnvironment() {
  D3LOG(INFO) << "setupEnvironment()";
  AppendEnvVar(PATH, GetDirClang() + "/usr/bin:" + GetDirClang() + "/usr/bin/applets");
  AppendEnvVar(PATH_LIB, GetDirClang() + "/usr/lib");

  D4LOG(INFO) << "export " PATH "=" << GetEnvVar(PATH) << "\n"
              << "export " PATH_LIB "=" << GetEnvVar(PATH_LIB);
}

std::string ClangInterface::GetIncludeDirClang() {
  std::string dir_clang = GetDirClang() + "/usr/include/";
  checkFolderExists(dir_clang);
  return dir_clang;
}

std::string ClangInterface::GetDirClang() {
  std::string dir_clang = McrRT::GetDirMcr() + std::string("/clang");
  checkFolderExists(dir_clang);
  return dir_clang;
}
#endif

}  // namespace mcr
}  // namespace art
