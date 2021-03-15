/**
 * Compiles methods that belong to the OS/framework. This essentially
 * makes more code available to LLVM, and reduces the amount of times
 * needed to call back to the runtime.
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
#include "mcr_cc/os_comp.h"

#include <regex>
#include "art_method-inl.h"
#include "base/logging.h"
#include "base/mutex.h"
#include "dex/dex_file.h"
#include "dex/dex_file_loader.h"
#include "mcr_rt/filereader.h"
#include "mcr_rt/mcr_rt.h"
#include "dex/method_reference.h"
#include "thread-inl.h"
#include "thread.h"

namespace art {

bool OsCompilation::os_comp_done_ = false;
std::set<std::string> OsCompilation::os_methods_blocklist_;
std::set<std::string> OsCompilation::os_methods_comp_failed_;
std::vector<OsDexFile*> OsCompilation::os_dex_files_;
std::vector<MethodReference> OsCompilation::os_compiled_methods_;

ALWAYS_INLINE const DexFile* GetDexFile(std::vector<OsDexFile*> os_dex_files,
                                        std::string dexFilename, std::string dexLocation) {
  for (OsDexFile* odf : os_dex_files) {
    const DexFile* dex_file = odf->GetDexFile();
    std::string dl = dex_file->GetLocation();
    if ((dexFilename.compare(DexFileLoader::GetBaseLocation(dl)) == 0) &&
        dexLocation.compare(dl) == 0) {
      return dex_file;
    }
  }
  return nullptr;
}

const DexFile* OsCompilation::AddDexFile(std::string dexFilename,
                                         std::string dexLocation) {
  const DexFile* dex_file = GetDexFile(os_dex_files_, dexFilename, dexLocation);
  if (dex_file == nullptr) {
    dex_file = mcr::McrRT::OpenDexFileOS(dexFilename, dexLocation);
    OsDexFile* odf = new OsDexFile(dex_file);
    os_dex_files_.push_back(odf);
    D3LOG(INFO) << "Added dex file: " << dexFilename << " loc:" << dexLocation;
  }
  return dex_file;
}

ALWAYS_INLINE OsDexFile* OsCompilation::GetOsDexFile(const DexFile* dex_file) {
  for (OsDexFile* odf : os_dex_files_) {
    if (odf->GetDexFile() == dex_file) {
      return odf;
    }
  }
  return nullptr;
}

void OsCompilation::AddMethod(const DexFile* dex_file, uint32_t class_idx,
                              uint32_t method_idx, InvokeType invoke_type) {
  // Ignore uncompilable and blocklisted OS methods
  MethodReference method_ref(dex_file, method_idx);
  std::string pretty_method = method_ref.PrettyMethod();
  if (IsOsMethodsBlocklisted(pretty_method)) {
    LOG(WARNING) << "Skip OS method: " << pretty_method;
    return;
  }

  D3LOG(WARNING) << "Add OS method: " << pretty_method;
  OsDexFile* odf = GetOsDexFile(dex_file);
  CHECK(odf != nullptr) << "AddMethod: dex_file null";
  odf->AddMethod(class_idx, method_idx, invoke_type);
}

void OsCompilation::GetDexFiles(std::vector<const DexFile*>& dex_files) {
  dex_files.clear();
  for (OsDexFile* odf : os_dex_files_) {
    dex_files.push_back(odf->GetDexFile());
  }
}

void OsCompilation::PrintReport() {
  DLOG(WARNING) <<  "+----------------------------------------------"
    << "\n| OsCompilation Report: "
    << "\n| DexFiles: " << os_dex_files_.size();
  for (OsDexFile* odf : os_dex_files_) {
    DLOG(INFO) << "+-- Classes: " << odf->GetClasses().size()
      << " (DexFile: " << odf->GetDexFile()->GetLocation() << ")";
    for (OsDexClass* odc : odf->GetClasses()) {
      DLOG(INFO) << "+- Class: " << odc->GetClassIdx()
        << " Methods: " << odc->GetMethods().size();
      for (OsDexMethod* odm : odc->GetMethods()) {
        DLOG(INFO) << "| Method: " << odf->GetDexFile()->PrettyMethod(odm->GetMethodIdx());
      }
    }
  }
  DLOG(WARNING) <<  "\n+---------------------------------------------";
}

OsDexClass* OsDexFile::GetOsDexClass(uint32_t class_idx) {
  for (OsDexClass* cl : classes_) {
    if (cl->GetClassIdx() == class_idx) {
      return cl;
    }
  }
  return nullptr;
}

bool OsCompilation::IsCompileMethod(MethodReference method_ref) {
  return IsCompileMethod(method_ref.index, method_ref.dex_file);
}

bool OsCompilation::IsCompileMethod(uint32_t method_idx, const DexFile* dex_file) {
  for (OsDexFile* odf : GetOsDexFiles()) {
    if (dex_file->GetLocation().compare(
            odf->GetDexFile()->GetLocation()) == 0) {
      for (OsDexClass* odc : odf->GetClasses()) {
        for (OsDexMethod* odm : odc->GetMethods()) {
          if (odm->GetMethodIdx() == method_idx) {
            std::string pretty_method(dex_file->PrettyMethod(method_idx));
            // exclude blocklisted os apps
            return !IsOsMethodsBlocklisted(pretty_method) &&
                   !odm->IsMethodNative();
          }
        }
      }
    }
  }
  return false;
}

bool OsCompilation::IsOsMethodCompiled(MethodReference method_ref) {
  if (!os_comp_done_) {
    return IsCompileMethod(method_ref);
  } else {
    for (MethodReference mr : os_compiled_methods_) {
      if (mr == method_ref) {
        D2LOG(INFO) << "IsOsMethodCompiled: YES: " << method_ref.PrettyMethod() << "/"
                    << method_ref.index << ":" << method_ref.dex_file->GetLocation();
        return true;
      }
    }
    return false;
  }
}

void OsCompilation::AddOsMethod(MethodReference method_ref, bool compiled) {
  if (compiled) {
    AddOsMethodCompiled(method_ref);
  } else {
    AddOsMethodCantCompile(method_ref);
  }
}
void OsCompilation::AddOsMethodCompiled(MethodReference method_ref) {
  os_compiled_methods_.push_back(method_ref);
}

void OsDexFile::AddMethod(uint32_t class_idx, uint32_t method_idx, InvokeType invoke_type) {
  OsDexClass* os_class = GetOsDexClass(class_idx);
  if (os_class == nullptr) {
    D3LOG(INFO) << "AddDexMethod:: Adding new class: " << class_idx;
    os_class = new OsDexClass(class_idx);
    classes_.push_back(os_class);
  }
  os_class->AddMethod(method_idx, invoke_type);
}

void OsDexClass::AddMethod(uint32_t method_idx, InvokeType invoke_type) {
  if (!MethodExists(method_idx)) {
    methods_.push_back(new OsDexMethod(method_idx, invoke_type));
  }
}

bool OsDexClass::MethodExists(uint32_t method_idx) {
  for (OsDexMethod* odm : methods_) {
    if (odm->GetMethodIdx() == method_idx) return true;
  }
  return false;
}

std::string OsCompilation::GetOsMethodsBlocklistFilename() {
  return mcr::McrRT::GetDirMcr() + "/" FILE_OS_BLOCKLIST;
}

std::string OsCompilation::GetOsMethodsCompFailedFilename() {
  return mcr::McrRT::GetDirMcr() + "/" FILE_OS_COMP_FAILED;
}

ALWAYS_INLINE void ReadOsMethodsList(
    std::set<std::string>& s, std::string filename, bool append = false) {
  if (!append) { s.clear(); }
  mcr::FileReader fr(__func__, filename, false, true, true);
  if (fr.exists()) {
    std::vector<std::string> blocklist = fr.GetData();
    for(std::string line: blocklist) { 
      // ignore empty lines and comments
      if(line.size() == 0) continue;
      else if (line.rfind("#", 0) == 0) continue;
      s.insert(line);
      D5LOG(ERROR) << __func__ << ": Added: " + line;
    }
  }
}

void OsCompilation::ReadOsMethodsBlocklist() {
  ReadOsMethodsList(os_methods_blocklist_, GetOsMethodsBlocklistFilename());
  ReadOsMethodsCantCompile();
  LOG(WARNING) << __func__ << ": methods: " << os_methods_comp_failed_.size();
}

void OsCompilation::ReadOsMethodsCantCompile(bool append) {
  ReadOsMethodsList(os_methods_comp_failed_, GetOsMethodsCompFailedFilename(), append);
}

void OsCompilation::AddOsMethodCantCompile(MethodReference method_ref) {
  os_methods_comp_failed_.insert(method_ref.PrettyMethod());
}

void OsCompilation::UpdateOsMethodsCantCompile() {
  std::string filename(GetOsMethodsCompFailedFilename());
  if (os_methods_comp_failed_.size() != 0) {
    std::ofstream out(filename, std::ios::out);
    for (std::string m : os_methods_comp_failed_) {
      out << m << std::endl;
    }
    out.close();
    chmod(filename.c_str(), 0777);
  }
}

bool OsCompilation::IsOsMethodsBlocklisted(std::string method_name) {
  const std::string regex_tag = "regex:";
  for (std::string m : os_methods_blocklist_) {
    bool is_regex = (m.rfind(regex_tag, 0) == 0);
    if (is_regex) {
      std::string sregex = m;
      sregex.erase(0, regex_tag.length());
      D4LOG(ERROR) << "REGEX: '" << sregex << "' ORIG: '" << m << "'";
      if(std::regex_match(method_name, std::regex(sregex) )) {
        return true;
      }
    } else { // exact match
      if (method_name.compare(m) == 0) {
        return true;
      }
    }
  }

  for (std::string m : os_methods_comp_failed_) {
    if (method_name.compare(m) == 0) {
      return true;
    }
  }
  return false;
}

void OsCompilation::SetOsCompilationDone() {
  os_comp_done_ = true;
  UpdateOsMethodsCantCompile();
}

}  // namespace art
