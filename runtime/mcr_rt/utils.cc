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
#include "mcr_rt/utils.h"

#include "mcr_rt/mcr_rt.h"

#include <ctype.h>
#include <sys/stat.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "base/logging.h"
#include "base/os.h"

namespace art {
namespace mcr {

std::string PrettyPrimitive(Primitive::Type type) {
  std::stringstream ss;
  ss << type;

  std::string res = ss.str();
  res = res.substr(res.find("Prim"));

  if (res == "Not") {
    res = "Object";
  }

  std::transform(res.begin(), res.end(), res.begin(), ::tolower);

  return res;
}

void checkFileExists(std::string file) {
  struct stat sb;
  if (!(stat(file.c_str(), &sb) == 0 && !S_ISDIR(sb.st_mode))) {
    DLOG(ERROR) << "File not found: " << file;
  }
}

void checkFolderExists(std::string folder) {
  if (!OS::DirectoryExists(folder.c_str())) {
    DLOG(FATAL) << "Directory not found: " << folder;
  }
}

#ifdef __ANDROID__
void CheckDirExists(std::string dir) {
  if (!OS::DirectoryExists(dir.c_str())) {
    umask(0);
    __mkdir(dir.c_str(), 0777);
  }

  checkFolderExists(dir);
}
#endif

std::string GetDirAppHfSrc(std::string hf) {
  std::string shf = StripHf(hf);
  std::string dir_src = McrRT::GetAppHfDir(shf, true) 
    + std::string("/") + std::string(FOLDER_SRC);
  CheckDirExists(dir_src);

  return dir_src ;
}

std::string GetFileSrc(std::string pretty_hf, std::string filename) {
  return GetDirAppHfSrc(StripHf(pretty_hf)) + "/" + filename;
}

std::string GetFileAppHf(std::string hf, std::string filename) {
  return McrRT::GetAppHfDir(hf) + "/" + filename;
}

std::string GetFileApp(std::string filename) {
  return McrRT::GetDirApp() + "/" + filename;
}

#ifdef __ANDROID__
/**
 * from :MIRGraph::ReplaceSpecialChars
 */
const char* StripHf(std::string hf) {
  static const struct {
    const char before;
    const char after;
  } match[] = {
    { '/', '-' }, { ';', '#' }, { ' ', '#' }, { '$', '+' }, { '(', '@' }, { ')', '@' }, { '<', '=' }, { '>', '=' }, { '[', '-' }, { ']', '-' }
  };

  if (hf.find("(") == std::string::npos) { // already stripped
    return hf.c_str();
  }

  for (unsigned int i = 0; i < sizeof(match) / sizeof(match[0]); i++) {
    std::replace(hf.begin(), hf.end(), match[i].before, match[i].after);
  }

  // INFO filename limit in unix is 255, and we will use one to store code, images, etc
  // we also append "hf_" in front of the method name, so we reduce a few more characters
  if (hf.size() > 250) {
    hf = __HashCode(hf.c_str());
  }

  return hf.c_str();
}
#endif

std::vector<std::string> SplitString(const std::string& s,
                                     char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}


inline std::string toupper(const char* str) {
  std::string res;
  for (uint32_t i = 0; i < strlen(str); i++) {
    if (!isdigit(str[i]) && islower(str[i])) {
      res += (char)(str[i] & ~32);
    } else {
      res += str[i];
    }
  }

  return res;
}

inline uint8_t addToByte(int b, int n) {
  int r = b + n;
  int c = (r % 256);
  uint8_t res = static_cast<uint8_t>(c);
  return res;
}

inline std::string to_hex(uint8_t b) {
  int i = b;

  std::stringstream ss;
  ss << std::hex << i;
  return ss.str();
}

/*
 hf_ must stay the same!
 should be hf_<hashcode>
 */
std::string __HashCode(const char* str) {
  int l = strlen(str);
  std::string res;
  uint8_t c = 0;

  for (int i = 0; i < l; i++) {
    if (i % 20) {
      c = addToByte(c, str[i]);
    } else {
      if (c != 0) {
        std::string cc = toupper(to_hex(c).c_str());
        res += cc;
      }
      res += str[i];
      c = 0;
    }
  }
  return res;
}

uint64_t ProcessCpuNanoTime() {
#if defined(__linux__)
  timespec now;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
  return static_cast<uint64_t>(now.tv_sec) * UINT64_C(1000000000) + now.tv_nsec;
#else  // __APPLE__
  UNIMPLEMENTED(WARNING);
  return -1;
#endif
}

std::string GetEnvVar(std::string const& key) {
  char const* val = getenv(key.c_str());
  return val == NULL ? std::string() : std::string(val);
}

/*
 * Appends to the envar if it does not already exist
 */
void AppendEnvVar(std::string envvar, std::string data) {
  std::string curVal = GetEnvVar(envvar);
  D3LOG(INFO) << "AppendEnvVar(): " << data;
  // It does not exist
  if (curVal.find(data) == std::string::npos) {
    setenv(envvar.c_str(), std::string(curVal + ":" + data).c_str(), true);
  }
}

inline void _vlog(const char* file, const int line, const char* caller,
    int severity, const char* tag, const char* format,
                                va_list args) {
  char debug_info[200], error_info[500], log_msg[1000];
  debug_info[0] = error_info[0] = log_msg[0] = '\0';

  if (severity == ANDROID_LOG_FATAL) {
    sprintf(debug_info, "%s:%d: %s:", file, line, caller);
  }

  vsprintf(log_msg, format, args);

#define _ALOG(severity, tag, ...) \
  __android_log_print(severity, tag, __VA_ARGS__)

  _ALOG(severity, tag, "%s%s", debug_info, log_msg);

#undef _ALOG
}

void vlog(const char* file, const int line, const char* caller,
    int art_severity, const char* tag,
    const char* fmt, va_list va_args) {
  // map ART enum LogSeverity to Log severity defines from NDK
  int ndk_severity = ANDROID_LOG_DEFAULT;
  switch (art_severity) {
    case VERBOSE:
      ndk_severity = ANDROID_LOG_VERBOSE;
      break;
    case DEBUG:
      ndk_severity = ANDROID_LOG_DEBUG;
      break;
    case INFO:
      ndk_severity = ANDROID_LOG_INFO;
      break;
    case WARNING:
      ndk_severity = ANDROID_LOG_WARN;
      break;
    case ERROR:
      ndk_severity = ANDROID_LOG_ERROR;
      break;
    case FATAL:
      ndk_severity = ANDROID_LOG_FATAL;
      break;
    default:;
  }

  _vlog(file, line, caller, ndk_severity, tag, fmt, va_args);
}

}  // namespace mcr
}  // namespace art
