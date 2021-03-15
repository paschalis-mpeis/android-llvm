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
#include <dlfcn.h>

#include "mcr_rt/mcr_rt.h"

#include "art_field-inl.h"
#include "art_field.h"
#include "base/os.h"
#include "gc/heap.h"
#include "gc/space/image_space.h"
#include "mcr_rt/art_impl.h"
#include "mcr_rt/art_impl_arch-inl.h"
#include "mcr_rt/mcr_dbg.h"
#include "mcr_rt/opt_interface.h"
#include "mcr_rt/utils.h"
#include "mirror/object-inl.h"
#include "mirror/object.h"
#include "oat_file.h"
#include "read_barrier.h"

#include "entrypoints/quick/quick_default_externs.h"

#include "entrypoints/entrypoint_utils.h"

namespace art {
namespace mcr {


// QuickEntryPoints* OptimizingInterface::qpoints_ = nullptr;

std::map<uint32_t, void*> OptimizingInterface::dl_pointers_;
std::map<uint32_t, void*> OptimizingInterface::dl_handlers_;

inline void* dl_sym(void* handle, const char* symbol) {
  D3LOG(INFO) << __func__ << ": symbol: " << symbol << ": handle: " << handle;
  const char* err;
  void* s = dlsym(handle, symbol);
  // dlsym might return null and that could be ok (see manpage)
  err = dlerror();
  if (err != NULL) {
    DLOG(FATAL) << __func__ << ": FAILED: " << symbol << ": " << err;
  }
  return s;
}

inline  void* dl_open_llvm(const char* file_so) {
  const char* err;
  if (!OS::FileExists(file_so)) {
    DLOG(ERROR) << "LLVM code not found:  " << file_so;
    return nullptr;
  }

  void* h = dlopen(file_so, RTLD_NOW);

  // called to be cleared later on for dlsym (see manpage)
  err = dlerror();
  if (h == nullptr) {
    DLOG(FATAL) << __func__ << ": " << err << "\nFile: " << file_so;
  }
  return h;
}

void OptimizingInterface::LoadCodeForMethod(
    std::string stripped_hf, uint32_t midx) {
  std::string file_so = std::string(
      GetDirAppHfSrc(stripped_hf)) + "/hf.so";
  LoadCode(file_so, midx);
}

void OptimizingInterface::LoadCode(std::string file_so, uint32_t midx) {
  D5LOG(INFO) << __func__ << ": " << file_so;

  void* handle = dl_open_llvm(file_so.c_str());
  std::pair<uint32_t, void*> ph(midx, handle);
  dl_handlers_.insert(ph);

  // only live_llvm_ symbol is used in this version
  std::string symbol = SYMBOL_LLVM_LIVE;

  void* s = dl_sym(handle, symbol.c_str());
  std::pair<uint32_t, void*> ps(midx, s);
  dl_pointers_.insert(ps);
}

/**
 * @brief It executes the preloaded llvm code.
 *        which will either be the llvm_ entrypoint,
 *        or the llvm_live_ entrypoint!
 */

bool OptimizingInterface::ExecuteLLVM(Thread* self, ArtMethod* method,
    uint32_t* args, JValue* result, const char* shorty) {
  D2LOG(WARNING) << __func__;
  UNUSED(shorty);

  const uint32_t boot_image_begin = dchecked_integral_cast<uint32_t>(reinterpret_cast<uintptr_t>(
        Runtime::Current()->GetHeap()->GetBootImageSpaces().front()->Begin()));

  void (*entrypoint)(void*, uint32_t*, JValue*, void*, uint32_t);
  *(void**)(&entrypoint) = GetDlPointer(method->GetDexMethodIndex());

  /* EXECUTE LLVM CODE.
   *  Arguments:
   *    x0  - method
   *    x1  - args
   *    x2  - jvalue result
   *    x3  - self
   *    x4  - boot image begin
   */
  LLVM_ENTERED();
  (*entrypoint)(method, args, result, self, boot_image_begin);
  LLVM_EXITED();

  // INFO when we jump LLVM, for some reason the TopManagedStack
  // (Quick Stack Frame) is set. Since we are creating a ShadowFrame
  // for LLVM call, while in LLVM there are both TopQuickFrame and
  // TopShadowFrame set for the top MangedStack.
  // But once we return from the llvm entrypoint (here), and until
  // the latest ShadowFrame and QuickFrame are free'ed (they were
  // created before entering LLVM), the TopQuickFrame is invalid.
  // So we manually set it to null, just in case someone walks the
  // stack and tries to access ArtMethod.
  // TODO: align native LLVM's frame to match Quick's
  LLVM_FRAME_FIXUP(self);

  // INFO there are issues when calling and returning from LLVM
  // Some registers are not properly reserved.
  // As a workaround do NOT use the rest of this method
  // e.g. the register holding shorty is not restored..
  
  return true;
}

void OptimizingInterface::UnloadCode(uint32_t midx) {
  D2LOG(INFO) << __func__;
  dlclose(GetDlHandler(midx));
  dl_handlers_.erase(midx);
  dl_pointers_.erase(midx);
}

void* OptimizingInterface::GetDlHandler(uint32_t midx) {
  std::map<uint32_t, void*>::iterator f = dl_handlers_.find(midx);
  if (f == dl_handlers_.end()) {
    DLOG(FATAL) << __func__ << ": " << midx;
    return nullptr;
  }
  return f->second;
}

void* OptimizingInterface::GetDlPointer(uint32_t midx) {
  std::map<uint32_t, void*>::iterator f = dl_pointers_.find(midx);
  if (f == dl_pointers_.end()) {
    DLOG(FATAL) << __func__ << ": " << midx;
    return nullptr;
  }
  return f->second;
}

}  // namespace mcr
}  // namespace art
