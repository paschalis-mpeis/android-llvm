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
#ifndef ART_COMPILER_LLVM_COMPILATION_UNIT_H_
#define ART_COMPILER_LLVM_COMPILATION_UNIT_H_

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/MDBuilder.h>
#include "arch/instruction_set.h"
#include "compiler_tls.h"
#include "intrinsic_helper.h"
#include "llvm_info.h"
#include "optimizing/code_generator.h"

using namespace ::llvm;

namespace art {
class HGraphFilePrettyPrinter;

namespace LLVM {

class HGraph;
class IRBuilder;
class FunctionHelper;
class IntrinsicHelper;

class LLVMCompilationUnit {
 public:
  explicit LLVMCompilationUnit(CodeGenerator* codegen,
                               HGraphFilePrettyPrinter* prt,
                               const std::vector<const DexFile*>* app_dex_files,
                               std::string main_hf_,
                               bool is_outer);
  bool VerifyModule();
  void PrettyPrintBitcode(std::string error_msg = "");
  std::string GetPrettyBitcodeErrorFile();
  void StoreBitcode(std::string postfix = "");

  ~LLVMCompilationUnit();

  LLVMContext* GetContext() { return context_.get(); }
  Module* GetModule() { return mod_; }
  IRBuilder* GetIRBuilder() const { return irb_.get(); }
  MDBuilder* GetMDBuilder() const { return mdb_.get(); }
  HGraphFilePrettyPrinter* GetHGraphPrettyPrinter() { return prt_; }
  FunctionHelper* GetFunctionHelper() const { return fh_.get(); }
  IntrinsicHelper* GetIntrinsicHelper() const { return ih_.get(); }
  TargetMachine* GenerateTargetMachine();

#ifdef UNUSED
  void RunOptimizationsAndStoreObjectFile();
#endif

  CodeGenerator* GetCodeGen() const { return codegen_; }

  InstructionSet GetInstructionSet() const {
    return codegen_->GetInstructionSet();
  }

  // whether this is the outer entrypoint, as we have both inner and outer CUs
  bool IsOuter() const { return is_outer_; }

  const DexFile* GetDexFile(std::string dexFile, std::string dexLoc);
  CompilerTls* GetTls();
  void InstructionSetToLLVMTarget(InstructionSet instruction_set,
                                  std::string* target_triple,
                                  std::string* target_cpu,
                                  std::string* target_attr);
  static std::string PrettyMethod(art::HGraph* graph);

 private:
  CodeGenerator* codegen_;
  HGraphFilePrettyPrinter* prt_;
  // Main hf of the compilation unit
  std::string main_hf_;
  bool is_outer_;
  std::unique_ptr<LLVMContext> context_;
  Module* mod_ = nullptr;
  std::unique_ptr<IRBuilder> irb_;
  std::unique_ptr<MDBuilder> mdb_;
  std::unique_ptr<FunctionHelper> fh_;
  std::unique_ptr<IntrinsicHelper> ih_;
  std::unique_ptr<TargetMachine> target_machine_;
  const bool using_api_for_opt = false;

  const std::vector<const DexFile*>* app_dex_files_;

  LLVMInfo* llvm_info_ = nullptr;
  pthread_key_t llvm_tls_key_;
};

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_COMPILATION_UNIT_H_
