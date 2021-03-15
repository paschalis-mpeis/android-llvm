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

#ifndef ART_COMPILER_OPTIMIZING_LLVM_DEBUG_H
#define ART_COMPILER_OPTIMIZING_LLVM_DEBUG_H

#include <string>

#include "mcr_rt/mcr_rt.h"

////////////////////
// Debug Options (for any app)
//////////////////

#define F_ERR_CC DIR_MCR "/err.comp"

#define F_DBG_LLVM_CODE DIR_MCR "/dbg.llvm_code"
#define F_DBG_INVOKE_JNI DIR_MCR "/dbg.invoke.jni"
#define F_DBG_INVOKE_QUICK DIR_MCR "/dbg.invoke.quick"

#define F_SKIP_SUSPEND_CHECK DIR_MCR "/suspend_check.dont_run"
#define F_VERIF_INVOKE DIR_MCR "/verif.invoke"
#define F_VERIF_INIT_INNER DIR_MCR "/verif.init_inner"
#define F_VERIF_BASIC_BLOCK DIR_MCR "/verif.basic_block"
#define F_VERIF_INVOKE_JNI DIR_MCR "/verif.invoke.jni"
#define F_VERIF_INVOKE_QUICK DIR_MCR "/verif.invoke.quick"
#define F_VERIF_ART_METHOD DIR_MCR "/verif.art_method"
#define F_VERIF_ART_OBJ DIR_MCR "/verif.art_obj"
#define F_VERIF_ART_CLASS DIR_MCR "/verif.art_class"

#define F_VERIF_INVOKE_QUICK_LLVM_TO_QUICK DIR_MCR "/verif.invoke.quick.LlvmToQuick"
#define F_VERIF_INVOKE_QUICK_THROUGH_RT_SLOW DIR_MCR "/verif.invoke.quick.ThroughRTslow"
#define F_VERIF_LLVM_INVOKE_WRAPPER DIR_MCR "/verif.llvm.invoke_wrapper"
#define F_VERIF_INVOKE_LLVM DIR_MCR "/verif.invoke.llvm"
#define F_VERIF_LLVM_CALLED DIR_MCR "/verif.llvm_called"

#define F_VERIF_LOAD_CLASS DIR_MCR "/verif.load_class"
#define F_VERIF_SPECULATION DIR_MCR "/verif.speculation"
#define F_VERIF_SPECULATION_MISS DIR_MCR "/verif.speculation.miss"
#define F_DISABLE_SPECULATION DIR_MCR "/spec_devirt.disable"

#define F_INTEPRET_NONHOT DIR_MCR "/opt.interpret_nonhot"
#define F_EXP_PROF_BREAKDOWN DIR_MCR "/exp.profile.breakdown"

#define F_LLVM_RECOMPILE DIR_MCR "/llvm.recompile"

//////////////////
// PER APP OPTIONS
//////////////////
#define F_SUSPEND_CHECK_SIMPLIFY "/suspend_check.simplify"
#define F_OPT_QUICK_THROUGH_RT "opt.quick_through_rt"

#define PREFIX_EXP_PROF_BREAKDOWN "/profile.breakdown"

namespace art {

class McrDebug {
 public:
  static constexpr bool kFakeImplicitCheck = true;

  static bool IsEnabled(std::string option);

  static void PrintOptions();
  static void ReadOptions();
  static void ClearRecompilation();
  static void SetRecompilation();

  static bool ImplicitNullChecks();

  static void ReadDebugLlvmCode();
  static void ReadDebugInvokeJni();
  static void ReadDebugInvokeQuick();

  static void ReadVerifyInitInner();
  static void ReadVerifyBasicBlock();

  static void ReadQuickThroughRT();
  static void ReadSuspendCheckSimplify();
  static void ReadSkipSuspendCheck();
  
