/**
 * Copyright (C) 2020 Paschalis Mpeis
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
 */

#include "mcr_cc/llvm/art_plugin.h"

/**
 *  All HELPER methods should be manually cleared out from art_module_full.ll,
 *  into ../art_module.ll
 */
namespace art {

void HELPER() {
  // jvalue getters
  JValue v;
  v.GetB();
  v.GetC();
  v.GetD();
  v.GetF();
  v.GetI();
  v.GetJ();
  v.GetL();
  v.GetS();
  v.GetZ();
  v.GetGCRoot();

  // jvalue setters
  v.SetB(0);
  v.SetC(0);
  v.SetD(0);
  v.SetF(0);
  v.SetI(0);
  v.SetJ(0);
  v.SetS(0);
  v.SetZ(0);

#if defined(ART_MCR_ANDROID_10)
  // JValue->SetL: done with entrypoint
  // LLVM::jvalue_SetL(nullptr, 0); // this has RT context issues
#elif defined(ART_MCR_ANDROID_6)
  v.SetL(0);  // issues @android10
#endif

  // object related
  StackReference<mirror::Object>* t1 = nullptr;
  t1->AsMirrorPtr();
  mirror::Object* t2 = nullptr;
  mirror::CompressedReference<mirror::Object>::FromMirrorPtr(t2);
  LLVM::ArrayPutObject(nullptr, 0, nullptr);

  // mcr wrappers
#ifdef ART_MCR_ANDROID_6
  LLVM::TestSuspend();
  LLVM::ClassInit(nullptr);
  LLVM::ResolveClass(nullptr, 0, true);
  LLVM::InitializeStaticStorageFromCode(nullptr, nullptr, 0, nullptr);
  LLVM::InitializeTypeFromCode(nullptr, nullptr, 0, nullptr);

  LLVM::ResolveInternalMethod(nullptr, 1, 0);
  LLVM::ResolveExternalMethod(nullptr, nullptr, nullptr, 1, 0);
  LLVM::ResolveVirtualMethod(nullptr, nullptr);
  LLVM::ResolveInterfaceMethod(nullptr, nullptr, 0);

  LLVM::VerifyArtMethod(nullptr, nullptr);
  LLVM::VerifyArtObject(nullptr, nullptr);
  LLVM::VerifyArtClass(nullptr, nullptr);

  // on android10 there are issues!
  env->AddLocalReference<jobject>(t2);
  env->DeleteLocalRef(nullptr);

  LLVM::InvokeWrapper(0, 0, 0, 0, 0);
#endif

  LLVM::ResolveString(nullptr, 0);
  LLVM::InvokeMethodSLOW(nullptr, nullptr, nullptr, 1, 3, nullptr, 5);
  LLVM::DebugInvoke(nullptr, nullptr, nullptr, 1, 3, nullptr, 5);
  LLVM::DebugInvokeJniMethod(nullptr, nullptr);
  LLVM::InvokeJniMethod(nullptr, nullptr, nullptr, 1, 3, nullptr, 5);
  LLVM::AndroidLog(0, nullptr, 1, 2, 3);
  LLVM::AllocObject(nullptr, 0);
  LLVM::AllocObjectWithAccessChecks(nullptr, 0);
  LLVM::AllocArray(nullptr, 0, 0);
  LLVM::AllocArrayWithAccessChecks(nullptr, 0, 0);
  LLVM::CheckCast(nullptr, nullptr);
  LLVM::InstanceOf(nullptr, nullptr);
#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
  LLVM::AddToInvokeHistogram_fromLLVM(nullptr, nullptr, 0, nullptr);
#endif

  LLVM::AddJniReference(nullptr, nullptr);
  LLVM::RemoveJniReference(nullptr, nullptr);

  LLVM::VerifyCurrentThreadMethod();
  LLVM::VerifyThread(nullptr);

  LLVM::VerifyString(nullptr);
  LLVM::VerifyBssObject(nullptr);

  JNIEnvExt* env = nullptr;
  env->GetSelf(); 

  LLVM::StringCompareTo(nullptr, nullptr); 

  LLVM::GetDeclaringClass(nullptr);

  LLVM::workaround();
  LLVM::workaroundII(0, 0);

  // workaround for LLVMtoJNI
  LLVM::SetJniMethod(nullptr);

  LLVM::EnableDebugLLVM();
}

}  // namespace art
