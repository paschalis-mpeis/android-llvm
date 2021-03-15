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
#ifndef ART_COMPILER_LLVM_UTILS_H_
#define ART_COMPILER_LLVM_UTILS_H_

#include <stddef.h>
#include <cstdint>
#include <string>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "entrypoints/quick/quick_entrypoints_enum.h"

using namespace ::llvm;
namespace art {

class HInvoke;

namespace LLVM {

size_t GetCacheOffset(uint32_t index);

void FindEntrypoint(uint32_t hex_num);

std::string Pretty(QuickEntrypointEnum e);
std::string Pretty(Type* type);
std::string Pretty(::llvm::Instruction& i);
std::string Pretty(::llvm::Instruction* i);
std::string Pretty(Function *f);
std::string Pretty(BasicBlock& bb);
std::string Pretty(BasicBlock *bb);

std::string Pretty(const HInvoke *h);

// Custom operator overloading for this preprocessor-generated enum
// removing const from enum is needed to remove ambiguous overloading
std::ostream& operator<<(std::ostream& os, art::QuickEntrypointEnum& e);

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_UTILS_H_
