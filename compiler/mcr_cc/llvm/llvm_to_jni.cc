/**
 * Calling JNI code from LLVM.
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

#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"
#include "llvm_utils.h"

#include "llvm_macros_irb.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

/**
 * @brief 
 *
 * INFO we are not calling JniMethodStart or JniMethodEnd but we call RT
 * for each jobject to register it for the JNI environment
 */
Value* FunctionHelper::LLVMtoJNI(
      HGraphToLLVM* HL, IRBuilder* irb, Value* lmethod,
      Value* receiver, bool is_static,
      std::vector<Value*> callee_args, DataType::Type ret_type, uint32_t didx,
      const char* shorty, uint32_t shorty_len, 
      ArtMethod* art_method, std::string pretty_method) {
  const bool fast_jni = art_method->IsFastNative();
  const bool critical_jni = art_method->IsCriticalNative();

  Value* this_object_or_class_object = nullptr;
  Value* lclass = nullptr;
  std::string info;
  D2LOG(INFO) << __func__ << ": " << pretty_method << ":" << shorty << ":"
    << (is_static ? "STATIC" : "");

  if (McrDebug::VerifyInvokeJni()) {
    std::string extra = (is_static ? "static:" : "");
    irb->AndroidLogPrint(INFO, "-> JNI:" + extra + pretty_method + " \n");
  }

  if (McrDebug::DebugInvokeJni()) {
    std::stringstream ss;
    ss <<__func__ << ": art_method";
    irb->AndroidLogPrintHex(WARNING, ss.str(), lmethod);
  }

  if (!is_static) {
    info = "object";
    receiver->setName("receiver");
    this_object_or_class_object = receiver;
  } else {
    info = "class";
    // CHECK_LLVM if we can add in entry basic block
    // BasicBlock* prev_block = irb->GetInsertBlock();
    // irb->SetInsertPoint(HL->GetCurrentMethodEntryBlock());
    lclass = HL->GetArtMethodClass(lmethod);
    lclass->setName("class");
    this_object_or_class_object = lclass;
    // irb->SetInsertPoint(prev_block);
  }

  // Count the number of Object* arguments
  // this for instance, class for static
  uint32_t handle_scope_size = 1; 
  // INFO receiver is extracted from callee_args
  // and sent separately (that's OK)
  for (Value* arg : callee_args) {
    if (arg->getType() == irb->getJObjectTy()) {
      ++handle_scope_size;
    }
  }

  Value* jni_env = HL->GetLoadedJNIEnv();

  if(fast_jni) OPTIMIZE_LLVM("FAST JNI: " + pretty_method);
  if(critical_jni) OPTIMIZE_LLVM("CRITICAL JNI: " + pretty_method);

  if(!fast_jni) {
    std::vector<Value*> args {lmethod};
    irb->CreateCall(__SetJniMethod(), args);
  }

  if (McrDebug::DebugInvokeJni() & McrDebug::DebugLlvmCode4()) {
    irb->AndroidLogPrintHex(WARNING, "LLVMtoJNI: JNIenv", jni_env);
  }

  // Get code_addr
  FunctionType* native_func_ty =
    GetFunctionType(shorty, shorty_len, is_static, true, irb);

  // Get callee code_addr
  Value* code_addr = HL->LoadFromObjectOffset(
      lmethod, 
      HL->GetArtMethodEntryPointFromJniOffset(),
      native_func_ty->getPointerTo());
  code_addr->setName("native_function");

  if (McrDebug::DebugInvokeJni()) {
    irb->AndroidLogPrintHex(INFO, "LLVMtoJNI: native_ptr", code_addr);
  }

  // Load actual parameters
  std::vector<Value*> native_args;

  // param1: JNIEnv*
  native_args.push_back(jni_env);

  // param2: jobject or jclass (for static calls)
  std::vector<Value*> jni_refs;
  Value* jobject = nullptr;

  {
    std::vector<Value*> args;
    args.push_back(jni_env);
#ifdef ART_MCR_ANDROID_10  // INFO @JNI_REF_REG_RT
    args.push_back(this_object_or_class_object);
    jobject = irb->CreateCall(__AddJniReference(), args);
#elif defined(ART_MCR_ANDROID_6)
    Value* mirror_obj = irb->CreateBitCast(this_object_or_class_object,
        irb->getMirrorObjectPointerTy());
    mirror_obj->setName("art_obj");
    args.push_back(mirror_obj);
    jobject = irb->CreateCall(__JNIEnvExt_AddLocalReference(), args);
#endif
    jobject->setName("j" + info);
  }

  native_args.push_back(jobject);
  jni_refs.push_back(jobject);

#ifdef ART_MCR_ANDROID_6
  if (McrDebug::DebugInvokeJni()) {
    irb->AndroidLogPrint(INFO, "LLVMtoJNI: DEBUG:InvokeJniMethod: call\n");
    std::vector<Value*> args { lmethod, this_object_or_class_object};
    irb->CreateCall(__DebugInvokeJniMethod(), args);
    irb->AndroidLogPrint(INFO, "LLVMtoJNI: DEBUG:InvokeJniMethod: [RETURNED]\n");
  }
#endif

  // Store arguments, and in case of objects register them to JNI
  for (Value* arg : callee_args) {
    D3LOG(INFO) << "ARG: " << Pretty(arg->getType());
    if (arg->getType() == irb->getJObjectTy()->getPointerTo()) {
      Value* jobj_arg = nullptr;
      {
        std::vector<Value*> a;
        a.push_back(jni_env);
#ifdef ART_MCR_ANDROID_10 // INFO @JNI_REF_REG_RT
        a.push_back(arg);
        jobj_arg = irb->CreateCall(__AddJniReference(), a);
#elif defined(ART_MCR_ANDROID_6)
        Value* mirror_obj =
          irb->CreateBitCast(arg, irb->getMirrorObjectPointerTy());
        mirror_obj->setName("art_obj");
        a.push_back(mirror_obj);
        jobj_arg = irb->CreateCall(__JNIEnvExt_AddLocalReference(), a);
#endif
        jobj_arg->setName("jobj_arg");
      }

      native_args.push_back(jobj_arg);
      jni_refs.push_back(jobj_arg);
    } else {
      arg->setName("arg");
      native_args.push_back(arg);
    }
  }

  // Push quick frame
  StructType* managed_stack_type = irb->GetManagedStackTy();
  AllocaInst* managed_stack = irb->CreateAlloca(managed_stack_type);
  managed_stack->setName("ManagedStack");
  HL->ArtCallPushQuickFrame(managed_stack);
  if(McrDebug::DebugLlvmCode3()) {
    irb->AndroidLogPrint(WARNING, "===== LL2 LtJ: PushQuickFrame ========");
    HL->ArtCallVerifyStackFrameCurrent();
    irb->AndroidLogPrint(WARNING, "=================================");
  }

  if (McrDebug::DebugInvokeJni()) {
    irb->AndroidLogPrint(INFO, "-> JNI: " + pretty_method + " \n");
  }

  Value* retval = irb->CreateCall(code_addr, native_args);

  if (McrDebug::DebugInvokeJni()) {
    irb->AndroidLogPrint(INFO, "<- JNI: " + pretty_method + " \n");
  }

  // Pop quick frame
  HL->ArtCallPopQuickFrame(managed_stack);
  if(McrDebug::DebugLlvmCode3()) {
    irb->AndroidLogPrint(WARNING, "===== LL3 LtJ: PopQuickFrame ========");
    HL->ArtCallVerifyStackFrameCurrent();
    irb->AndroidLogPrint(WARNING, "=================================");
  }

  // Unregister JNI references
  for (Value* jni_obj : jni_refs) {
      std::vector<Value*> args { jni_env, jni_obj};
#ifdef ART_MCR_ANDROID_10
      irb->CreateCall(__RemoveJniReference(), args);
#elif defined(ART_MCR_ANDROID_6)
      irb->CreateCall(__JNIEnv_DeleteLocalRef(), args);
#endif
  }

  if(!fast_jni) {
    std::vector<Value*> args {irb->getJNull()};
    irb->CreateCall(__SetJniMethod(), args);
  }

  if (McrDebug::DebugInvokeJni()) {
    irb->AndroidLogPrint(INFO, "LLVMtoJNI: returning\n");
  }
  return retval;
}

}  // namespace LLVM
}  // namespace art
