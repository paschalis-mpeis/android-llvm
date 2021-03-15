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
#ifndef ART_COMPILER_MCR_PASS_MANAGER_H_
#define ART_COMPILER_MCR_PASS_MANAGER_H_

#include <string>
#include <vector>

// This could be used in an iterative-compilation scenario to set the ic_flags
#define FILE_COMPILE_PLAN "plan.comp"

namespace art {
namespace mcr {

class PassManager {
 public:
  static void LoadLlvmCompilationFlags();

  static std::string GetCompilationFlagsOPT() {
    return ic_flags_opt_;
  }

  static std::string GetCompilationFlagsLLC() {
    return ic_flags_llc_;
  }

  static void SetBaseline(std::string baseline);
  
  static void SetBackend(std::string backend) {
    backend_ = backend;
  }

  static std::string GetBaseline()  {
    return baseline_;
  }

  static std::string GetDebugFlags()  {
    return dbg_flags_;
  }

  static std::string GetFlagsInfoOPT() {
    return backend_ + "|" + baseline_ + "|" + ic_flags_opt_;
  }

  static std::string GetFlagsInfoLLC() {
    return backend_ + "|" + baseline_ + "|" + ic_flags_llc_;
  }

 private:
  // On clang it is -O1, -O2, etc, and on ART's it is the
  // android version. e.g. m for Marshmallow
  static std::string baseline_;
  static std::string dbg_flags_;
  static std::string backend_;
  static std::string ic_flags_opt_;
  static std::string ic_flags_llc_;
};
}  // namespace mcr
}  // namespace art

#endif  // ART_COMPILER_MCR_PASS_MANAGER_H_
