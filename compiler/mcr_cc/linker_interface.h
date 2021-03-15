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
#ifndef ART_COMPILER_MCR_LINKER_INTERFACE_H_
#define ART_COMPILER_MCR_LINKER_INTERFACE_H_

#include <set>
#include "mcr_cc/compiler_interface.h"

namespace art {
namespace mcr {

class LinkerInterface : public CompilerInterface {
 public:
  static void AddDependency(std::string caller, std::string callee);
  static void StoreDependencies(std::string caller);
  static bool HasDependencies(std::string hf);

  static int LinkMethods(std::string start_method);
  static bool Link2Methods(std::string method1, std::string method2,
                           std::string result);
  static std::set<std::string> GetLinkMethods(std::string start_method);

 private:
  static std::set<std::string> dependencies_;
  static const std::string LINK;
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_LINKER_INTERFACE_H_
