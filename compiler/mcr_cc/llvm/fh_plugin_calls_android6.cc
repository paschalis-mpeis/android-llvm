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
#include "function_helper.h"

using namespace ::llvm;
namespace art {
namespace LLVM {
#ifdef ART_MCR_ANDROID_6

Function* FunctionHelper::__ResolveClass() {
  Function* f = mod_->getFunction("_ZN3art4LLVM12ResolveClassEPvtb");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}

Function* FunctionHelper::__ClassInit() {
  Function* f = mod_->getFunction("_ZN3art4LLVM9ClassInitEPv");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}

Function* FunctionHelper::__ResolveInternalMethod() {
  return mod_->getFunction("_ZN3art4LLVM21ResolveInternalMethodEPvjj");
}

Function* FunctionHelper::__ResolveExternalMethod() {
  return mod_->getFunction("_ZN3art4LLVM21ResolveExternalMethodEPvPKcS3_jj");
}

Function* FunctionHelper::__ResolveVirtualMethod() {
  return mod_->getFunction("_ZN3art4LLVM20ResolveVirtualMethodEPvS1_");
}

Function* FunctionHelper::__ResolveInterfaceMethod() {
  return mod_->getFunction("_ZN3art4LLVM22ResolveInterfaceMethodEPvS1_j");
}

// object
Function* FunctionHelper::__jvalue_SetL() {
  Function* f = mod_->getFunction("_ZN3art4LLVM11jvalue_SetLEPNS_6JValueEPNS_6mirror6ObjectE");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}

Function* FunctionHelper::__JNIEnvExt_AddLocalReference() {
  Function* f =
      mod_->getFunction("_ZN3art9JNIEnvExt17AddLocalReferenceIP8_jobjectEET_PNS_6mirror6ObjectE");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}
Function* FunctionHelper::__JNIEnv_DeleteLocalRef() {
  Function* f = mod_->getFunction("_ZN3art9JNIEnvExt14DeleteLocalRefEP8_jobject");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}

Function* FunctionHelper::__InitializeStaticStorageFromCode() {
  Function* f =
      mod_->getFunction(
          "_ZN3art4LLVM31InitializeStaticStorageFromCodeEPvS1_jS1_");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}

Function* FunctionHelper::__InitializeTypeFromCode() {
  Function* f =
      mod_->getFunction("_ZN3art4LLVM22InitializeTypeFromCodeEPvS1_jS1_");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}

Function* FunctionHelper::__StringCompareTo() {
  Function* f = mod_->getFunction("_ZN3art4LLVM15StringCompareToEPvS1_");
  f->addFnAttr(Attribute::AlwaysInline);
  return f;
}

ART_METHOD_INL(__ResolveString, "_ZN3art4LLVM13ResolveStringEPvj");

ART_METHOD_INL(__InvokeWrapper, "_ZN3art4LLVM13InvokeWrapperEPvPjjPNS_6JValueE");

ART_METHOD_INL(__TestSuspend, "_ZN3art4LLVM11TestSuspendEv");

#endif
}  // namespace LLVM
}  // namespace art
