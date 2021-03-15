/**
 * LLVM utilities and pretty printers for LLVM components.
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
#include "llvm_utils.h"

#include <llvm/Support/raw_ostream.h>
#include <sstream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "asm_arm64.h"
#include "mcr_rt/mcr_rt.h"
#include "mirror/array-inl.h"
#include "mirror/object_array-inl.h"
#include "mirror/object_reference.h"
#include "optimizing/nodes.h"

namespace art {

std::ostream& operator<<(std::ostream& os, const QuickEntrypointEnum& kind);

namespace LLVM {

size_t GetCacheOffset(uint32_t index) {
  return mirror::ObjectArray<mirror::Object>::OffsetOfElement(index).SizeValue();
}

std::string Pretty(const HInvoke* h) {
  // if called from HInvoke it will cause an infinite loop?
  D5LOG(INFO) << __func__;
  if(h==nullptr) return "";
  std::stringstream ss;

  if (h->IsInvokeVirtual()) {
    ss << "Virtual";
  } else if (h->IsInvokeStaticOrDirect()) {
    if (h->AsInvokeStaticOrDirect()->IsStatic()) {
      ss << "Static";
    } else {
      ss << "Direct";
    }
  } else if (h->IsInvokeCustom()) {
    ss << "Custom";
  } else if (h->IsInvokeInterface()) {
    ss << "Interface";
  } else if (h->IsInvokeUnresolved()) {
    ss << "Unresolved";
  }

  return ss.str();
}

std::string Pretty(QuickEntrypointEnum e) {
  std::stringstream ss;
  ss << e;

  std::string s = ss.str();
  const std::string prefix = "LLVM";
  if(s.find(prefix) != std::string::npos) {
    size_t pos = s.find(prefix);
    if (pos != std::string::npos) s.erase(pos, prefix.length());
    s="Llvm"+s;
  }

  return "entrypoint" + s;
}

std::string Pretty(Type* type) {
  std::string tmp;
  raw_string_ostream OS(tmp);
  OS << *type;
  return OS.str();
}

std::string Pretty(::llvm::Instruction& i) {
  std::string tmp;
  raw_string_ostream OS(tmp);
  OS << i;
  return OS.str();
}

std::string Pretty(::llvm::Instruction* i) {
  return Pretty(*i);
}

std::string Pretty(Function* f) {
  if(f==nullptr) return "indirect call";
  return f->getName().str();
}

std::string Pretty(BasicBlock& bb) {
  return bb.getName().str();
}

std::string Pretty(BasicBlock* bb) {
  if(bb==nullptr) return "<BB:null>";
  return Pretty(*bb);
}

std::ostream& operator<<(std::ostream& os, art::QuickEntrypointEnum& e) {
  const char* sqpoints[] = { // NOLINT(whitespace/braces)
#define ENTRYPOINT_STR(name, rettype, ...) STRINGIFY(name),
#include "entrypoints/quick/quick_entrypoints_list.h"
                             QUICK_ENTRYPOINT_LIST(ENTRYPOINT_STR)
#undef QUICK_ENTRYPOINT_LIST
#undef ENTRYPOINT_STR
  };

  os << sqpoints[static_cast<int>(e)];
  return os;
}

void FindEntrypoint(uint32_t num) {
  // count the entrypoints
  int num_entrypoints = 0;
#define ENTRYPOINT_CNT(name, rettype, ...) num_entrypoints++;
#include "entrypoints/quick/quick_entrypoints_list.h"
  QUICK_ENTRYPOINT_LIST(ENTRYPOINT_CNT)
#undef QUICK_ENTRYPOINT_LIST
#undef ENTRYPOINT_CNT

  QuickEntrypointEnum one = static_cast<QuickEntrypointEnum>(0);
  QuickEntrypointEnum prelast =
      static_cast<QuickEntrypointEnum>(num_entrypoints - 1);
  D5LOG(INFO) << __func__ << " one: " << one;
  D5LOG(INFO) << __func__ << " prelast: " << prelast;

  for (int i = 0; i < num_entrypoints; i++) {
    QuickEntrypointEnum qpoint = static_cast<QuickEntrypointEnum>(i);
    uint32_t offset = Arm64::offset(qpoint);
    if (offset == num) {
      DLOG(INFO) << __func__ << ": " << std::to_string(num)
                 << ": " << qpoint;
      return;
    }
  }
  DLOG(ERROR) << __func__ << " Failed to find : " << std::to_string(num);
}

}  // namespace LLVM
}  // namespace art
