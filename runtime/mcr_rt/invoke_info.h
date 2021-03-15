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
#ifndef ART_RUNTIME_MCR_INVOKE_INFO_H_
#define ART_RUNTIME_MCR_INVOKE_INFO_H_

#include <map>
#include <ostream>
#include <sstream>
#include "base/macros.h"
#include "dex/invoke_type.h"

#define FILE_INVOKE_HISTOGRAM "invoke.hist"

namespace art {
namespace mcr {

class InvokeInfo final {
 public:
  // INFO using 0 for caller_method_idx and dex_pc in the case of static OS calls
  // which will also have always 1 invoke_times, because there is no need
  // to do a weighted Histogram for those. Its a direct call.
  // We just needed a way to add static OS calls to the compilation process
  InvokeInfo(uint16_t caller_method_idx, uint32_t dex_pc,
             uint16_t spec_class_idx, uint32_t spec_method_idx,
             InvokeType spec_invoke_type,
             std::string dex_location_caller,
             std::string dex_filename, std::string dex_location,
             uint32_t invoke_times)
      : caller_method_idx_(caller_method_idx),
        dex_pc_(dex_pc),
        spec_class_idx_(spec_class_idx),
        spec_method_idx_(spec_method_idx),
        spec_iinvoke_type_(static_cast<int>(spec_invoke_type)),
        dex_location_caller_(dex_location_caller),
        dex_filename_(dex_filename),
        dex_location_(dex_location),
        invoke_times_(invoke_times) {
  }
  InvokeInfo(uint16_t caller_method_idx, uint32_t dex_pc,
             uint16_t spec_class_idx, uint32_t spec_method_idx,
             InvokeType spec_invoke_type,
             std::string dex_location_caller,
             std::string dex_filename, std::string dex_location)
      : InvokeInfo(caller_method_idx, dex_pc,
                   spec_class_idx, spec_method_idx,
                   spec_invoke_type,
                   dex_location_caller,
                   dex_filename, dex_location, 1) {
  }

  static std::map<std::string, uint32_t> shistogram_;

  static std::string GetFilename();
  static void AddToCache(InvokeInfo info);
  static bool ShouldUpdateHistogram();
  static void UpdateHistogram();
  static InvokeInfo ParseLine(std::string line);

  uint32_t GetCallerMethodIdx() const {
    return caller_method_idx_;
  }

  uint32_t GetDexPC() const {
    return dex_pc_;
  }

  uint32_t GetSpecClassIdx() const {
    return spec_class_idx_;
  }

  uint32_t GetSpecMethodIdx() const {
    return spec_method_idx_;
  }

  uint32_t GetSpecInvokeTypeInt() const {
    return spec_iinvoke_type_;
  }

  InvokeType GetSpecInvokeType() const {
    return static_cast<InvokeType>(spec_iinvoke_type_);
  }

  std::string GetDexLocationCaller() const {
    return dex_location_caller_;
  }

  std::string GetDexFilename() const {
    return dex_filename_;
  }

  std::string GetDexLocation() const {
    return dex_location_;
  }

  uint32_t GetInvokeTimes() const {
    return invoke_times_;
  }

  void IncreaseInvokeTimes() {
    invoke_times_++;
  }

  std::string str() const;
  std::string str_detailed() const;

  bool IsInternalMethod() const;

  const static char sep_ = ',';

 private:
  const uint32_t caller_method_idx_;
  const uint32_t dex_pc_;
  const uint32_t spec_class_idx_;
  const uint32_t spec_method_idx_;
  const uint32_t spec_iinvoke_type_;
  // DexFile information is from callee
  const std::string dex_location_caller_;
  const std::string dex_filename_;  // base location
  const std::string dex_location_;
  uint32_t invoke_times_;
};

std::ostream& operator<<(std::ostream& os, const InvokeInfo& rhs);
bool operator<(InvokeInfo const& lhs, InvokeInfo const& rhs);

}  // namespace mcr
}  // namespace art

#endif  // ART_RUNTIME_MCR_INVOKE_INFO_H_
