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
#ifndef ART_COMPILER_MCR_OS_COMP_H
#define ART_COMPILER_MCR_OS_COMP_H

#ifndef ART_MCR_COMPILE_OS_METHODS
#error ENABLE ART_MCR_COMPILE_OS_METHODS
#endif

#include <set>
#include <string>
#include <vector>
#include "art_method.h"
#include "base/macros.h"
#include "dex/invoke_type.h"

#define FILE_OS_BLOCKLIST "os_methods.blocklist"
#define FILE_OS_COMP_FAILED "os_methods.comp.failed"

namespace art {

class DexFile;
class MethodReference;

class OsDexMethod final {
 public:
  OsDexMethod(uint32_t method_idx, InvokeType invoke_type)
      : method_idx_(method_idx),
        invoke_type_(invoke_type) {
    is_native_ = false;
    is_ctor_ = false;
  }

  uint32_t GetMethodIdx() const {
    return method_idx_;
  }
  InvokeType GetInvokeType() const {
    return invoke_type_;
  }

  void SetIsNative(bool is_native) {
    is_native_ = is_native;
  }

  bool IsMethodNative() const {
    return is_native_;
  }

  bool IsConstructor() const {
    return is_ctor_;
  }

  void SetIsConstructor(bool v) {
    is_ctor_ = v;
  }

  bool CanCompile() const {
    return !(IsMethodNative());
  }

 private:
  uint32_t method_idx_;
  InvokeType invoke_type_;
  bool is_native_;
  bool is_ctor_;
};

class OsDexClass final {
 public:
  OsDexClass(uint32_t class_idx)
      : class_idx_(class_idx) {
  }

  uint32_t GetClassIdx() const {
    return class_idx_;
  }

  std::vector<OsDexMethod*> GetMethods() const {
    return methods_;
  }

  void AddMethod(uint32_t method_idx, InvokeType invoke_type);

  bool IsCompileMethod(uint32_t method_idx) {
    return MethodExists(method_idx);
  }

 private:
  bool MethodExists(uint32_t method_idx);

  uint32_t class_idx_;
  std::vector<OsDexMethod*> methods_;
};

class OsDexFile final {
 public:
  OsDexFile(const DexFile* df)
      : dex_file_(df) {
  }

  const DexFile* GetDexFile() {
    return dex_file_;
  }

  std::vector<OsDexClass*> GetClasses() const {
    return classes_;
  }

  OsDexClass* GetOsDexClass(uint32_t class_idx);
  void AddMethod(uint32_t class_idx, uint32_t method_idx, InvokeType invoke_type);

 private:
  const DexFile* dex_file_;
  std::vector<OsDexClass*> classes_;
};

class OsCompilation final {
 public:
  static const DexFile* AddDexFile(std::string dexFilename, std::string dexLocation);
  static void AddMethod(const DexFile* dex_file, uint32_t class_idx,
                        uint32_t method_idx, InvokeType invoke_type);
  static void PrintReport();
  static void GetDexFiles(std::vector<const DexFile*>& dex_files);
  static bool IsCompileMethod(MethodReference method_ref);
  static bool IsCompileMethod(uint32_t method_idx, const DexFile* dex_file);
  static bool IsOsMethodCompiled(MethodReference method_ref);
  static void AddOsMethod(MethodReference method_ref, bool compiled);

  static void ReadOsMethodsBlocklist();
  static void ReadOsMethodsCantCompile(bool append = false);
  static bool IsOsMethodsBlocklisted(std::string method_name);

  static std::string GetOsMethodsBlocklistFilename();
  static std::string GetOsMethodsCompFailedFilename();
  static void UpdateOsMethodsCantCompile();
  static void AddOsMethodCantCompile(MethodReference method_ref);
  static void SetOsCompilationDone();

  static std::vector<OsDexFile*> GetOsDexFiles() { return os_dex_files_; }
  static bool IsOsCompilationDone() { return os_comp_done_; }

 private:
  ALWAYS_INLINE static OsDexFile* GetOsDexFile(const DexFile* dex_file);
  static void AddOsMethodCompiled(MethodReference method_ref);

  static std::vector<OsDexFile*> os_dex_files_;
  static std::vector<MethodReference> os_compiled_methods_;
  static bool os_comp_done_;
  static std::set<std::string> os_methods_blocklist_;
  static std::set<std::string> os_methods_comp_failed_;
};

}  // namespace art

#endif  // ART_COMPILER_MCR_OS_COMP_H
