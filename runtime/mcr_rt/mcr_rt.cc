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

#include <sys/resource.h>
#include <sys/stat.h>
#include <fstream>

#include "art_method-inl.h"
#include "base/os.h"
#include "class_linker.h"
#include "dex/art_dex_file_loader.h"
#include "dex/dex_instruction.h"
#include "dex/method_reference.h"
#include "gc/heap.h"
#include "mcr_rt/art_impl.h"
#include "mcr_rt/filereader.h"
#include "mcr_rt/invoke_info.h"
#include "mcr_rt/opt_interface.h"
#include "mcr_rt/utils.h"
#include "mirror/class-inl.h"
#include "mirror/class.h"
#include "mirror/iftable.h"
#include "runtime.h"
#include "thread.h"
#include "thread_list.h"

namespace art {

bool dbg_log_ = false;
int dbg_cnt_ = 0;
#ifdef CRDEBUG2
bool dbg_llvm_code_ = false;
#else
const bool dbg_llvm_code_ = false;
#endif

#ifdef ART_MCR_RT
bool __in_llvm = false;
bool __in_quick = false;
bool __llvm_called_quick = false;
#endif

namespace mcr {

std::string McrRT::userid_;
std::string McrRT::pkg_;

std::set<std::string> McrRT::hot_functions_;
bool McrRT::dbg_linker_ = false;
bool McrRT::llvm_enabled_= false;
bool McrRT::dbg_qres_trampoline_ = false;

uint64_t McrRT::timer_start_ = 0;
uint64_t McrRT::timer_end_ = 0;

bool McrRT::IsLlvmTestMethod(ArtMethod* method) {
  if (method == nullptr) {
    return false;
  }
  return IsLlvmTestMethod(method->PrettyMethod());
}

bool McrRT::IsLlvmTestMethod(std::string pretty_method) {
  return pretty_method.find(PKG_LLVM_DEMO) != std::string::npos;
}

bool McrRT::IsFrameworkMethod(ArtMethod* method) {
  return IsFrameworkDexLocation(method->GetDexFile()->GetLocation());
}

bool McrRT::IsFrameworkMethodRef(const MethodReference& method_ref) {
  return IsFrameworkDexLocation(method_ref.dex_file->GetLocation());
}

bool McrRT::IsFrameworkDexLocation(std::string dex_filename) {
  const std::string sys_framework = "/system/framework/";
  const std::string apex = "/apex/";
  if ((dex_filename.compare(0, sys_framework.size(), sys_framework) == 0) || (dex_filename.compare(0, apex.size(), apex) == 0)) {
    return true;
  }
  return false;
}

void McrRT::ReadOptions() {
  ReadLlvmEnabled();
  ReadLOGDBG();
  ReadDBG_QuickResolutionTrampoline();

  if (IsDemoApp()) {
    if(!IsLlvmEnabled()) { DLOG(WARNING) << "DEMO APP: LLVM NOT enabled!"; }
    else { DLOG(INFO) << "DEMO APP: LLVM enabled!"; }
  }
}

bool McrRT::IsDemoApp() {
  return (pkg_.find(PKG_LLVM_DEMO) != std::string::npos);
}

void McrRT::ReadLOGDBG() {
  std::string filename = GetFileApp(F_DBG_LOG);
  ::art::dbg_log_ = OS::FileExists(filename.c_str());

  if (::art::dbg_log_ && McrRT::IsLlvmEnabled()) {
    DLOG(WARNING) << "LOGDBG enabled for  " << McrRT::GetPackage() << ")";
  }
}

void McrRT::ReadLlvmEnabled() {
  std::string filename = GetFileApp(F_LLVM_ENABLED);
  llvm_enabled_ = OS::FileExists(filename.c_str());
  if (llvm_enabled_) {
    DLOG(WARNING) << "LLVM enabled for: "  << McrRT::GetPackage();
  }
}

void McrRT::ReadDBG_QuickResolutionTrampoline() {
  std::string filename = GetFileApp(F_QRES_TRAMPOLINE);
  dbg_qres_trampoline_ = OS::FileExists(filename.c_str());
  if (dbg_qres_trampoline_) {
    DLOG(WARNING) << "DBG: artQuickResolutionTrampoline:";
  }
}

void McrRT::InitApp(std::string userid, std::string pkg) {
  if (!pkg.empty()) {
    userid_= userid;
    pkg_ = pkg;
  }
}

std::string McrRT::GetDirMcrData() {
  CheckInitialized();
  return DIR_MCR "/"+userid_;
}

std::string McrRT::GetDirApp() {
  CheckInitialized();
  return GetDirMcrData() + "/" + pkg_;
}

void McrRT::CheckInitialized() {
  if(pkg_.size() == 0 || userid_.size()==0) {
    DLOG(FATAL) << "MCR: Not initialized: "
      << "pkg: '" << pkg_ << "'"
      << "userid: '" << userid_ << "'";
  }
}

void McrRT::ProcessApp(std::string str) {
  const char* appDir=str.c_str();

  if (appDir != NULL && strlen(appDir)> 0) {
    char uid[10];
    char pkg[100];

#ifdef ART_MCR_ANDROID_10
    char _appDir[1000];
    sprintf(_appDir,  "%s", appDir);
    int idx;

    // cut of primary.prof
    const char* t = strrchr(_appDir, '/');
    idx = (int)(t - _appDir);
    _appDir[idx] =  '\0';

    // package
    t = strrchr(_appDir, '/');
    idx = (int)(t - _appDir);
    sprintf(pkg, "%s", t + 1);
    _appDir[idx] =  '\0';

    // user id
    t = strrchr(_appDir, '/');
    sprintf(uid, "%s", t + 1); 
#elif defined(ART_MCR_ANDROID_6)
    const char* t = strrchr(appDir, '/');
    int appDataDirSize = (t - appDir) / sizeof(char);
    char duPath[100];

    // package
    memcpy(duPath, appDir, appDataDirSize);
    duPath[appDataDirSize] = '\0';
    sprintf(pkg, "%s", t + 1);

    // user id
    t = strrchr(duPath, '/') + 1;
    sprintf(uid, "%s", t); 
#else
#error unimplemented
#endif
    InitApp(uid, pkg);
  }
}

std::string McrRT::GetAppHfDir(std::string pretty_hf, bool create) {
  std::string shf = std::string("hf_") + StripHf(pretty_hf);
  std::string hf_dir = GetFileApp(shf);

  if (create) {
    if (!OS::DirectoryExists(hf_dir.c_str())) {
      umask(0);
      __mkdir(hf_dir.c_str(), 0777);
    }
  }
  return hf_dir;
}

const DexFile* McrRT::_OpenDexFile(std::string dex_filename, std::string dex_location, bool only_os) {
  D3LOG(INFO) << __func__ << ":loc: " << dex_location;

  if (!OS::FileExists(dex_filename.c_str())) {
    DLOG(FATAL) << "_OpenDexFile: Not exists: " << dex_filename;
    UNREACHABLE();
  }

  // it may belong to boot class path: the classes.dex is removed from the
  // jars in /system/framework, and AOT compiled in boot.art/oat image files
  if (McrRT::IsFrameworkDexLocation(dex_filename)) {
    std::vector<const DexFile*> boot_dex_files =
        Runtime::Current()->GetClassLinker()->GetBootClassPath();
    for (const DexFile* dex_file : boot_dex_files) {
      if (dex_file->GetLocation().compare(dex_location) == 0) {
        return dex_file;
      }
    }
    DLOG(FATAL) << "Failed to find framework dex_file: "
                << dex_filename << "\ndex_loc: " << dex_location;
    UNREACHABLE();

  } else {
    if (only_os) {
      DLOG(FATAL) << "_OpenDexFile: does not belong to OS:" << dex_location;
      UNREACHABLE();
    } else {
      // might have issues with this
      DLOG(INFO) << "_OpenDexFile: app dex_file: " << dex_location;
      std::vector<std::unique_ptr<const DexFile>> opened_dex_files;
      std::string error_msg;
      const ArtDexFileLoader dex_file_loader;
      if (!dex_file_loader.Open(dex_filename.c_str(),
                                dex_location.c_str(),
                                false, false,
                                &error_msg,
                                &opened_dex_files)) {
        DLOG(FATAL) << "_OpenDexFile: Failed."
                    << "\ndex_filename: " << dex_filename
                    << "\ndex_location: " << dex_location
                    << "\nError: " << error_msg;
      }
      std::unique_ptr<const DexFile> dex_file_ptr = std::move(opened_dex_files[0]);
      const DexFile* ptr = dex_file_ptr.get();
      return ptr;
    }
  }
}

std::string PrettyReturn(Primitive::Type type, JValue* result)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  std::stringstream ss;
  ss << type << ":";
  switch (type) {
    case Primitive::kPrimBoolean: {
      ss << std::to_string(static_cast<bool>(result->GetZ()));
    } break;
    case Primitive::kPrimByte: {
      ss << std::to_string(static_cast<short>(result->GetB()));
    } break;
    case Primitive::kPrimShort: {
      ss << std::to_string(static_cast<short>(result->GetS()));
    } break;
    case Primitive::kPrimChar: {
      ss << std::to_string(static_cast<char>(result->GetC()));
    } break;
    case Primitive::kPrimInt: {
      ss << std::to_string(result->GetI());
    } break;
    case Primitive::kPrimLong: {
      ss << std::to_string(result->GetJ());
    } break;
    case Primitive::kPrimFloat: {
      ss << std::to_string(result->GetF());
    } break;
    case Primitive::kPrimDouble: {
      ss << std::to_string(result->GetD());
    } break;
    case Primitive::kPrimNot: {
      ss << "OBJ (" << std::hex << result->GetL();
      ss << ")";
    } break;
    case Primitive::kPrimVoid: {
      ss << "void";
    } break;
  }
  return ss.str();
}

int __mkdir(const char* fulldir, mode_t mode) {
  const char* filename = strrchr(fulldir, '/');
  filename++;
  if (strlen(filename) > 255) {
    DLOG(FATAL) << "__mkdir: will fail: " << fulldir << " len: " << strlen(filename);
  }

  return mkdir(fulldir, mode);
}

}  // namespace mcr
}  // namespace art
