#include "mcr_cc/cc_log.h"
#include "mcr_cc/llvm/llvm_compiler.h"
/**
 * Special logger just for helping to generate a summary of issues once an
 * LLVM compilation is done.
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

namespace art {
namespace mcr {

  CCLogMessage::~CCLogMessage(){
    LogSeverity severity=data_->GetSeverity();

      std::string msg(data_->ToString());

    if(severity == FATAL) {
      printf("%s\n", msg.c_str());
    }

    if(severity == FATAL) {
      LLVM::LlvmCompiler::LogError(msg);
    }
  }

}  // namespace mcr
}  // namespace art
