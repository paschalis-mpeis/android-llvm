/**
 * Some helper macros. This is an ugly solution. they might have to be
 * undef'ed using llvm_macros_undef.h. They cause issues in -inl.h headers.
 *
 * They are specialised by llvm_macros_*.h headers.
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
#ifndef ART_COMPILER_LLVM_MACROS_H
#define ART_COMPILER_LLVM_MACROS_H 

#ifndef __IRB
#error Must define '__IRB' (with a wrapped header)
#endif

#define __func___ __func__ << ": "

#include <sstream>
#include "llvm_utils.h"
#include "debug.h"

/**
 * Always enabled options:
 * - CHECK_LLVM
 * - TODO_LLVM
 * - VERIFY_LLVM
 * - DIE_LLVM
 * - WARNING_LLVM
 *
 * Options enabled with ccdbg:
 *  - Options appended with D: e.g.,
 *      + CHECK_LLVMD, TODO_LLVMD, ..
 *      + OPTIMIZE_LLVM
 *      + VERIFY_LLVMD, VERIFY_LLVMD1, 2, 3, 4
 */

#define CHECK_NO_INTERMEDIATE_ACCESS(h) \
  CHECK(!(h)->GetArray()->IsIntermediateAddress()) \
  << "Disabled in LLVM";

#define AVOID_ASM LOGBOTH_nofunc(ERROR, "AVOID_ASM:: " <<  __func__)
#define XLLVM LOGBOTH_nofunc(ERROR, "XLLVM:: " <<  __func__)
#define XLLVM_(msg) LOGBOTH_nofunc(ERROR, "XLLVM:: " << msg <<  __func__)

#define DIE_ASM_REPLACED DLOG(ERROR) << __func__ << ": Replaced with LLVM"

#define TODO_LLVM(msg) LOGBOTH(ERROR, msg << " TODO_LLVM")
#define CHECK_LLVM(msg) LOGBOTH(ERROR, msg << " CHECK_LLVM")
#define VERIFY_LLVM_ LOGBOTH(WARNING, "VERIFY_LLVM")
#define INFO_ DLOG(INFO) << __func__
#define INFO4_ D4LOG(INFO) << __func__
#define INFO(msg) DLOG(INFO) << __func__ << msg
#define VERIFY_LLVM(msg) LOGBOTH(WARNING, msg << " VERIFY_LLVM")
#define VERIFIED_ if((false)) LOGBOTH(WARNING, "")
#define VERIFIED(msg) if((false)) LOGBOTH(WARNING, msg << " VERIFY_LLVM")
#define DIE_TODO DLOG(FATAL) << __func__ << ": "

#define DIE_UNIMPLEMENTED_ARCH(ISA) DIE_TODO \
  "Unimplemented for architecture: " << ISA; \
  UNREACHABLE() 

#define DIE_UNIMPLEMENTED(msg) DIE_TODO \
  << msg \
  << ": Unimplemented: " << prt_->GetInstruction(h) \
  << "\nMethod: " << GetPrettyMethod(); \
  UNREACHABLE() 

#define DIE_UNIMPLEMENTED_ DIE_TODO \
  << ": Unimplemented: " << prt_->GetInstruction(h) \
  << "\nMethod: " << GetPrettyMethod(); \
  UNREACHABLE()

#define DIE DLOG(FATAL) << __func__ << ": "
#define WARNING_LLVM(msg) LOGBOTH(WARNING, msg << " CHECK_LLVM")

// Debug LLVM macros:
#define DBG_LLVM() (McrDebug::DebugLLVM())
#define CHECK_LLVMD(msg) if(DBG_LLVM()) CHECK_LLVM(msg)
#define TODO_LLVMD(msg) if(DBG_LLVM()) TODO_LLVM(msg)
#define OPTIMIZE_LLVM(msg) if(DBG_LLVM()) \
  LOGBOTH(ERROR, "OPTIMIZE_LLVM: " << msg)
#define VERIFY_LLVMD(msg) if(DBG_LLVM()) VERIFY_LLVM(msg)
#define VERIFY_LLVMD_ if(DBG_LLVM()) VERIFY_LLVM_
#define INFO_LLVM if(DBG_LLVM()) LOGLLVM3(INFO, "")
#define WARNING_LLVMD(msg) if(DBG_LLVM()) WARNING_LLVM(msg)

#define ANDROID_LOG_HEX(LVL, HL, instruction, val) \
  if(DBG_LLVM()) { \
    std::stringstream ss; \
    ss << __func___ << HL->GetTwine(instruction) \
    << ": " << STRINGIFY(val); \
    __IRB->AndroidLogPrintHex(LVL, ss.str(), (val)); \
  }

