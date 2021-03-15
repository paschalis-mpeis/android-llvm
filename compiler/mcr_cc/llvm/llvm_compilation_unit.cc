/**
 * Controlling the LLVM compilation, providing pretty printers for
 * LLVM  bitcode (in case of errors), providing a thread local storage,
 * and setting some defaults like target architecture
 *
 * (C) 2021  Paschalis Mpeis (paschalis.mpeis-AT-gmail.com)
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
#include "llvm_compilation_unit.h"

#include <llvm/Bitcode/BitcodeWriter.h>  // WriteBitcodeToFile
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <fstream>
#include <iostream>
#include "base/os.h"
#include "base/unix_file/fd_file.h"
#include "compiler_tls.h"
#include "function_helper.h"
#include "ir_builder.h"
#include "llvm_compiler.h"
#include "mcr_cc/linker_interface.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_rt/mcr_rt.h"
#include "mcr_rt/utils.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

Module* makeLLVMModuleContents(Module* module);

LLVMCompilationUnit::LLVMCompilationUnit(
    CodeGenerator* codegen,
    art::HGraphFilePrettyPrinter* prt,
    const std::vector<const DexFile*>* app_dex_files,
    std::string main_hf, bool is_outer)
    : codegen_(codegen),
      prt_(prt),
      main_hf_(main_hf),
      is_outer_(is_outer),
      app_dex_files_(app_dex_files) {

  D2LOG(INFO) << __func__ << ": "
    << (is_outer?"outer":"inner") << ": " << main_hf;

  CHECK_PTHREAD_CALL(pthread_key_create,
      (&llvm_tls_key_, nullptr), "LLVM: tls key: create");

  if (llvm_info_ == nullptr) {
    CompilerTls* tls = GetTls();
    llvm_info_ = static_cast<LLVMInfo*>(tls->GetLLVMInfo());
    if (llvm_info_ == nullptr) {
      llvm_info_ = new LLVMInfo();
      tls->SetLLVMInfo(llvm_info_);
    }
  }

  context_.reset(llvm_info_->GetLLVMContext());

  mod_=new Module(
      (is_outer_ ? "outer" : "inner"), *GetContext());
  fh_.reset(new FunctionHelper(*GetContext(), *GetModule()));
  irb_.reset(new IRBuilder(*GetContext(), *GetModule(),
        *GetFunctionHelper(), GetInstructionSet()));
  mdb_.reset(new ::llvm::MDBuilder(*GetContext()));

  ih_.reset(new IntrinsicHelper(*GetContext(), *GetModule(),
                                GetIRBuilder(), GetFunctionHelper()));
  LlvmCompiler::Initialize();
  target_machine_.reset(GenerateTargetMachine());

  D3LOG(INFO) << "Target Triple: " << target_machine_->getTargetTriple().str();
  mod_->setTargetTriple(target_machine_->getTargetTriple().str());

  UNUSED(using_api_for_opt);

  // Initialize module with plugin code (code that belongs to runtime)
  D5LOG(INFO) << "makeLLVMModuleContents";
  makeLLVMModuleContents(mod_);
  irb_->LoadFromArtModule();
}

bool LLVMCompilationUnit::VerifyModule() {
  std::string module=(is_outer_ ? "Outer" : "Inner");
 D5LOG(INFO) << __func__ << module;
  std::string s;
  raw_string_ostream OS(s);
  std::stringstream ss;

  if (verifyModule(*mod_, &OS)) {
    ss << module << " Module Verification Error:" << std::endl;
    ss << module << "Method:" << main_hf_ << std::endl;
    ss << OS.str();
    PrettyPrintBitcode(OS.str());
    LlvmCompiler::LogError(ss.str());
    return false;
  }
  return true;
}

LLVMCompilationUnit::~LLVMCompilationUnit() {
  D5LOG(INFO) << __func__ << ": " << (is_outer_?"outer":"inner");

  fh_.release();
  ih_.release();
  mdb_.release();
  irb_.release();
  context_.release();

  delete llvm_info_;

  // Delete pthread key
  CHECK_PTHREAD_CALL(pthread_key_delete,
      (llvm_tls_key_), "LLVM: tls key: delete");
}

TargetMachine* LLVMCompilationUnit::GenerateTargetMachine() {
  D5LOG(INFO) << "GenerateTargetMachine";
  std::string target_triple;
  std::string target_cpu;
  std::string target_attr;

  InstructionSetToLLVMTarget(
      codegen_->GetInstructionSet(),
      &target_triple, &target_cpu,
      &target_attr);

  std::string errmsg;
  const Target* target =
      TargetRegistry::lookupTarget(target_triple, errmsg);
 
  CHECK(target != NULL) << "Error: " << errmsg;

  // INFO these not so necessary as we are using external opt and llc
  // tools to generate the final machine code.
  TargetOptions target_options;
  target_options.FloatABIType = FloatABI::Soft;
  // target_options.NoFramePointerElim = true; // CHECK llvm 3.8
  // target_options.UseSoftFloat = false; // CHECK llvm 3.8

  target_options.EnableFastISel = false;
  // CodeGenOpt::Aggressive
  TargetMachine* target_machine = target->createTargetMachine(
      target_triple, target_cpu, target_attr, target_options,
      Reloc::Static, CodeModel::Small,
      CodeGenOpt::Default);

  D3LOG(INFO) << "Target triple: " << target_triple;
  D3LOG(INFO) << "Target CPU: " << target_cpu;
  D3LOG(INFO) << "Target attr: " << target_attr;

  CHECK(target_machine != nullptr) << "Failed to create target machine";
  return target_machine;
}

void LLVMCompilationUnit::PrettyPrintBitcode(std::string error_msg) {
  std::string strOut;
  std::string tmp;
  raw_string_ostream OS(tmp);

  // print error msg as comment on top
  if (error_msg.size() > 0) {
    strOut = "; COMPILATION ERROR:\n";
    std::istringstream iss(error_msg);
    for (std::string line; std::getline(iss, line);) {
      strOut += ";" + line + "\n";
    }
  }

  strOut += "\n";

  // CHECK iterate over aliases?

  for (Module::global_iterator itg = mod_->global_begin();
       itg != mod_->global_end(); itg++) {
    GlobalValue* gv = &*itg;
    OS << *gv;
    strOut += OS.str() + "\n";
    tmp.clear();
  }

  for (Module::iterator itm = mod_->begin();
       itm != mod_->end(); itm++) {
    Function* f = &*itm;
    OS << *f;
    strOut += OS.str() + "\n";
    tmp.clear();
  }

  std::string pretty_bitcode_filename = GetPrettyBitcodeErrorFile();
  D2LOG(INFO) << "Writing pretty bitcode to file: "
              << pretty_bitcode_filename;
  std::ofstream out(pretty_bitcode_filename);
  out << strOut;
  out.close();
}

std::string LLVMCompilationUnit::GetPrettyBitcodeErrorFile() {
  std::string filename = std::string(FILE_PRETTY_LLVM_BITCODE_PREFIX) +
                         (is_outer_ ? "outer" : "inner") + FILE_PRETTY_BITCODE_EXT;
  return mcr::GetFileSrc(main_hf_, filename);
}

void LLVMCompilationUnit::StoreBitcode(std::string postfix) {
  std::string bitcode_filename = mcr::GetFileSrc(main_hf_,
      mcr::McrCC::GetBitcodeFilename(is_outer_, postfix));
  std::string msg = std::string("Writing bitcode to: ") + bitcode_filename;
  DLOG(INFO) << msg;
  printf("%s\n", msg.c_str());
  std::error_code ec;
  std::unique_ptr<ToolOutputFile> out_file(
      new ToolOutputFile(bitcode_filename.c_str(), ec,
                                   sys::fs::F_None));
  WriteBitcodeToFile(*mod_, out_file->os());
  out_file->keep();

  if (!is_outer_) {
    mcr::LinkerInterface::StoreDependencies(main_hf_);
  }
}

const DexFile* LLVMCompilationUnit::GetDexFile(
    std::string dexFile, std::string dexLoc) {
  if (mcr::McrRT::IsFrameworkDexLocation(dexLoc)) {
    return mcr::McrRT::OpenDexFileOS(dexFile, dexLoc);
  } else {
    CHECK(app_dex_files_) << "GetDexFile: app_dex_files_ is null!";
    for (size_t i = 0; i != app_dex_files_->size(); ++i) {
      const DexFile* dex_file = app_dex_files_->at(i);
      CHECK(dex_file != nullptr);
      if (dex_file->GetLocation().compare(dexLoc) == 0) {
        return dex_file;
      }
    }
    DLOG(FATAL) << "GetDexFile: failed to get app's DexFile: " << dexLoc;
    UNREACHABLE();
  }
}

CompilerTls* LLVMCompilationUnit::GetTls() {
  // Lazily create thread-local storage
  CompilerTls* res =
      static_cast<CompilerTls*>(pthread_getspecific(llvm_tls_key_));
  if (res == nullptr) {
    res = CompilerTls::GenerateCompilerTls();
    CHECK_PTHREAD_CALL(pthread_setspecific, (llvm_tls_key_, res), "compiler tls");
  }
  return res;
}

inline std::string GetArm64TargetCPU() {
  std::string scpu = TARGET_CPU_VARIANT;
  if(scpu.length() == 0) {
    std::string sdevice= STRINGIFY(TARGET_DEVICE);
    if(sdevice.compare("sailfish") == 0) {
      return "kryo";
    } else if(sdevice.compare("flame") == 0) {
      return "kryo";
    }

    DLOG(FATAL) << "Can't find CPU for device: " << sdevice;
    UNREACHABLE();
  }

  return scpu;
}

void LLVMCompilationUnit::InstructionSetToLLVMTarget(
    InstructionSet instruction_set,
    std::string* target_triple,
    std::string* target_cpu,
    std::string* target_attr) {
  switch (instruction_set) {
    case InstructionSet::kThumb2:
      *target_triple = "thumb-none-linux-gnueabi";
      *target_cpu = "cortex-a9";
      *target_attr = "+thumb2,+neon,+neonfp,+vfp3,+db";
      break;
    case InstructionSet::kArm:
      *target_triple = "armv7-none-linux-gnueabi";
      *target_cpu = "cortex-a9";
      *target_attr = "+v7,+neon,+neonfp,+vfp3,+db";
      break;
    case InstructionSet::kArm64:
      *target_triple = "aarch64-linux-android";
      *target_cpu = GetArm64TargetCPU(); 
      // Regarding thread register (x18/x19):
      // llc_interface.h has a discusson on the Thread Register/xSELF
      // we are using x18 in LLVM as x19 (used by art) cannot be reserved
      // +reserve-x19: invalid option
      *target_attr = "+reserve-x20";
      break;
    case InstructionSet::kX86:
      *target_triple = "i386-pc-linux-gnu";
      *target_attr = "";
      break;
    case InstructionSet::kX86_64:
      *target_triple = "x86_64-pc-linux-gnu";
      *target_attr = "";
      break;
    case InstructionSet::kMips:
      *target_triple = "mipsel-unknown-linux";
      *target_attr = "mips32r2";
      break;
    default:
      DLOG(FATAL) << "Unknown instruction set: " << instruction_set;
  }
  D4LOG(INFO) << "InstructionSetToLLVMTarget: Triple:" << (*target_attr);
}

}  // namespace LLVM
}  // namespace art
