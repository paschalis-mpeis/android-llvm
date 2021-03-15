/**
 * Initializing LLVM (used by LLVMCU)
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

#include "llvm_compiler.h"
#include "mcr_rt/mcr_rt.h"

#include <sstream>
#include <llvm/LinkAllPasses.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/TargetSelect.h>

#include "debug.h"
#include "optimizing/nodes.h"
#include "llvm_utils.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

std::set<std::string> LlvmCompiler::errors_;
std::set<std::string> LlvmCompiler::warnings_;

bool LlvmCompiler::gen_invoke_histogram_ = false;

#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
bool LlvmCompiler::GenerateInvokeHistogram() {
  return gen_invoke_histogram_;
}
#endif

void LlvmCompiler::Initialize() {
  D4LOG(INFO) << "LlvmCompiler::Initialize:";

  // TimePassesIsEnabled = true;

  // we initialize all targets because on arm64 we are forced to use the arm 32 bit version
  // due to the fact that it is called by dex2oat which is a 32bit binary
  // Initialize LLVM target, MC subsystem, asm printer, and asm parser.
  if (kIsTargetBuild && false) {
    D3LOG(INFO) << "Initializing only NativeTarget";
    // Don't initialize all targets on device. Just initialize the device's native target
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
  } else {
    D3LOG(INFO) << __func__ << ": Initializing all targets!";
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();
  }

  // Initialize LLVM optimization passes
  PassRegistry& registry = *PassRegistry::getPassRegistry();

  initializeCore(registry);
  initializeScalarOpts(registry);
  initializeIPO(registry);
  initializeAnalysis(registry);
  // initializeIPA(registry);  // removed from llvm 3.8
  initializeTransformUtils(registry);
  initializeInstCombine(registry);
  initializeInstrumentation(registry);
  initializeTarget(registry);
}

void LlvmCompiler::LogError(std::string msg) {
  errors_.insert(msg);
}

void LlvmCompiler::LogWarning(std::string msg) {
  warnings_.insert(msg);
}

void LlvmCompiler::WriteCompErr(std::string err) {
  std::ofstream errFile;
  errFile.open (F_ERR_CC, std::ios_base::app);
  errFile << err;
  errFile.close();
}

bool LlvmCompiler::PrintReport() {
  // Debug code:
  // FindEntrypoint(1912); // kQuickLLVMVerifyArtClass
  // FindEntrypoint(1808); // kLLVMResolveTypeAndVerifyAccess

  if (errors_.size()>0 || warnings_.size()>0) {
    DLOG(WARNING) << "|-----   LLVM REPORT";
  }

  if(errors_.size() >0) {
    DLOG(ERROR) << "|- LLVM Errors: " << errors_.size();
    int i=0;
    for(std::string error: errors_) {
      i++;
      std::stringstream ss;
      ss << "| " << i << ": " << error;
      DLOG(ERROR) << ss.str();
      WriteCompErr(ss.str());
    }
  }

  if(warnings_.size() >0) {
    LOG(WARNING) << "|- LLVM Warnings: " << warnings_.size();
    int i=0;
    for(std::string warning: warnings_) {
      i++;
      std::stringstream ss;
      ss << "| " << i << ": " << warning;
      DLOG(WARNING) << ss.str();
      // Compilation warnigns are NOT written to a file
    }
  }
  return false;
}

}  // namespace LLVM
}  // namespace art
