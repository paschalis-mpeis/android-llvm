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
#ifndef ART_MCR_OPT_INTERFACE_H_
#define ART_MCR_OPT_INTERFACE_H_

#include <map>
#include "art_method.h"
#include "base/mutex.h"
#include "entrypoints/quick/quick_entrypoints.h"
#include "jvalue.h"

#define SYMBOL_LLVM_LIVE "llvm_live_"
#define SYMBOL_INIT "init_"

namespace art {
namespace mcr {

class OptimizingInterface {
 public:
  static void LoadCodeForMethod(std::string stripped_hf, uint32_t midx);
  static void LoadCode(std::string file_so, uint32_t midx);
  static void UnloadCode(uint32_t midx);
  static bool ExecuteLLVM(Thread* self, ArtMethod* method,
      uint32_t* args, JValue* result, const char* shorty)
      REQUIRES_SHARED(Locks::mutator_lock_);
  static void* GetDlPointer(uint32_t midx);
  static void* GetDlHandler(uint32_t midx);
  static QuickEntryPoints* qpoints_;
  
 private:
  static std::map<uint32_t, void*> dl_pointers_;
  static std::map<uint32_t, void*> dl_handlers_;

};

}  // namespace mcr
}  // namespace art

#endif  // ART_MCR_OPT_INTEFACE_H_
