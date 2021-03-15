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
#ifndef ART_RUNTIME_MCR_RT_H_
#define ART_RUNTIME_MCR_RT_H_

#include <android/log.h>
#include <set>

#include "dex/primitive.h"
#include "jvalue.h"

#include "base/locks.h"
#include "dex/dex_file.h"
#include "dex/method_reference.h"
#include "mcr_rt/macros.h"
#include "mcr_rt/mcr_log.h"
#include "mcr_rt/utils.h"

#if defined(ART_MCR_D2O) || defined(ART_MCR_CC)
#include "mcr_cc/cc_log.h"
#endif

#define __resolve_mode ClassLinker::ResolveMode::kNoChecks

#define F_LLVM_ENABLED "/llvm.enabled"
#define F_DBG_LOG "/log.dbg"
#define F_QRES_TRAMPOLINE  "/dbg.qres_trampoline"

#if defined(ART_MCR_ANDROID_10)
#define DIR_MCR "/data/misc/profiles/llvm"
#elif defined(ART_MCR_ANDROID_6)
#define DIR_MCR "/data/misc/llvm"
#else  // host (we don't really care..)
#define DIR_MCR "/tmp"

#endif

#define FOLDER_SRC "src"
#define PKG_LLVM_DEMO "mp.paschalis.llvm.demo"
#define PKG_GOOGLE_CAM "com.google.android.GoogleCamera"

namespace art {

#ifdef ART_MCR_RT
extern bool __in_llvm;
extern bool __in_quick;
extern bool __llvm_called_quick;
#endif

#ifdef CRDEBUG2
extern bool dbg_llvm_code_;
#else
extern const bool dbg_llvm_code_;
#endif
extern bool dbg_log_;
extern int dbg_cnt_;

class ArtMethod;
class Thread;

namespace gc {
class Heap;

namespace collector {
class GarbageCollector;
}  // namespace collector
}  // namespace gc

namespace mirror {
class Class;
}

namespace mcr {

std::string PrettyReturn(Primitive::Type type, JValue* result);

class McrRT final {
 public:
  static std::set<std::string> hot_functions_;

  static bool IsLlvmTestMethod(ArtMethod* method)
    REQUIRES_SHARED(Locks::mutator_lock_);
  static bool IsLlvmTestMethod(std::string pretty_method);
  static bool IsDemoApp();

  static bool IsFrameworkMethod(ArtMethod* method)
    REQUIRES_SHARED(Locks::mutator_lock_);
  static bool IsFrameworkMethodRef(const MethodReference& method_ref);
  static bool IsFrameworkDexLocation(std::string dex_filename);
  static bool DBG_QResTrampoline();

  static std::string GetPackage() { return pkg_; }
  static std::string GetUserId() { return userid_; }
  static std::string GetDirMcr() { return DIR_MCR; }
  static std::string GetDirMcrData();
  static std::string GetDirApp();
  static void CheckInitialized();
  
  static bool IsLlvmEnabled() { return llvm_enabled_; }
  static void DisableLlvm() { llvm_enabled_ = false; }

  static void ProcessApp(std::string str);
  static void InitApp(std::string userid, std::string pkg);

  static std::string GetAppHfDir(std::string hf, bool create = false);

  static const DexFile* _OpenDexFile(std::string dex_filename, std::string dex_location, bool os_only);
  static const DexFile* OpenDexFileANY(std::string dex_filename, std::string dex_location) {
    return _OpenDexFile(dex_filename, dex_location, false);
  }
  static const DexFile* OpenDexFileOS(std::string dex_filename, std::string dex_location) {
    return _OpenDexFile(dex_filename, dex_location, true);
  }
  static void ReadOptions();
  static void ReadLlvmEnabled();
  static void ReadLOGDBG();

  static void ReadDBG_QuickResolutionTrampoline();

  static bool dbg_qres_trampoline_;
  static bool dbg_linker_;
  static bool llvm_enabled_;

  static uint64_t timer_start_;
  static uint64_t timer_end_;

  static std::string pkg_;
  static std::string userid_;
};

int __mkdir(const char* dir, mode_t mode);

}  // namespace mcr
}  // namespace art

#endif  // ART_RUNTIME_MCR_RT_H_
