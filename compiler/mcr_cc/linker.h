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
#ifndef ART_COMPILER_MCR_LINKER_H_
#define ART_COMPILER_MCR_LINKER_H_

#include "dex/dex_file.h"
#include "mirror/class.h"

namespace art {
namespace mcr {

class Linker {
 public:
  static mirror::Class* ResolveClass(jobject jclass_loader, const DexFile* dex_file,
                                     const dex::ClassDef& class_def);
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_LINKER_H_
