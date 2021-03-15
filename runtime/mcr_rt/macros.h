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
#ifndef ART_RUNTIME_MCR_MACROS_H_
#define ART_RUNTIME_MCR_MACROS_H_

#include <ostream>

#undef DLOG

#ifdef ART_MCR
#undef LOG
#define LOG LOGART
// VV: verbose log: (stuff we want to ignore from RT)
#define LOGVV LOGART_VERBOSE
#else
#define LOGVV LOG
#endif

#undef D1LOG
#undef D2LOG
#undef D3LOG
#undef D4LOG
#undef D5LOG

// FROM system/core/base/include/android-base/logging.h
#define __SEVERITY_LAMBDA(severity) ([&]() {    \
  using ::android::base::VERBOSE;             \
  using ::android::base::DEBUG;               \
  using ::android::base::INFO;                \
  using ::android::base::WARNING;             \
  using ::android::base::ERROR;               \
  using ::android::base::FATAL_WITHOUT_ABORT; \
  using ::android::base::FATAL;               \
  return (severity); }())

#if defined(ART_MCR_RT)
#define TAG "MCRrt"
#define _LOG_METHOD McrLogMessage
#elif defined(ART_MCR_CC)
#define TAG "MCRcc"
#define _LOG_METHOD CCLogMessage
#elif defined(ART_MCR_D2O)
#define TAG "MCRd2o"
#define _LOG_METHOD CCLogMessage
#elif defined(ART_MCR)
#define TAG "MCR"
#define _LOG_METHOD McrLogMessage
#else
// ::android::base::DEFAULT
#define TAG "MCR"
#endif

#ifdef ART_MCR
#define DLOG(severity) __LOG(severity, TAG)
#else
#define DLOG(severity) __HOSTLOG(severity)
#endif

#define LOGART(severity) __LOG(severity, "art")

#define LOGART_VERBOSE(severity) __LOG(severity, "artvv")

#define __LOG(severity, tag) \
  ::art::mcr::_LOG_METHOD(__FILE__, __LINE__, ::android::base::DEFAULT, __SEVERITY_LAMBDA(severity), tag, -1, ::art::mcr::McrLog::NORMAL).stream()

// can be used only by CCLogMessage
#define __LOG_STDOUT(severity, tag) \
  ::art::mcr::_LOG_METHOD(__FILE__, __LINE__, ::android::base::DEFAULT, __SEVERITY_LAMBDA(severity), tag, -1, ::art::mcr::McrLog::NORMAL, true).stream()

// logging for STDOUT and Logcat (dex2oat and compiler)
#define TLOG(severity) __LOG_STDOUT(severity, TAG)

// for the LLVM backend generated code
#define LLVM_LOG(severity, fmt, va_args) \
  ::art::mcr::vlog(__FILE__, __LINE__, __func__, severity, "LLVM", fmt, va_args)

#define __HOSTLOG(severity) LOG(__SEVERITY_LAMBDA(severity))

#define HCHECK(x) \
  if (LIKELY((x))) \
    ::art::mcr::_LOG_METHOD(__FILE__, __LINE__, ::android::base::DEFAULT, ::android::base::FATAL, _LOG_TAG_INTERNAL, -1, ::art::mcr::McrLog::NORMAL).stream() \
        << "Check failed: " #x << " "

// RUNTIME TARGET ONLY
#if defined(ART_MCR_RT) && defined(ART_MCR_TARGET)

#define LLVM_DBG() (art::dbg_llvm_code_)

#define LLVM_ENTERED() __in_llvm=true
#define LLVM_EXITED()  __in_llvm=false

#define QUICK_ENTERED() __in_quick=true
#define QUICK_EXITED()  __in_quick=false

#define SET_LLVM_CALLED_QUICK() __llvm_called_quick=true
#define UNSET_LLVM_CALLED_QUICK() __llvm_called_quick=false

#define LLVM_CALLED_QUICK() (__llvm_called_quick)

#define LLVM_ENABLED() (mcr::McrRT::IsLlvmEnabled())
#define IN_LLVM() (__in_llvm)
#define IN_LLVM_DIRECTLY() (IN_LLVM() && !LLVM_CALLED_QUICK())
#define IN_QUICK() (__in_quick)
#define _IN_ICHF() (IN_LLVM() || IN_QUICK())

#ifdef CRDEBUG
#define IS_DBGRT() (UNLIKELY(__dbg_runtime_))
#define DBGRT_ON() __dbg_runtime_ = true
#define DBGRT_OFF()  __dbg_runtime_ = false

#define DBGRT(LVL) if (IS_DBGRT()) DLOG(LVL)

#define LOGLLVM(LVL)  if (IN_LLVM()) DLOG(LVL)
#define LOGLLVMCalledQuick(LVL)  if (IN_LLVM_DIRECTLY()) DLOG(LVL)
#define LOGLLVMinl(LVL)  if (__IsInLiveLLVM()) DLOG(LVL)
#define LOGICHFinl(LVL)  if (__IsInLiveAny()) DLOG(LVL)

#define LOGLLVM1(LVL) if (IN_LLVM()) D1LOG(LVL)
#define LOGLLVM2(LVL) if (IN_LLVM()) D2LOG(LVL)
#define LOGLLVM3(LVL) if (IN_LLVM()) D3LOG(LVL)
#define LOGLLVM4(LVL) if (IN_LLVM()) D4LOG(LVL)

