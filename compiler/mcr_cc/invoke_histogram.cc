/**
 * The InvokeHistogram can significantly speedup execution as it 
 * speculatively devirtualizes virtual/interface calls.
 * 
 * This release includes code where the histogram can be created from
 * an actual LLVM invocation of a hot code. 
 *
 * As described on the paper, with a replayed execution we generate
 * the histogram by calling the interpreter. This is slow, but as it is
 * done offline it does not affect the user.
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
#include "mcr_cc/invoke_histogram.h"

#include <algorithm>
#include <string>
#include "mcr_rt/filereader.h"
#include "mcr_rt/mcr_rt.h"
#include "mcr_rt/utils.h"

namespace art {
using namespace mcr;

InvokeHistogram::InvokeHistogram()
    : load_all_(true), filter_callee_dexfile_(false) {
  Load();
}

InvokeHistogram::InvokeHistogram(const char* dex_location)
    : load_all_(true), filter_callee_dexfile_(true) {
  if (dex_location != nullptr) {
    strcpy(dex_location_, dex_location);
  }
  Load();
}

InvokeHistogram::InvokeHistogram(uint32_t caller_method_idx, uint32_t dex_pc, const char* dex_location)
    : load_all_(false), filter_callee_dexfile_(true), caller_method_idx_(caller_method_idx), dex_pc_(dex_pc) {
  if (dex_location != nullptr) {
    strcpy(dex_location_, dex_location);
  }
  Load();
  GenerateSpeculationMap();
}

void InvokeHistogram::Load() {
  std::string filename = InvokeInfo::GetFilename();
  FileReader fr(__func__, filename, false, true, true);
  D4LOG(INFO) << __PRETTY_FUNCTION__;

  if (fr.exists()) {
    for (std::string line : fr.GetData()) {
      InvokeInfo info = mcr::InvokeInfo::ParseLine(line);
      if (!filter_callee_dexfile_ ||  // dex2oat doesn't need this filter!
          info.GetDexLocationCaller().compare(dex_location_) == 0) {
        // the current method
        if (load_all_ || info.GetCallerMethodIdx() == caller_method_idx_) {
          if (McrRT::IsFrameworkDexLocation(info.GetDexLocationCaller()) &&
              !McrRT::IsFrameworkDexLocation(info.GetDexLocation())) {
            D1LOG(WARNING) << "Histogram:: IGNORING: " << line
              << " | OS to app call";
          } else {
            histogram_.insert(info);
          }
        }
      }
    }
  }
}

void InvokeHistogram::GenerateSpeculationMap() {
  D3LOG(INFO) << "GenerateSpeculationMap: dex_pc:" << dex_pc_;
  for (mcr::InvokeInfo ii : histogram_) {
    if (load_all_ || ii.GetDexPC() == dex_pc_) {
      speculations_.insert(ii);
    }
  }

  int s = speculations_.size();
  if (s == 0) {
    // while performing an LLVM execution might update the Histogram,
    // it will also cause a slowdown (as it will be writing to a file).
    // 
    // The histogram really speeds up execution, as the LLVM has more code
    // to compile and optimize, instead of calling the method cold through
    // the runtime.
    D2LOG(WARNING) << "InvokeHistogram is empty!";
  } else {
    D2LOG(INFO) << "InvokeHistogram: Loaded  " << s << " speculations.";
  }
}

std::string InvokeHistogram::str() {
  std::stringstream ss;
  for (mcr::InvokeInfo ii : speculations_) {
    ss << "histogram:size:" << std::to_string(speculations_.size());
    ss << ii;
    ss << ":" << std::to_string(ii.GetInvokeTimes()) << std::endl;
  }

  return ss.str();
}

bool InvokeHistogram::HasSpeculation() const {
    if (load_all_) {
      return histogram_.size() > 0;
    }
    size_t sz = speculations_.size();
    if (sz > 10) {
      DLOG(ERROR) << "Speculation size: " << std::to_string(sz)
                 << " for: " << std::to_string(caller_method_idx_);
    }
    return sz != 0;
  }

}  // namespace art
