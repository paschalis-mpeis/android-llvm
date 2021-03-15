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
#ifndef ART_RUNTIME_MCR_OAT_AUX_H_
#define ART_RUNTIME_MCR_OAT_AUX_H_

#include <utility>
#include <string>
#include <map>
#include <set>

#include "base/locks.h"
#include "dex/method_reference.h"

#define FILE_OAT_AUX "oat.aux"
namespace art {
class ArtMethod;
class ArtMethodAux;
class DexFile;
class MethodReference;

namespace mcr {

  /**
   * @brief Used for both CompiledMethods and RtMethods
   *
   */
class ArtMethodAux {
  public:
    // For RT: pointer comparison will be faster
    ArtMethodAux(bool compiled) :
      method_ref_(0, 0),
      compiled_(compiled) { }

    ArtMethodAux(MethodReference mref, bool compiled) :
      method_ref_(mref),
      compiled_(compiled){}

    ArtMethodAux(const DexFile* dex_file,
        uint32_t method_idx,
        bool compiled):
      method_ref_(dex_file, method_idx),
      compiled_(compiled) {}

    MethodReference GetMethodReference() const {
      return method_ref_; } 

    bool IsCompiled() const {
      return compiled_;
    }

  private:
    MethodReference method_ref_;
    // LLVM compiled
    bool compiled_ = false;
};

struct ArtMethodAuxCompiledComparator {
  bool operator()(ArtMethodAux ao1, ArtMethodAux ao2) const; 
};

struct ArtMethodAuxRtComparator {
  bool operator()(ArtMethodAux ao1, ArtMethodAux ao2) const; 
};

typedef std::set<ArtMethodAux, ArtMethodAuxCompiledComparator> CompiledMethods;
typedef std::map<ArtMethod*, ArtMethodAux> RtMethods;

class OatAux {
 public:
   static void AddCompiledMethod(const DexFile* dex_file, uint32_t method_idx,
       bool val=true);
  static void AddCompiledMethod(MethodReference mref, bool val=true); 
  static std::string GetFileOatAux();
  static void StoreOatAux();
  static void ReadOatAux();
  static void LoadData();
  static void SetMethodAuxData(ArtMethod* method)
    REQUIRES_SHARED(Locks::mutator_lock_);
 
  static void SetIchf(ArtMethod *method, bool val=true); 
  static bool IsIchf(MethodReference mref);
  static bool IsIchfSLOW(ArtMethod *method);
  static ArtMethodAux* GetArtMethodAux(ArtMethod *method);

  friend std::ostream& operator<<(std::ostream& os, const ArtMethodAux& ao);

 private:
  static std::map<std::string, uint32_t> stripped_methods_;
  static CompiledMethods compiled_methods_;
  static RtMethods rt_methods_;
};

}  // namespace mcr
}  // namespace art

#endif  // ART_RUNTIME_MCR_OAT_AUX_H_
