/**
 * Some helper methods for the interfaces (wrappers) over utilities like:
 * - opt, llc, clang (lld linker)
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
#include "mcr_cc/compiler_interface.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "base/logging.h"
#include "base/time_utils.h"
#include "mcr_rt/mcr_rt.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_rt/utils.h"

namespace art {
namespace mcr {

void CompilerInterface::cdSrcDir() {
  std::string dir_src = GetDirAppHfSrc(McrCC::GetLlvmEntrypointStripped());
  D2LOG(INFO) << "chdir(" << dir_src << ")";
  if (chdir(dir_src.c_str()) == -1) {
    DLOG(FATAL) << __func__ << ": Failed to chdir:" << dir_src
               << " ERROR: " << errno;
  }
}

bool CompilerInterface::CHMOD(std::string file, std::string perms) {
#ifdef ART_MCR_ANDROID_6
  std::string cmd = "chmod " + perms + " " + file;
  return exe(cmd);
#else
  return true;
#endif
}

/*
 * In case of an error it returns an empty string.
 * Warnings are not considered as errors.
 */
bool CompilerInterface::EXE(std::string orig_cmd, bool print_output,
    bool increased_timeout) {
  std::string result;
  bool long_output = false;
  orig_cmd = McrCC::remove_extra_whitespaces(orig_cmd);

  std::string timeout = "timeout ";
  if (increased_timeout) {
    timeout += "120s ";
  } else {
    timeout += "60s ";
  }

  std::string cmd = timeout + orig_cmd + " 2>&1";  // redirect error on stdout
  D4LOG(INFO) << "EXEC: " << cmd;

  uint64_t timer_start = NanoTime();
  FILE* pp = popen(cmd.c_str(), "r");
  if (!pp) {
    DLOG(ERROR) << "Failed to popen command: " << cmd;
    return false;
  }

#define MAX_RESULT_SIZE 5000
  char buffer[256];
  while (!feof(pp)) {
    if (fgets(buffer, 256, pp) != NULL) {
      result += buffer;
      if (result.size() > MAX_RESULT_SIZE) {
        long_output = true;
        result.clear();
      }
    }
  }

  int exit_code = WEXITSTATUS(pclose(pp));

  if (print_output) {
    if (result.size() > 0) {
      D3LOG(INFO) << "exe(): output: " << (long_output ? " [TRIMMED] " : "") << result;
    }

    std::string extra_info;

    // compilation error
    if (result.find("error") != std::string::npos ||
        (result.find(": not found") != std::string::npos)) {
      extra_info += "Compiler:";
    }
    // link error
    if (result.find("clang: error:") != std::string::npos) {
      DLOG(ERROR) << "Linker: error: " << result.c_str();
      extra_info += "Linker:";
    }
    // warning
    if (result.find("warning:") != std::string::npos) {
      extra_info += "WARNING:";
      D2LOG(WARNING) << "WARNING: " << result.c_str() << "\nCMD:" << orig_cmd;
    }

    // TODO dynamically enable/disable through file
    if (exit_code != 0 && print_output) { 
      std::stringstream ss;
      ss << "Error:" << extra_info << " CMD: " << orig_cmd
         << "\nInfo: " << result;
      McrCC::LogLongMessage(ss.str(), ERROR);
    }
  }

  uint64_t total_time= NanoTime() - timer_start; 
  if (print_output) {
    D3LOG(WARNING) << "exe: time: " << PrettyDuration(total_time);
    if (total_time >= 5*1000) {
      DLOG(WARNING) << "Long exec time: " << PrettyDuration(total_time)
                   << ": " << cmd;
    }
  }
  return (exit_code == 0);
}

}  // namespace mcr
}  // namespace art
