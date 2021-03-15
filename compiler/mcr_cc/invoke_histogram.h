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
#ifndef ART_COMPILER_MCR_INVOKE_HISTOGRAM_H_
#define ART_COMPILER_MCR_INVOKE_HISTOGRAM_H_

#include <iterator>
#include <map>
#include <set>
#include <string>

#include "base/macros.h"
#include "base/mutex.h"
#include "mcr_rt/invoke_info.h"

namespace art {

struct InvokeInfoComparator {
  bool operator()(mcr::InvokeInfo o1, mcr::InvokeInfo o2) const {
    return o1 < o2;
  }
};

/**
 * @brief Special Sorting: if cnts (number of times method was resolved)
 *        don't match, then sort descenting.
 *        if they do, then sort ascending based on InvokeInfo
 *        
 *        int here is number of times method was resolved to
 */
struct SortedSpeculationReverseCmp {
  bool operator()(mcr::InvokeInfo p1,
                  mcr::InvokeInfo p2) const {
    if (p2.GetInvokeTimes() != p1.GetInvokeTimes()) {
      return p2.GetInvokeTimes() < p1.GetInvokeTimes();
    }

    return false;
  }
};

// Sorting speculation based on number of times the method was resolved to
// To give priority to the method that is most likely to call
// typedef std::map<mcr::InvokeInfo, int, InvokeInfoComparator> InvokeInfoMap;
typedef std::set<mcr::InvokeInfo, SortedSpeculationReverseCmp>
    SortedSpeculation;

typedef SortedSpeculation::iterator SortedSpeculationIterator;

class InvokeHistogram final {
 public:
  InvokeHistogram();
  InvokeHistogram(const char* dex_location);
  InvokeHistogram(uint32_t caller_method_idx, uint32_t dex_pc, const char* dex_location);
  void GenerateSpeculationMap();

  bool HasSpeculation() const;
  std::string str();

  uint32_t GetSize() {
    if (load_all_) {
      return histogram_.size();
    }
    return speculations_.size();
  }

  std::set<mcr::InvokeInfo>::iterator hist_begin() {
    return histogram_.begin();
  }

  std::set<mcr::InvokeInfo>::iterator hist_end() {
    return histogram_.end();
  }

  SortedSpeculationIterator begin() {
    return speculations_.begin();
  }
  SortedSpeculationIterator end() {
    return speculations_.end();
  }

 private:
  bool load_all_;
  bool filter_callee_dexfile_ = false;
  void Load();
  uint32_t caller_method_idx_;
  uint32_t dex_pc_;
  char dex_location_[500];
  std::set<mcr::InvokeInfo> histogram_;

  // histogram tha concerns the method we care about
  std::set<mcr::InvokeInfo, SortedSpeculationReverseCmp> speculations_;
  // used in compiler so we can see what internal android methods we need to compile
  // and try to do so in a pass that precedes the normal app compilation
};

}  // namespace art

#endif  // ART_COMPILER_MCR_INVOKE_HISTOGRAM_H_