#define LOGICHF(LVL)  if (_IN_ICHF()) DLOG(LVL)
#define LOGICHF1(LVL) if (_IN_ICHF()) D1LOG(LVL)
#define LOGICHF2(LVL) if (_IN_ICHF()) D2LOG(LVL)
#define LOGICHF3(LVL) if (_IN_ICHF()) D3LOG(LVL)
#define LOGICHF4(LVL) if (_IN_ICHF()) D4LOG(LVL)

#define DBGLLVM(LVL) if ((LLVM_ENABLED() || IS_DBGRT())) DLOG(LVL)

#else  // !CRDEBUG

#endif // ART_MCR_RT && CRDEBUG

#else  // host OS (!ART_MCR_RT)
// not optimized, but is for host os, so we don't care

#define LLVM_DBG() (false)
#define _IN_ICHF() ((false && true))
#define IN_LLVM_DIRECTLY() ((false))
#define LLVM_CALLED_QUICK() ((false))

#define IN_LLVM() ((false && true))
#define IN_QUICK() ((false && true))
#define LLVM_ENABLED() ((false && true))

#define LLVM_ENTERED()
#define LLVM_EXITED()
#define QUICK_ENTERED()
#define QUICK_EXITED()

#endif

#define LOGLLVMDRT(LVL)  if (LLVM_DBG()) DLOG(LVL)

// if LOGLLVM was not defined it means we are compiling for host
// (TODO: use host preprocessor definition, from ART to make this clear)
#ifndef LOGLLVM
#define DBGRT_ON()
#define DBGRT_OFF()

#define IS_DBGRT() (false)
#define DBGRT(LVL) if (false) DLOG(LVL)
#define DBGLLVM(LVL) if (false) DLOG(LVL)
#define LOGLLVM(LVL) if (false) DLOG(LVL)
#define LOGLLVMCalledQuick(LVL)  if (false) DLOG(LVL)

#define LOGLLVMinl(LVL)  if (false) DLOG(LVL)
#define LOGICHFinl(LVL)  if (false) DLOG(LVL)
#define LOGLIVE(LVL)  if (false) DLOG(LVL)
#define LOGLLVM1(LVL) LOGLLVM(LVL)
#define LOGLLVM2(LVL) LOGLLVM(LVL)
#define LOGLLVM3(LVL) LOGLLVM(LVL)
#define LOGLLVM4(LVL) LOGLLVM(LVL)

#define LOGICHF(LVL) LOGLLVM(LVL)
#define LOGICHF1(LVL) LOGLLVM(LVL)
#define LOGICHF2(LVL) LOGLLVM(LVL)
#define LOGICHF3(LVL) LOGLLVM(LVL)
#define LOGICHF4(LVL) LOGLLVM(LVL)

#endif

#if !defined(ART_MCR_TARGET) || !defined(CRDEBUG)
#define D1LOG(LVL) if (false) DLOG(LVL)

#endif

#ifdef ART_MCR
#ifdef CRDEBUG1
#define D1LOG(LVL) DLOG(LVL)
#define D1CHECK(x) CHECK(x)
#endif

#ifdef CRDEBUG2
#define LOGDBG(LVL) if (UNLIKELY(art::dbg_log_)) DLOG(LVL)
#define D2LOG(LVL) DLOG(LVL)
#define D2CHECK(x) CHECK(x)
#else
#define LOGDBG(LVL) if (false) DLOG(LVL)
#define D2LOG(LVL) if (false) DLOG(LVL)
#define D2CHECK(x) if (false) CHECK(x)
#endif

#ifdef CRDEBUG3
#define D3LOG(LVL) DLOG(LVL)
#define D3CHECK(x) CHECK(x)
#else
#define D3LOG(LVL) if (false) DLOG(LVL)
#define D3CHECK(x) if (false) CHECK(x)
#endif

#ifdef CRDEBUG4
#define D4CHECK(x) CHECK(x)
#define D4LOG(LVL) DLOG(LVL)
#else
#define D4CHECK(x) if (false) CHECK(x)
#define D4LOG(LVL) if (false) DLOG(LVL)
#endif

#ifdef CRDEBUG5
#define D5LOG(LVL) DLOG(LVL)
#else
#define D5LOG(LVL) if (false) DLOG(LVL)
#endif
#else  // this is for basically hosts (not ART_MCR)
#define D1LOG(LVL) if (false) DLOG(LVL)
#define D1CHECK(x) if (false) CHECK(x)
#define LOGDBG(LVL) if (false) DLOG(LVL)
#endif

#define EXIT_DEX2OAT_RECOMPILE() \
  printf("\nRECOMPILE\n");   \
  _exit(EXIT_FAILURE)

#ifdef ART_MCR_ANDROID_10
#define DIE_ANDROID10() DLOG(FATAL) << __func__
#define DIE_ANDROID10_ENTRYPOINT() DLOG(FATAL) << __func__ << ": TODO_LLVM: use quick entrypoint!"
#else
#define DIE_ANDROID10() if((false)) DLOG(FATAL)
#define DIE_ANDROID10_ENTRYPOINT()
#endif

#define LLVM_FRAME_FIXUP(THREAD) \
  if(IN_LLVM_DIRECTLY()) { \
    (THREAD)->SetTopOfStack(nullptr); \
  }

#endif  // ART_RUNTIME_MCR_MACROS_H_
