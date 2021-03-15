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
#include "mcr_rt/invoke_info.h"
#include "mcr_rt/mcr_rt.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <stdint.h>
#include "mcr_rt/utils.h"
#include "base/os.h"

namespace art {
namespace mcr {

std::map<std::string, uint32_t> InvokeInfo::shistogram_;

std::string InvokeInfo::GetFilename() {
  return mcr::GetFileApp(FILE_INVOKE_HISTOGRAM);
}

InvokeInfo InvokeInfo::ParseLine(std::string line) {
  D3LOG(INFO) << "line: " << line;
  size_t n = std::count(line.begin(), line.end(), InvokeInfo::sep_);
  CHECK(n == 8) << "InvokeInfo: problem parsing line: '" << line
    << "' Tokens: " << n;

  size_t p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string scaller_method_idx = line.substr(0, p);

  // dex location caller
  line = line.substr(p + 1, line.size() - p + 1);
  p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string dex_location_caller = line.substr(0, p);

  // dex filename
  line = line.substr(p + 1, line.size() - p + 1);
  p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string dex_filename = line.substr(0, p);

  // dex location
  line = line.substr(p + 1, line.size() - p + 1);
  p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string dex_location = line.substr(0, p);

  line = line.substr(p + 1, line.size() - p + 1);
  p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string sdex_pc = line.substr(0, p);

  // spec class id
  line = line.substr(p + 1, line.size() - p + 1);
  p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string sspec_class_idx = line.substr(0, p);

  // spec method id
  line = line.substr(p + 1, line.size() - p + 1);
  p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string sspec_method_idx = line.substr(0, p);

  // InvokeType
  line = line.substr(p + 1, line.size() - p + 1);
  p = line.find(InvokeInfo::sep_);
  CHECK(p != std::string::npos);
  std::string sspec_iinvoke_type = line.substr(0, p);

  // Invoke times
  line = line.substr(p + 1, line.size() - p + 1);
  std::string sinvoke_times(line);

  uint32_t caller_method_idx = std::stoul(scaller_method_idx);
  uint32_t dex_pc = std::stoul(sdex_pc);
  uint16_t spec_dex_class_idx = std::stoul(sspec_class_idx);
  uint32_t spec_dex_method_idx = std::stoul(sspec_method_idx);
  InvokeType spec_invoke_type =
      static_cast<InvokeType>(std::stoul(sspec_iinvoke_type));
  uint32_t invoke_times = std::stoul(sinvoke_times);

  return InvokeInfo(caller_method_idx, dex_pc,
                    spec_dex_class_idx, spec_dex_method_idx,
                    spec_invoke_type,
                    dex_location_caller,
                    dex_filename, dex_location, invoke_times);
}

void InvokeInfo::AddToCache(InvokeInfo newinfo) {
  D4LOG(INFO) << "AddToCache: " << newinfo.str();
  auto it = shistogram_.find(newinfo.str());
  if (it != shistogram_.end()) {
    if(it->second < UINT32_MAX) {
      it->second++;
    } else {
      DLOG(WARNING) << "Histogram: UINT32_MAX hits for: " << newinfo;
    }
  } else {
    std::pair<std::string, uint32_t> pair(newinfo.str(), 1);
    shistogram_.insert(pair);
  }
}

bool InvokeInfo::ShouldUpdateHistogram() {
  return !shistogram_.empty();
}

void InvokeInfo::UpdateHistogram() {
  int sz = shistogram_.size();
  if (sz > 0) {
    D4LOG(INFO) << __func__ << ": " << sz << " new entries.";
  } else {
    D2LOG(INFO) << __func__ << ": no new entries.";
    return;
  }

  // load previous entries
  std::string filename = GetFilename();
  if (OS::FileExists(filename.c_str())) {
    std::ifstream in(filename);
    std::string line;
    while (std::getline(in, line)) {
      InvokeInfo ii = ParseLine(line);
      auto it = shistogram_.find(ii.str());
      if (it != shistogram_.end()) {  // update old pair
        it->second += ii.GetInvokeTimes();
      } else {  // new histogram entry
        std::pair<std::string, uint32_t> newpair(
            ii.str(), ii.GetInvokeTimes());
        shistogram_.insert(newpair);
      }
    }
    in.close();
    D3LOG(INFO) << __func__ << ": loaded previous entries!";
  }

  std::ofstream out;
  out.open(filename);
  D3LOG(INFO) << __func__ << ": writing entries: " << shistogram_.size();
  for (auto ii : shistogram_) {
    out << ii.first << sep_ << std::to_string(ii.second) << std::endl;
  }
  out.close();
  chmod(filename.c_str(), 0666);
  D2LOG(INFO) << __func__ << ": SUCCESS: " << sz << " entries.";

  shistogram_.clear();
}

bool InvokeInfo::IsInternalMethod() const {
  return McrRT::IsFrameworkDexLocation(GetDexFilename());
}

std::ostream& operator<<(std::ostream& os, const InvokeInfo& rhs) {
  os << rhs.str();
  return os;
}

std::string InvokeInfo::str() const {
  std::string result;
  result = std::to_string(GetCallerMethodIdx()) +
           InvokeInfo::sep_ + GetDexLocationCaller() +
           InvokeInfo::sep_ + GetDexFilename() +
           InvokeInfo::sep_ + GetDexLocation() +
           InvokeInfo::sep_ + std::to_string(GetDexPC()) +
           InvokeInfo::sep_ + std::to_string(GetSpecClassIdx()) +
           InvokeInfo::sep_ + std::to_string(GetSpecMethodIdx()) +
           InvokeInfo::sep_ + std::to_string(GetSpecInvokeTypeInt());
  return result;
}

std::string InvokeInfo::str_detailed() const {
  std::string result;
  result = "CallerIDX:" + std::to_string(GetCallerMethodIdx()) +
           "\nCallerDexLoc:" + GetDexLocationCaller() +
           "\nDexFile:" + GetDexFilename() +
           "\nDexLoc:" + GetDexLocation() +
           "\nDexPC:" + std::to_string(GetDexPC()) +
           "\nSpecClassIDX:" + std::to_string(GetSpecClassIdx()) +
           "\nSpecMethodIDX:" + std::to_string(GetSpecMethodIdx()) +
           "\nSpecInvokeType:" + std::to_string(GetSpecInvokeTypeInt());
  return result;
}

bool operator<(InvokeInfo const& lhs, InvokeInfo const& rhs) {
  // we store one invoke.hist per app, so we first
  // compare the outer method idx
  if (lhs.GetCallerMethodIdx() == rhs.GetCallerMethodIdx()) {
    int cmp_dex_filename = lhs.GetDexFilename().compare(
        rhs.GetDexFilename());

    // dex filename (apk or jar file)
    if (cmp_dex_filename == 0) {
      int cmp_dex_location = lhs.GetDexLocation().compare(
          rhs.GetDexLocation());

      // classes.dex file inside of an apk or jar
      if (cmp_dex_location == 0) {
        // speculative class
        if (lhs.GetSpecClassIdx() == rhs.GetSpecClassIdx()) {
          // speculative method idx (it's declaring method might
          // not be in the speculative class)
          if (lhs.GetSpecMethodIdx() == rhs.GetSpecMethodIdx()) {
            if (lhs.GetSpecInvokeTypeInt() == rhs.GetSpecInvokeTypeInt()) {
              // invoke id (of HGraph)
              return lhs.GetDexPC() < rhs.GetDexPC();
            } else {
              return lhs.GetSpecInvokeTypeInt() < rhs.GetSpecInvokeTypeInt();
            }
          } else {
            return lhs.GetSpecMethodIdx() < rhs.GetSpecMethodIdx();
          }
        } else {
          return lhs.GetSpecClassIdx() < rhs.GetSpecClassIdx();
        }

      } else {
        return cmp_dex_location < 0;
      }
    } else {
      return cmp_dex_filename < 0;
    }
  } else {
    return lhs.GetCallerMethodIdx() < rhs.GetCallerMethodIdx();
  }
}

}  // namespace mcr
}  // namespace art
