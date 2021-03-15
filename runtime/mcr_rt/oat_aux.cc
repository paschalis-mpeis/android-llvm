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
#include "mcr_rt/mcr_rt.h"
#include "mcr_rt/oat_aux.h"

#include <fstream>
#include "art_method-inl.h"
#include "art_method.h"
#include "base/os.h"
#include "dex/dex_file.h"
#include "entrypoints/quick/quick_default_externs.h"
#include "mcr_rt/filereader.h"
#include "mcr_rt/opt_interface.h"
#include "mcr_rt/utils.h"

namespace art {
namespace mcr {

// compiled_methods: set and used by compiler
// stripped_methods: set by compiler, and then used by RT
// on initial loading of the app, to pre-load the llvm code (hf.so)
// rt_methods: used by RT, and should be faster (map of ArtMethod pointers)
std::map<std::string, uint32_t> OatAux::stripped_methods_;
CompiledMethods OatAux::compiled_methods_;
RtMethods OatAux::rt_methods_;

bool ArtMethodAuxCompiledComparator::operator()(ArtMethodAux ao1, ArtMethodAux ao2) const {
  return ao1.GetMethodReference() < ao2.GetMethodReference();
}

/**
 * @brief For both cases of ArtMethodAux
 */
std::ostream& operator<<(std::ostream& os, const ArtMethodAux& ao) {
  if(ao.GetMethodReference().index != 0){ 
    os << ao.GetMethodReference().PrettyMethod();
  } else {
    os << "NAN";
  }
  return os;
}

void OatAux::SetIchf(ArtMethod* method, bool val) {
  std::pair p(method, ArtMethodAux(val));
  rt_methods_.insert(p);
}

bool OatAux::IsIchf(MethodReference mref) {
  ArtMethodAux maux(mref, false);
  auto it = compiled_methods_.find(maux);
  if (it != compiled_methods_.end()) {
    D3LOG(WARNING) << "IS ICHF: " << maux;
    return it->IsCompiled();
  }
  return false;
}

bool OatAux::IsIchfSLOW(ArtMethod *method) {
  bool ichf = false;
  if(method==nullptr) return false;



  auto it = rt_methods_.find(method);
  if (it != rt_methods_.end()) {
    ichf = it->second.IsCompiled();
  }

#ifdef CRDEBUG5
  Thread* const self = Thread::Current();
  Locks::mutator_lock_->SharedLock(self);
  std::string pretty_method = method->PrettyMethod();
  Locks::mutator_lock_->SharedUnlock(self);

  DLOG(WARNING) << __func__ << ": " << (ichf?"yes":"no")
    << ": " << pretty_method;
#endif
  return ichf;
}

ArtMethodAux* OatAux::GetArtMethodAux(ArtMethod *method) {
  auto it = rt_methods_.find(method);
  if (it != rt_methods_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::string OatAux::GetFileOatAux() {
  return GetFileApp(FILE_OAT_AUX);
}

void OatAux::AddCompiledMethod(const DexFile* dex_file, uint32_t method_idx, bool val) {
  ArtMethodAux method_aux(ArtMethodAux(dex_file, method_idx, val));
  DLOG(INFO) << __func__ << ": " << method_aux.GetMethodReference().PrettyMethod();
  compiled_methods_.insert(method_aux);
}

void OatAux::AddCompiledMethod(MethodReference mref, bool val) {
  DLOG(INFO) << __func__ << ": " << mref.PrettyMethod();
  compiled_methods_.insert(ArtMethodAux(mref, val));
}

/**
 * @brief Iterares over ichf_methods_ (compiled methods)
 *        and stores the stripped hf (from pretty method) and the method index
 *        into the oat aux file, which will be loaded on application load.
 */
void OatAux::StoreOatAux() {
  D3LOG(INFO) << __func__;
  std::string filename_oat_aux(GetFileOatAux());
  std::ofstream out(filename_oat_aux, std::ios::out | std::ios::binary);

  D3LOG(INFO) << __func__ <<  ": " << compiled_methods_.size() << " methods.";
  for(const ArtMethodAux maux: compiled_methods_) {
    std::stringstream ss;
    ss << maux;
    std::string stripped_hf = StripHf(ss.str());
    ss.str("");

    ss << stripped_hf << " "
      << std::to_string(maux.GetMethodReference().index);
    D5LOG(INFO) << "StoreOatAux: " << ss.str();
    out << ss.str() << std::endl;
  }
  out.close();
}

/**
 * @brief Reads oat aux and loads into the stripped_methods.
 * It also loads the LLVM compiled code.
 * it relies on the stripped_hf because we do not have the 
 * full ArtMethod name yet. However the stripped_hf matches the folder
 * folder created during compilation, which is what is needed for 
 * dynamically linking llvm code (hf.so).
 *
 * LLVM_CHECK where art_method ->SetIchf is loaded?
 */
void OatAux::ReadOatAux() {
  D3LOG(INFO) << __func__;
  compiled_methods_.clear();
  std::string filename_oat_aux(GetFileOatAux());

  if (!OS::FileExists(filename_oat_aux.c_str())) {
    DLOG(ERROR) << "LLVM was enabled but no code found (OatAux)\n"
      << "Were any methods compiled?\n"
      << "Disabling LLVM for this app run.";
    McrRT::DisableLlvm();
    return;
  }

  mcr::FileReader fr(__func__, filename_oat_aux, false, false, false);
  std::vector<std::string> data = fr.GetData();
  for (std::string line : data) {
    size_t p = line.find(' ');
    CHECK(p != std::string::npos);
    std::string shf= line.substr(0, p);
    std::string sdidx = line.substr(p + 1);
    uint32_t didx = std::stoul(sdidx);

    std::pair<std::string, uint32_t> pair(shf, didx);
    stripped_methods_.insert(pair);

    if (McrRT::IsLlvmEnabled()) {
      mcr::OptimizingInterface::LoadCodeForMethod(shf, didx);
      D2LOG(WARNING) << "Loaded LLVM code for: " << shf;
    }
  }
}

/**
 * @brief TODO do this once, after loading Oat file.
 *        Is GetMethodIndexDuringLinking helpful?
 *        But we will need access to all classes of the dexfiles,
 *        so we can get the methods.
 *        
 *        The method to do this should look something like the above method.
 *        It will be better to get ArtMethods dynamically and not iterate
 *        every single ART method of an app.
 *
 * @param method
 */
void OatAux::SetMethodAuxData(ArtMethod* method) {
  std::string pretty_method = method->PrettyMethod();
  std::string stripped_hf = StripHf(pretty_method);
  if (stripped_methods_.find(stripped_hf) != stripped_methods_.end()) { 
    D4LOG(INFO) << "OatAux: SetICHF:  " << stripped_hf;
    SetIchf(method);

    JNIEnvExt* env = Thread::Current()->GetJniEnv();
    ScopedObjectAccessUnchecked soa(env);
    ClassLinker* linker = Runtime::Current()->GetClassLinker();
    StackHandleScope<1> hs(soa.Self());
    Handle<mirror::Class> klass(hs.NewHandle(method->GetDeclaringClass()));

    // Get the resolved quick code, so we can temporarily store it to JNI
    // (this is a hack so we won't 'loose' ptr to quick_code)
    const void* quick_code = nullptr;
    if (LIKELY(klass->IsInitialized())) {
      quick_code = method->GetEntryPointFromQuickCompiledCode();
    } else {
      if (method->IsStatic()) {
        // Class is still initializing, go to oat and grab code
        // (trampoline must be left in place until class is initialized
        // to stop races between threads).
        quick_code = linker->GetQuickOatCodeFor(method);
      } else {
        // No trampoline for non-static methods.
        quick_code = method->GetEntryPointFromQuickCompiledCode();
      }
    }

    if(McrRT::IsLlvmEnabled()) {
      const void* llvm_code = reinterpret_cast<const void*>(art_quick_to_llvm_bridge);
      D4LOG(WARNING) << "OatAux: ForceQtoLlvm: " << pretty_method;
      method->SetQuickToLlvm(quick_code, llvm_code);
    } else {
      D4LOG(WARNING) << "OatAux: ForceQtoI: " << pretty_method;
      method->SetQuickToInterpreter(quick_code);
    }
  }
}

}  // namespace mcr
}  // namespace art
