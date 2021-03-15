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

#ifndef ART_COMPILER_MCR_CC_LOG_H
#define ART_COMPILER_MCR_CC_LOG_H

#include <android-base/logging.h>
#include "mcr_rt/mcr_log.h"
#include "mcr_rt/macros.h"

#undef LOGLLVM
#undef LOGLLVM2
#undef LOGLLVM3
#undef LOGLLVM4
#undef LOGLLVM5

using namespace android::base;

namespace art {
namespace mcr {

class McrLogMessage;
class CCLogMessage : public McrLogMessage {
  public:
CCLogMessage(const char* file, unsigned int line, LogId id,
        LogSeverity severity, const char* tag, int error, McrLog mcr_log):
  McrLogMessage(file, line, id, severity, tag, error, mcr_log) { }
CCLogMessage(const char* file, unsigned int line, LogId id,
        LogSeverity severity, const char* tag, int error, McrLog mcr_log,
        bool pipe_stdout):
  McrLogMessage(file, line, id, severity, tag, error, mcr_log, pipe_stdout) { }

  virtual ~CCLogMessage();
};

}  // namespace mcr
}  // namespace art
#endif  // ifndef ART_COMPILER_MCR_CC_LOG_H
