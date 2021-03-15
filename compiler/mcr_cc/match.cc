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
#include "mcr_cc/match.h"

#include "mcr_rt/mcr_rt.h"
#include "mcr_cc/os_comp.h"

namespace art {
namespace mcr {

bool Match::IsInDemoApp(std::string pkg) {
  return (pkg.find(PKG_LLVM_DEMO) != std::string::npos);
}

bool Match::equal(std::string s1, std::string s2) {
  return s1.compare(s2) == 0;
}

bool Match::IsMarkedNotHot(std::string pretty_method) {
  return pretty_method.find(NOT_HOT) != std::string::npos;
}

bool Match::ShouldAddToHistogram(std::string pretty_method) {
  // not an exception, and not blocklisted
  return pretty_method.find("Exception") == std::string::npos &&
    !OsCompilation::IsOsMethodsBlocklisted(pretty_method);
}

}  // namespace mcr
}  // namespace art
