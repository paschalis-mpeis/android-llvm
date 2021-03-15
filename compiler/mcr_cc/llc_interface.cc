/**
 * Interface for opt and llc LLVM tools.
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
#include "mcr_cc/llc_interface.h"

#include "base/os.h"
#include "base/time_utils.h"
#include "mcr_cc/clang_interface.h"  // used for linking (lld wraper)
#include "mcr_cc/linker_interface.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_cc/pass_manager.h"
#include "mcr_rt/mcr_rt.h"
#include "mcr_rt/utils.h"

namespace art {
namespace mcr {

const std::string LlcInterface::LLC = "llc ";
const std::string LlcInterface::OPT = "opt ";

bool LlcInterface::CleanupBeforeCompilation() {
  if (OS::FileExists(HFso)) {
    if (!EXE("rm -f " HFso)) return false;
  }
  if (OS::FileExists(HFlnkbc)) {
    if (!EXE("rm -f " HFlnkbc)) return false;
  }

  if (OS::FileExists(HFo)) {
    if (!EXE("rm -f " HFo)) return false;
  }

  return true;
}

bool LlcInterface::CleanupAfterCompilation() {
  if (!EXE("rm -f " HFo)) return false;
  return true;
}

bool LlcInterface::Compile(bool emit_llvm, bool emit_asm,
    std::string extraOptFlags) {
  std::string extraLlcFlags="";
  D3LOG(INFO) << __func__ << " "
    <<  (emit_llvm? "emit-llvm": "")
    <<  (emit_asm? "emit-asm": "");
  std::string cmd;
  std::string opt_flags(PassManager::GetCompilationFlagsOPT());
  std::string llc_flags(PassManager::GetCompilationFlagsLLC());
  std::string compiling_method = McrCC::GetLlvmEntrypoint();

  cdSrcDir();

#ifdef DEBUG5
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  DLOGD(INFO) << "CWD: " << cwd;
#endif

  CleanupBeforeCompilation();

  // Merge all flags w/ extra supplied flags
  const std::string baseline_optimization = mcr::PassManager::GetBaseline();
  // not valid options in opt/llc
  std::string debug_flags = ""; //mcr::PassManager::GetDebugFlags();

  std::string opt_baseline = baseline_optimization;
  std::string llc_baseline = baseline_optimization;
#ifdef ART_MCR_ANDROID_6
  // O0 level is invalid in opt tool
  if (baseline_optimization.compare("-O0") == 0) {
    opt_baseline = "";
  }
#endif

  opt_flags = spaced(opt_flags);
  bool keep_opt_output = emit_llvm;
  UNUSED(keep_opt_output);

  if (emit_asm) { DLOG(WARNING) << __func__ << ": Ignoring: emit_asm"; }

  bool longer_timeout = false;
  uint64_t total_time = 0;
  uint64_t s, t1;
  s = NanoTime();
  // llvm-link links bc files.
  int linkedMethods=LinkerInterface::LinkMethods(compiling_method);
  if (linkedMethods==0) return false;
  t1=NanoTime() - s;
  total_time+=t1;
  D1LOG(INFO) << "linked " << linkedMethods 
    << " methods in " << PrettyDuration(t1);

  if(linkedMethods>30 || t1/1e9 > 10) {
    D1LOG(WARNING) << "Enabling long timeout in compilations.";
    longer_timeout = true;
  }


  bool print_output = false;

#define ELIMINATE_DEAD_CODE
#ifdef ELIMINATE_DEAD_CODE
  s = NanoTime();
  cmd = OPT + spaced("-dce") + HFlnkbc + spaced("-o") + HFdcebc;
  cmd = McrCC::remove_extra_whitespaces(cmd);
  D3LOG(INFO) << "OPT-dce: " << cmd;
  if (!EXE(cmd, print_output, longer_timeout)) return false;
  if (!CHMOD(HFdcebc, "644")) return false;

  t1 = NanoTime()-s;
  total_time+=t1;
  D2LOG(INFO) << "DCE pass in " << PrettyDuration(t1);
#endif

  s = NanoTime();

  cmd = OPT + spaced(OPT_FLAGS) + spaced(extraOptFlags) + opt_flags + spaced(opt_baseline) +
        HFlnkbc + spaced("-o") + HFoptbc;
  cmd = McrCC::remove_extra_whitespaces(cmd);
#ifdef CRDEBUG5
  printf("%s\n", cmd.c_str());
  McrCC::LogLongMessage(cmd, INFO);
#endif
  if (!EXE(cmd, print_output, longer_timeout)) return false;
  t1 = NanoTime()-s;
  total_time+=t1;
  D2LOG(INFO) << "opt pass in " << PrettyDuration(t1);

  s = NanoTime();
  llc_flags = spaced(ARCH_FLAGS) + spaced(LLC_FLAGS) +
    spaced(extraLlcFlags) + spaced(llc_baseline) + spaced(debug_flags)
    + spaced(llc_flags);
  cmd = LLC + llc_flags + HFoptbc + spaced("-o") + HFo;

  cmd = McrCC::remove_extra_whitespaces(cmd);
#ifdef CRDEBUG5
  printf("%s\n", cmd.c_str());
  McrCC::LogLongMessage(cmd, INFO);
#endif

  if (!EXE(cmd, print_output, longer_timeout)) return false;
  if (!CHMOD(HFoptbc, "644")) return false;

  t1 = NanoTime()-s;
  total_time+=t1;
  D2LOG(INFO) << "llc pass in " << PrettyDuration(t1);

  if (!OS::FileExists(HFo)) {
    printf("ERROR: Failed to generate object file: %s\n", HFso);
    exit(1);
  }

  s = NanoTime();
  if (!ClangInterface::GenerateSharedObject()) return false;
  CleanupAfterCompilation();

  // Give permissions
  if (!CHMOD(HFso, "644")) return false;

  t1 = NanoTime()-s;
  total_time+=t1;
  DLOG(INFO) << "LLVM compilation finished in "
    << PrettyDuration(total_time);

  return true;
}

}  // namespace mcr
}  // namespace art
