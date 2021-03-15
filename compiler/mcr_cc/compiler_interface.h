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
#ifndef ART_COMPILER_MCR_COMPILER_INTERFACE_H_
#define ART_COMPILER_MCR_COMPILER_INTERFACE_H_

#include <string>
#include "base/macros.h"

#define AS "as "

namespace art {
namespace mcr {

class CompilerInterface {
 protected:
  static void cdSrcDir();
  static bool CHMOD(std::string file, std::string perms);
  static bool EXE(std::string cmd, bool print_output = false,
      bool increased_timeout = false);

  static inline std::string spaced(std::string tok) {
    return " " + tok + " ";
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CompilerInterface);
};

}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_COMPILER_INTERFACE_H_
