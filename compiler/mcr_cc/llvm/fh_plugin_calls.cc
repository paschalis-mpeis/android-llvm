/**
 * Code that imports the plugin-generated runtime calls
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
#include "function_helper.h"

#define ART_METHOD_INL(name, cppName) \
  Function* FunctionHelper::name() { \
   Function* f = \
        mod_->getFunction((cppName)); \
    f->addFnAttr(Attribute::AlwaysInline); \
    return f; \
  }

using namespace ::llvm;
namespace art {
namespace LLVM {

Function* FunctionHelper::AndroidLog() {
  return mod_->getFunction("_ZN3art4LLVM10AndroidLogEiPKcz");
}

ART_METHOD_INL(__ArrayPutObject, "_ZN3art4LLVM14ArrayPutObjectEPviS1_");
ART_METHOD_INL(__workaround, "_ZN3art4LLVM10workaroundEv");
ART_METHOD_INL(__workaroundII, "_ZN3art4LLVM12workaroundIIEii");

ART_METHOD_INL(__GetDeclaringClass, "_ZN3art4LLVM17GetDeclaringClassEPv");

ART_METHOD_INL(__VerifyThread, "_ZN3art4LLVM12VerifyThreadEPv");
ART_METHOD_INL(__VerifyArtMethod, "_ZN3art4LLVM15VerifyArtMethodEPvS1_");
ART_METHOD_INL(__VerifyArtObject, "_ZN3art4LLVM15VerifyArtObjectEPvS1_");
ART_METHOD_INL(__VerifyArtClass, "_ZN3art4LLVM14VerifyArtClassEPvS1_");

#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
ART_METHOD_INL(__AddToInvokeHistogram, "_ZN3art4LLVM29AddToInvokeHistogram_fromLLVMEPvS1_jS1_");
#endif

ART_METHOD_INL(__InvokeMethodSLOW, "_ZN3art4LLVM16InvokeMethodSLOWEPvS1_PNS_6JValueEz");
ART_METHOD_INL(__DebugInvoke, "_ZN3art4LLVM11DebugInvokeEPvS1_PNS_6JValueEz");
ART_METHOD_INL(__DebugInvokeJniMethod, "_ZN3art4LLVM20DebugInvokeJniMethodEPvS1_");
ART_METHOD_INL(__InvokeJniMethod, "_ZN3art4LLVM15InvokeJniMethodEPvS1_PNS_6JValueEz");

ART_METHOD_INL(__AllocObject, "_ZN3art4LLVM11AllocObjectEPvj");
ART_METHOD_INL(__AllocObjectWithAccessCheck, "_ZN3art4LLVM27AllocObjectWithAccessChecksEPvj");

ART_METHOD_INL(__AllocArray, "_ZN3art4LLVM10AllocArrayEPvjj");
ART_METHOD_INL(__AllocArrayWithAccessCheck, "_ZN3art4LLVM26AllocArrayWithAccessChecksEPvjj");

ART_METHOD_INL(__StringCompareTo, "_ZN3art4LLVM15StringCompareToEPvS1_");
// Not in use. if I inline it, I may use it..
ART_METHOD_INL(__mirrorStringCompareTo,
    "_ZN3art6mirror6String9CompareToENS_6ObjPtrIS1_EE");

ART_METHOD_INL(__CheckCast, "_ZN3art4LLVM9CheckCastEPvS1_");
ART_METHOD_INL(__InstanceOf, "_ZN3art4LLVM10InstanceOfEPvS1_");
ART_METHOD_INL(__object_AsMirrorPtr, "_ZNK3art6mirror15ObjectReferenceILb0ENS0_6ObjectEE11AsMirrorPtrEv");
ART_METHOD_INL(__object_FromMirrorPtr, "_ZN3art6mirror19CompressedReferenceINS0_6ObjectEE13FromMirrorPtrEPS2_");

// boolean
ART_METHOD_INL(__jvalue_SetZ, "_ZN3art6JValue4SetZEh");
ART_METHOD_INL(__jvalue_GetZ, "_ZNK3art6JValue4GetZEv");
// byte
ART_METHOD_INL(__jvalue_SetB, "_ZN3art6JValue4SetBEa");
ART_METHOD_INL(__jvalue_GetB, "_ZNK3art6JValue4GetBEv");
// char
ART_METHOD_INL(__jvalue_SetC, "_ZN3art6JValue4SetCEt");
ART_METHOD_INL(__jvalue_GetC, "_ZNK3art6JValue4GetCEv");
// short
ART_METHOD_INL(__jvalue_SetS, "_ZN3art6JValue4SetSEs");
ART_METHOD_INL(__jvalue_GetS, "_ZNK3art6JValue4GetSEv");
// int
ART_METHOD_INL(__jvalue_SetI, "_ZN3art6JValue4SetIEi");
ART_METHOD_INL(__jvalue_GetI, "_ZNK3art6JValue4GetIEv");

// long
#ifdef ART_MCR_ANDROID_10
ART_METHOD_INL(__jvalue_SetJ, "_ZN3art6JValue4SetJEl");
#else
ART_METHOD_INL(__jvalue_SetJ, "_ZN3art6JValue4SetJEx");
#endif
ART_METHOD_INL(__jvalue_GetJ, "_ZNK3art6JValue4GetJEv");

// float
ART_METHOD_INL(__jvalue_SetF, "_ZN3art6JValue4SetFEf");
ART_METHOD_INL(__jvalue_GetF, "_ZNK3art6JValue4GetFEv");

// double
ART_METHOD_INL(__jvalue_SetD, "_ZN3art6JValue4SetDEd");
ART_METHOD_INL(__jvalue_GetD, "_ZNK3art6JValue4GetDEv");

ART_METHOD_INL(__jvalue_GetL, "_ZNK3art6JValue4GetLEv");

#ifdef ART_MCR_ANDROID_10
ART_METHOD_INL(__AddJniReference, "_ZN3art4LLVM15AddJniReferenceEPNS_9JNIEnvExtEPv");
ART_METHOD_INL(__RemoveJniReference, "_ZN3art4LLVM18RemoveJniReferenceEPNS_9JNIEnvExtEP8_jobject");
#endif

ART_METHOD_INL(__SetJniMethod, "_ZN3art4LLVM12SetJniMethodEPv");

ART_METHOD_INL(__VerifyCurrentThreadMethod, "_ZN3art4LLVM25VerifyCurrentThreadMethodEv");
ART_METHOD_INL(__VerifyString, "_ZN3art4LLVM12VerifyStringEPv");
ART_METHOD_INL(__VerifyBssObject, "_ZN3art4LLVM15VerifyBssObjectEPv");

ART_METHOD_INL(__EnableDebugLLVM, "_ZN3art4LLVM15EnableDebugLLVMEv");

#undef ART_METHOD

}  // namespace LLVM
}  // namespace art
