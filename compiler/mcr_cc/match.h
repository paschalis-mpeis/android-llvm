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
#ifndef ART_COMPILER_MCR_MATCH_H_
#define ART_COMPILER_MCR_MATCH_H_

#include <string>

#define MATCH_CLASS_CTOR "<clinit>"
#define MATCH_CTOR "<init>()"

#define NOT_HOT "__NOT_HOT__"

namespace art {
namespace mcr {

class Match final {
 public:
  static bool IsInDemoApp(std::string pkg);
  static bool equal(std::string s1, std::string s2);
  static bool ShouldAddToHistogram(std::string pretty_method);
  static bool IsMarkedNotHot(std::string pretty_method);
 private:
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_MATCH_H_