#ifdef CRDEBUG1
#define VERIFY_LLVMD1(msg) if(DBG_LLVM()) VERIFY_LLVM(msg)
#define CHECK_LLVMD1(msg) if(DBG_LLVM()) CHECK_LLVM(msg)
#define WARNING_LLVMD1(msg) if(DBG_LLVM()) WARNING_LLVM(msg)
#define VERIFY_LLVMD1_ if(DBG_LLVM()) VERIFY_LLVM_
#define CHECK_LLVMD1_ if(DBG_LLVM()) CHECK_LLVM_
#define WARNING_LLVMD1_ if(DBG_LLVM()) WARNING_LLVM_
#else
#define VERIFY_LLVMD1(msg) if(false) VERIFY_LLVM(msg)
#define CHECK_LLVMD1(msg) if(false) CHECK_LLVM(msg)
#define WARNING_LLVMD1(msg) if(false) WARNING_LLVM(msg)
#define VERIFY_LLVMD1_ if(false) VERIFY_LLVM_
#define CHECK_LLVMD1_ if(false) CHECK_LLVM_
#define WARNING_LLVMD1_ if(false) WARNING_LLVM_
#endif

#ifdef CRDEBUG2
#define VERIFY_LLVMD2(msg) if(DBG_LLVM()) VERIFY_LLVM(msg)
#define CHECK_LLVMD2(msg) if(DBG_LLVM()) CHECK_LLVM(msg)
#define WARNING_LLVMD2(msg) if(DBG_LLVM()) WARNING_LLVM(msg)
#define VERIFY_LLVMD2_ if(DBG_LLVM()) VERIFY_LLVM_
#define CHECK_LLVMD2_ if(DBG_LLVM()) CHECK_LLVM_
#define WARNING_LLVMD2_ if(DBG_LLVM()) WARNING_LLVM_
#else
#define VERIFY_LLVMD2(msg) if(false) VERIFY_LLVM(msg)
#define CHECK_LLVMD2(msg) if(false) CHECK_LLVM(msg)
#define WARNING_LLVMD2(msg) if(false) WARNING_LLVM(msg)
#define VERIFY_LLVMD2_ if(false) VERIFY_LLVM_
#define CHECK_LLVMD2_ if(false) CHECK_LLVM_
#define WARNING_LLVMD2_ if(false) WARNING_LLVM_
#endif

#ifdef CRDEBUG3
#define VERIFY_LLVMD3(msg) if(DBG_LLVM()) VERIFY_LLVM(msg)
#define CHECK_LLVMD3(msg) if(DBG_LLVM()) CHECK_LLVM(msg)
#define WARNING_LLVMD3(msg) if(DBG_LLVM()) WARNING_LLVM(msg)
#define VERIFY_LLVMD3_ if(DBG_LLVM()) VERIFY_LLVM_
#define CHECK_LLVMD3_ if(DBG_LLVM()) CHECK_LLVM_
#define WARNING_LLVMD3_ if(DBG_LLVM()) WARNING_LLVM_
#else
#define VERIFY_LLVMD3(msg) if(false) VERIFY_LLVM(msg)
#define CHECK_LLVMD3(msg) if(false) CHECK_LLVM(msg)
#define WARNING_LLVMD3(msg) if(false) WARNING_LLVM(msg)
#define VERIFY_LLVMD3_ if(false) VERIFY_LLVM_
#define CHECK_LLVMD3_ if(false) CHECK_LLVM_
#define WARNING_LLVMD3_ if(false) WARNING_LLVM_
#endif

#ifdef CRDEBUG4
#define VERIFY_LLVMD4(msg) if(DBG_LLVM()) VERIFY_LLVM(msg)
#define CHECK_LLVMD4(msg) if(DBG_LLVM()) CHECK_LLVM(msg)
#define WARNING_LLVMD4(msg) if(DBG_LLVM()) WARNING_LLVM(msg)
#define VERIFY_LLVMD4_ if(DBG_LLVM()) VERIFY_LLVM_
#define CHECK_LLVMD4_ if(DBG_LLVM()) CHECK_LLVM_
#define WARNING_LLVMD4_ if(DBG_LLVM()) WARNING_LLVM_
#define TODO_LLVMD4(msg) if(DBG_LLVM()) TODO_LLVM(msg)
#define ANDROID_LOG_HEXD4(LVL, HL, instruction, val) \
  ANDROID_LOG_HEX(LVL, HL, instruction, val)
#else
#define VERIFY_LLVMD4(msg) if(false) VERIFY_LLVM(msg)
#define CHECK_LLVMD4(msg) if(false) CHECK_LLVM(msg)
#define WARNING_LLVMD4(msg) if(false) WARNING_LLVM(msg)
#define VERIFY_LLVMD4_ if(false) VERIFY_LLVM_
#define CHECK_LLVMD4_ if(false) CHECK_LLVM_
#define WARNING_LLVMD4_ if(false) WARNING_LLVM_
#define TODO_LLVMD4(msg) if(false) TODO_LLVM(msg)
#define ANDROID_LOG_HEXD4(LVL, HL, instruction, val) \
  if(false) { ANDROID_LOG_HEX(LVL, HL, instruction, val); }
