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
#include "hgraph_to_llvm.h"
#include "hgraph_to_llvm-inl.h"

#include "llvm_macros_irb_.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

Value* HGraphToLLVM::LoadGlobalBssSlot(std::string name) {
    Value* v = irb_->CreateLoad(bss_cache_[name]);
    v->setName("loadedBSS" + name);
    return v;
}

GlobalVariable* HGraphToLLVM::GetGlobalBssSlot(std::string name) {
  if(bss_cache_.find(name) != bss_cache_.end()) {
    D2LOG(INFO) << __func___ << ": found: " << name;
    return bss_cache_[name];
  } else {
    D2LOG(INFO) << __func___ << ": creating: " << name;
    GlobalVariable* newgbl = 
      GetGlobalVariable("gblBssReference"+name,
          DataType::Type::kReference, true);
    bss_cache_.insert(
        std::pair<std::string, GlobalVariable*>(name, newgbl));
    newgbl->setInitializer(irb_->getJZero(DataType::Type::kReference));

    return newgbl;
  }
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
