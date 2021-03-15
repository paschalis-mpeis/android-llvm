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
#include "llvm_info.h"

#include "mcr_rt/mcr_rt.h"
#include "mcr_cc/mcr_cc.h"

namespace art {
namespace LLVM {

LLVMInfo::LLVMInfo() {
  // Create context, module, intrinsic helper & ir builder
  llvm_context_.reset(new LLVMContext());
}

LLVMInfo::~LLVMInfo() {
  D5LOG(INFO) << __func__;
  llvm_context_.release();
}

}  // namespace LLVM
}  // namespace art
