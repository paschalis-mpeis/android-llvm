/**
 * This can be used to implement logic for supplying flags in an
 * iterative-compilation scenario.
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
#include "mcr_cc/pass_manager.h"

#include <stdio.h>
#include <sys/stat.h>
#include <fstream>
#include "mcr_rt/mcr_rt.h"
#include "mcr_rt/utils.h"
#include "mcr_cc/mcr_cc.h"

namespace art {
namespace mcr {

std::string PassManager::ic_flags_opt_ = "";
std::string PassManager::ic_flags_llc_ = "";

std::string PassManager::backend_ = "";
std::string PassManager::baseline_ = "";
std::string PassManager::dbg_flags_ = "";

void PassManager::SetBaseline(std::string baseline) {
  baseline_ = baseline;
  if (baseline.compare("-O0") == 0) {
    dbg_flags_="-g3";
  }
}

void PassManager::LoadLlvmCompilationFlags() {
  // IR-to-IR translation
  mcr::PassManager::SetBackend("hgraph-llvm");

  // Here one could load additional flags (i.e., for iterative compilation)
}

}  // namespace mcr
}  // namespace art