#endif

#ifdef CRDEBUG5
#define VERIFY_LLVMD5(msg) if(DBG_LLVM()) VERIFY_LLVM(msg)
#define CHECK_LLVMD5(msg) if(DBG_LLVM()) CHECK_LLVM(msg)
#define WARNING_LLVMD5(msg) if(DBG_LLVM()) WARNING_LLVM(msg)
#define VERIFY_LLVMD5_ if(DBG_LLVM()) VERIFY_LLVM_
#define CHECK_LLVMD5_ if(DBG_LLVM()) CHECK_LLVM_
#define WARNING_LLVMD5_ if(DBG_LLVM()) WARNING_LLVM_
#else
#define VERIFY_LLVMD5(msg) if(false) VERIFY_LLVM(msg)
#define CHECK_LLVMD5(msg) if(false) CHECK_LLVM(msg)
#define WARNING_LLVMD5(msg) if(false) WARNING_LLVM(msg)
#define VERIFY_LLVMD5_ if(false) VERIFY_LLVM_
#define CHECK_LLVMD5_ if(false) CHECK_LLVM_
#define WARNING_LLVMD5_ if(false) WARNING_LLVM_

#endif

#define LOGLLVM(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  _LOGLLVM(__IRB, (lvl), ___ss.str()); \
  }

// will also print errors at the end of compilation
#define LOGforDex2Oat(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  LlvmCompiler::LogError(___ss.str()); \
  DLOG(lvl) << ___ss.str(); \
  }

#define LOGBOTH(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  _LOGLLVM(__IRB, (lvl), ___ss.str()); \
  D3LOG(lvl) << ___ss.str(); \
  }

#define LOGBOTH_nofunc(lvl, msg) { \
  std::stringstream ___ss; ___ss << msg; \
  _LOGLLVM(__IRB, (lvl), ___ss.str()); \
  D3LOG(lvl) << ___ss.str(); \
  }

#define LOGLLVM2(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  _LOGLLVM2(__IRB, (lvl), ___ss.str()); \
  }

#define LOGLLVMD(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  if(McrDebug::DebugLlvmCode()) { \
    _LOGLLVM(__IRB, (lvl), ___ss.str()); \
  } \
}

#define LOGLLVMD1(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  if(McrDebug::DebugLlvmCode1()) { \
    _LOGLLVM(__IRB, (lvl), ___ss.str()); \
  } \
}

#define LOGLLVMD2(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  if(McrDebug::DebugLlvmCode2()) { \
    _LOGLLVM(__IRB, (lvl), ___ss.str()); \
  } \
}

#define LOGLLVMD3(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  if(McrDebug::DebugLlvmCode3()) { \
    _LOGLLVM(__IRB, (lvl), ___ss.str()); \
  } \
}

#define LOGBOTH2(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << msg; \
  _LOGLLVM2(__IRB, (lvl), ___ss.str()); \
  D2LOG(lvl) << ___ss.str(); \
  }


#define LOGLLVM2val(lvl, msg, val) { \
  std::stringstream ___ss; \
  ___ss << __func__ << ": " << msg; \
  _LOGLLVM2val(__IRB, (lvl), (___ss.str()), (val)); \
  D2LOG(lvl) << ___ss.str(); \
}

#define LOGLLVMD2val(lvl, msg, val) { \
  std::stringstream ___ss; \
  ___ss << __func__ << ": " << msg; \
  if(McrDebug::DebugLlvmCode2()) { \
    _LOGLLVM2val(__IRB, (lvl), (___ss.str()), (val)); \
    D2LOG(lvl) << ___ss.str(); \
  } \
}

#define LOGLLVM3(lvl, msg) { \
  std::stringstream ___ss; ___ss << __func__ << ": " << (msg); \
  _LOGLLVM3(__IRB, (lvl), ___ss.str()); \
  D2LOG(lvl) << ___ss.str(); \
  }

#define LOGLLVM3val(lvl, msg, val) { \
  std::stringstream ___ss; \
  ___ss << __func__ << ": " << msg; \
  _LOGLLVM3val(__IRB, (lvl), (___ss.str()), (val)); \
  D3LOG(lvl) << ___ss.str(); \
}

#define LOGLLVM4val(lvl, msg, val) { \
  std::stringstream ___ss; \
  ___ss << __func__ << ": " << msg; \
  _LOGLLVM4val(__IRB, (lvl), (___ss.str()), (val)); \
  D4LOG(lvl) << ___ss.str(); \
}

#define LOGLLVM4(lvl, msg) _LOGLLVM4(__IRB, (lvl), (msg)); \
  D3LOG(lvl) << (msg)

#endif  // ifndef ART_COMPILER_LLVM_MACROS_H
