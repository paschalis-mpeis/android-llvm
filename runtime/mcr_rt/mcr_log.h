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
#ifndef ART_RUNTIME_MCR_LOG_H_
#define ART_RUNTIME_MCR_LOG_H_

#include <android-base/logging.h>
#include <iostream>
#include <sstream>
#include <fstream>

namespace art {
namespace mcr {

using namespace android::base;

enum  McrLog {
  NORMAL
}; 


const char* GetFileBasename(const char* file);

class McrLogMessageData {
 public:
  McrLogMessageData(const char* file, unsigned int line, LogId id,
      android::base::LogSeverity severity,
                 const char* tag, int error, McrLog mcr_log)
      : file_(GetFileBasename(file)),
        line_number_(line),
        id_(id),
        severity_(severity),
        tag_(tag),
        error_(error),
        mcr_log_(mcr_log) {}

  const char* GetFile() const {
    return file_;
  }

  unsigned int GetLineNumber() const {
    return line_number_;
  }

  android::base::LogSeverity GetSeverity() const {
    return severity_;
  }

  const char* GetTag() const { return tag_; }

  LogId GetId() const {
    return id_;
  }

  int GetError() const {
    return error_;
  }

  std::ostream& GetBuffer() {
    return buffer_;
  }

  std::string ToString() const {
    return buffer_.str();
  }

  McrLog GetMcrLogType() const {
    return mcr_log_;
  }

 private:
  std::ostringstream buffer_;
  const char* const file_;
  const unsigned int line_number_;
  const LogId id_;
  const android::base::LogSeverity severity_;
  const char* const tag_;
  const int error_;
  const McrLog mcr_log_;

  DISALLOW_COPY_AND_ASSIGN(McrLogMessageData);
};

class McrLogMessage {
  public:
    McrLogMessage(const char* file, unsigned int line, LogId id,
        LogSeverity severity, const char* tag, int error, McrLog mcr_log);

   McrLogMessage(const char* file, unsigned int line, LogId id,
        LogSeverity severity, const char* tag, int error, McrLog mcr_log, bool pipe_stdout):
     McrLogMessage(file, line, id, severity, tag, error, mcr_log)
  { pipe_stdout_ = pipe_stdout; }

  virtual ~McrLogMessage();

  // Returns the stream associated with the message, the LogMessage performs
  // output when it goes out of scope.
  std::ostream& stream();

  protected:
  const std::unique_ptr<McrLogMessageData> data_;
  bool pipe_stdout_=false;

 private:

  DISALLOW_COPY_AND_ASSIGN(McrLogMessage);
};

}  // namespace mcr
}  // namespace art

#endif // ifndef ART_RUNTIME_MCR_LOG_H_