  static void ReadVerifyLLvmInvokeWrapper();
  static void ReadVerifyArtMethod();
  static void ReadVerifyArtObject();
  static void ReadVerifyArtClass();
  static void ReadVerifyInvoke();
  static void ReadVerifyInvokeQuick();
  static void ReadVerifyInvokeQuickLlvmToQuick();
  static void ReadVerifyInvokeQuickThroughRT_SLOW();
  static void ReadVerifyInvokeLlvm();
  static void ReadVerifyLlvmCalled();
  static void ReadVerifyInvokeJni();
  static void ReadVerifyLoadClass();
  static void ReadVerifySpeculation();
  static void ReadVerifySpeculationMiss();
  static void ReadSpeculativeDevirt();
  static void ReadInterpretNonhot();

  static bool QuickThroughRT();
  static bool SuspendCheckSimplify();
  static bool SkipSuspendCheck();
  static bool SpeculativeDevirt();
  static bool InterpretNonhot();
  static bool DebugInvokeQuick();
  static bool DebugInvokeJni();

  static bool VerifyInitInner();
  static bool VerifyBasicBlock(std::string pretty_method = "");
  static bool DebugLlvmCode() { return debug_llvm_code_; }


  static bool DebugLlvmCode2() {
#ifdef CRDEBUG2
    return debug_llvm_code_;
#else
    return false;
#endif
  }


  static bool DebugLlvmCode3() {
#ifdef CRDEBUG3
    return debug_llvm_code_;
#else
    return false;
#endif
  }

  static bool DebugLlvmCode4() {
#ifdef CRDEBUG4
    return debug_llvm_code_;
#else
    return false;
#endif
  }

  static bool DebugLLVM() { return DebugLlvmCode(); }
  static bool DieOnSpeculationMiss();
  static bool VerifyArtMethod();
  static bool VerifyArtClass();
  static bool VerifyArtObject();
  static bool VerifyInvoke();
  static bool VerifyInvokeJni();
  static bool VerifyInvokeQuick();
  static bool VerifyInvokeQuickLlvmToQuick();
  static bool VerifyInvokeQuickThourghRT_SLOW();
  static bool VerifyLlvmInvokeWrapper();
  static bool VerifyInvokeLlvm();
  static bool VerifyLlvmCalled();
  static bool VerifyLoadClass();
  static bool VerifySpeculation();
  static bool VerifySpeculationMiss();

 private:
  static bool debug_invoke_quick_;
  static bool debug_invoke_jni_;
  static bool debug_llvm_code_;

  static bool exp_profile_breakdown_;
  static bool opt_quick_through_rt_;
  static bool sc_simplify_;
  static bool skip_suspend_check_;
  

  static bool verify_invoke_;
  static bool verify_invoke_jni_;
  static bool verify_invoke_quick_;
  static bool verify_invoke_llvm_;
  static bool verify_llvm_called_;

  static bool verify_art_method_;
  static bool verify_art_obj_;
  static bool verify_art_class_;

  static bool verify_llvm_invoke_wrapper;  // for InvokeVirtualwrapper/helper methods (virtual calls)
  static bool verify_invoke_quick_ThroughRTslow_;
  static bool verify_invoke_quick_LlvmToQuick_;

  static bool verify_load_class_;
  static bool speculative_devirt_;
  static bool interpret_nonhot_;
  static bool verify_speculation_;
  static bool verify_speculation_miss_;
  static bool die_on_speculation_miss_;
  static bool verify_init_inner_;
  static bool verify_basic_block_;

  static bool AnyOptionEnabled();
};

}  // namespace art

// ENABLED OPTIONS
#define LLVM_TO_JNI
#define LLVM_TO_QUICK

// DISABLED OPTIONS
#ifdef __DISABLED_DEBUG__OPTIONS__
#define CALL_RT_ON_SPEC_MISS
#endif  // __DISABLED_DEBUG__OPTIONS__

#endif  // ART_COMPILER_OPTIMIZING_LLVM_DEBUG_H
