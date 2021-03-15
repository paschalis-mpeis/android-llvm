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
#include "mcr_rt/mcr_log.h"
#include "mcr_rt/mcr_rt.h"

#include <android-base/logging.h>


namespace art {
namespace mcr {

const char* GetFileBasename(const char* file) {
  // We can't use basename(3) even on Unix because the Mac doesn't
  // have a non-modifying basename.
  const char* last_slash = strrchr(file, '/');
  if (last_slash != nullptr) {
    return last_slash + 1;
  }
#if defined(_WIN32)
  const char* last_backslash = strrchr(file, '\\');
  if (last_backslash != nullptr) {
    return last_backslash + 1;
  }
#endif
  return file;
}

std::ostream& McrLogMessage::stream() {
  return data_->GetBuffer();
}

McrLogMessage::McrLogMessage(const char* file, unsigned int line, LogId id,
    android::base::LogSeverity severity,
                       const char* tag, int error, McrLog mcr_log)
    : data_(new McrLogMessageData(
          file, line, id, severity, tag, error, mcr_log)) {
    }


inline const char* LogSeverityToString(android::base::LogSeverity severity) {
  switch(severity) {
    case android::base::LogSeverity::ERROR:
      return "ERROR";
    case android::base::LogSeverity::FATAL:
      return "FATAL";
    case android::base::LogSeverity::WARNING:
      return "WARNING";
    case android::base::LogSeverity::INFO:
      return "INFO";
    case android::base::LogSeverity::DEBUG:
      return "DEBUG";
    default:
      return "UKNOWN";
  }
}

McrLogMessage::~McrLogMessage() {
  // Check severity again. This is duplicate work wrt/ LOG macros, but not LOG_STREAM.
  if (!WOULD_LOG(data_->GetSeverity())) {
    return;
  }

  // Finish constructing the message.
  if (data_->GetError() != -1) {
    data_->GetBuffer() << ": " << strerror(data_->GetError());
  }
  std::string msg(data_->ToString());

  static constexpr android_LogPriority kLogSeverityToAndroidLogPriority[] = {
      ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG, ANDROID_LOG_INFO,
      ANDROID_LOG_WARN,    ANDROID_LOG_ERROR, ANDROID_LOG_FATAL,
      ANDROID_LOG_FATAL,
    };
    static_assert(arraysize(kLogSeverityToAndroidLogPriority) == FATAL + 1,
        "Mismatch in size of kLogSeverityToAndroidLogPriority and values in LogSeverity");

    if(pipe_stdout_) {
      printf("%s\n", msg.c_str());
    }

    if(data_->GetMcrLogType() == McrLog::NORMAL) {
      ::android::base::LogMessage::LogLine(
          data_->GetFile(),
          data_->GetLineNumber(),
          data_->GetId(),
          data_->GetSeverity(),
          data_->GetTag(),
          msg.c_str());
    } else {
      DLOG(FATAL) << "Uknown McrLog type";
    }
  }

}  // namespace mcr
}  // namespace art
