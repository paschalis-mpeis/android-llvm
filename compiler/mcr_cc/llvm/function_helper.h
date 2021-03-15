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
#ifndef ART_COMPILER_LLVM_FUNCTION_HELPER_H_
#define ART_COMPILER_LLVM_FUNCTION_HELPER_H_

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "base/macros.h"
#include "dex/dex_file-inl.h"  // needed by dex_compilation_unit
#include "optimizing/data_type.h"
#include "driver/dex_compilation_unit.h"
#include "hgraph_printers.h"
#include "llvm_info.h"
#include "optimizing/code_generator.h"

using namespace ::llvm;

namespace art {
class ArtMethod;

namespace LLVM {

class IRBuilder;
class HGraphToLLVM;
class IntrinsicHelper;

class FunctionHelper {
 public:
  explicit FunctionHelper(LLVMContext& context, Module& module)
      : ctx_(&context), mod_(&module) {
  }
  void setHgraphPrinter(art::HGraphFilePrettyPrinter* prt) {
    prt_ = prt;
  }

  FunctionType* GetInnerFunctionType(
      const DexCompilationUnit& dex_cu, IRBuilder* irb);

  FunctionType* GetFunctionType(
      const char* shorty, uint32_t shorty_len,
      bool is_static, bool is_native, IRBuilder* irb);

  Function* Compare(IRBuilder* irb, DataType::Type type);
  Function* DivZeroCheck(IRBuilder* irb, DataType::Type type);
  Function* DivZeroFailedStatic(IRBuilder* irb);

  Function* NullCheck(IRBuilder* irb, bool make_implicit);
  Function* BoundsCheck(IRBuilder* irb, HGraphToLLVM* HL);
  bool IsSuspendCheck(Function* F);
  Function* SuspendCheckASM(HGraphToLLVM* HL, IRBuilder* irb,
      InstructionSet isa, HBasicBlock* successor);
  Function* MarkGCCard(
      IRBuilder* irb, DataType::Type type, bool value_can_be_null);
  Function* SuspendCheckGlobalVariable(IRBuilder* irb);
    
  Function* InvokeWrapper(
      ArtMethod* art_method, bool is_native,
      HGraphToLLVM* hgraph_to_llvm, IRBuilder* irb, IntrinsicHelper *IH,
      HInvoke* invoke,
      std::string signature, const char* shorty,
      uint32_t shorty_len, DataType::Type ret_type)
    SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

  Value* InvokeThroughRT_SLOW(HGraphToLLVM* hgraph_to_llvm,
                                      IRBuilder* irb, Value* art_method,
                                      Value* receiver,
                                      bool is_native,
                                      std::vector<Value*> callee_args,
                                      DataType::Type ret_type,
                                      std::string spec_method_name, std::string call_info);
  void DebugInvoke(HGraphToLLVM* HL, IRBuilder* irb, Value* art_method,
                   Value* receiver, bool is_native,
                   std::vector<Value*> callee_args, DataType::Type ret_type);

  Value* LLVMtoJNI(
      HGraphToLLVM* HL, IRBuilder* irb, Value* lmethod,
      Value* receiver, bool is_static,
      std::vector<Value*> callee_args, DataType::Type ret_type, uint32_t didx,
      const char* shorty, uint32_t shorty_len,
      ArtMethod* art_method, std::string pretty_method);

  Value* LLVMtoQuick(
      HGraphToLLVM* HL, IRBuilder* irb, HInvoke* hinvoke, Value* art_method,
      Value* receiver, bool is_static, std::vector<Value*> callee_args,
      DataType::Type ret_type, uint32_t didx, const char* shorty,
      uint32_t shorty_len, std::string pretty_method, std::string call_info);


  Function* LoadClass(HGraphToLLVM* HL, IRBuilder* irb,
      HLoadClass* cls, uint32_t caller_didx, bool use_cache);
  Function* LoadString(HGraphToLLVM* HL, IRBuilder* irb,
      HLoadString* h , uint32_t string_idx, bool use_cache);

  Function* ArrayGetMaybeCompressedChar(
      HGraphToLLVM* HL, IRBuilder* IRB, HArrayGet* h);

  Function* ArraySetWriteBarrier(
      HGraphToLLVM* HL, IRBuilder* IRB, HArraySet* h, uint32_t offset);

  Function* InstanceOf(
      IRBuilder* irb, HInstanceOf* instruction, HGraphToLLVM* HL);

  Function* CheckCast(
      IRBuilder* irb, HCheckCast* h, HGraphToLLVM* hgraph_to_llvm);

  Function* GenerateClassInitializationCheck(HGraphToLLVM* HL, IRBuilder* irb);

  // void FieldLoadWithBakerReadBarrier(
  //     HGraphToLLVM* HL, HInstruction* instruction, Value* lobj,
  //     uint32_t offset, bool needs_null_check, bool use_load_acquire);
  Function* FieldLoadWithBakerReadBarrier(
      HGraphToLLVM* HL, IRBuilder* IRB, HInstruction* instruction,
      bool needs_null_check, bool use_load_acquire);

  Value* GenerateFieldLoadWithBakerReadBarrier(
      HGraphToLLVM* HL, IRBuilder* IRB, HInstruction* instruction,
      Value* lobj, uint32_t offset,
    bool needs_null_check, bool use_load_acquire);
  Value* GenerateFieldLoadWithBakerReadBarrier(
      HGraphToLLVM* HL, IRBuilder* IRB, HInstruction* instruction,
      Value* lbase, Value* loffset,
      bool needs_null_check, bool use_load_acquire);

  Function* ArrayLoadWithBakerReadBarrier(
      HGraphToLLVM* HL, IRBuilder* IRB,
      HArrayGet* instruction, bool needs_null_check);

  Value* GenerateArrayLoadWithBakerReadBarrier(
      HGraphToLLVM* HL, IRBuilder* IRB, HArrayGet* instruction,
      Value* lobj, uint32_t data_offset, Value* lindex, bool needs_null_check);


  FunctionType* GetInitInnerFromIchfTy(IRBuilder* irb);
  FunctionType* GetInitInnerFromInitTy(IRBuilder* irb);

  Function* exit(IRBuilder *irb);
  void CallExit(IRBuilder* irb, int exit_code, bool emit_unreachable = true);
  Function* printf(IRBuilder *irb);
  Function* AndroidLog();


  void VerifyThread(HGraphToLLVM* HL, IRBuilder* irb);
  static void AddAttributesFastASM(Function* F);
  static void AddAttributesCommon(Function* F);
  static void AddAttributesCheckFunction(Function* F);
  static void AddAttributesSuspendCheck(Function* F);
  Function* LoadThread(HGraphToLLVM* HL, IRBuilder* irb);
  Function* LoadStateAndFlags(HGraphToLLVM* HL, IRBuilder* irb);
  Function* GetQuickEntrypoint(
      HGraphToLLVM* HL, IRBuilder* irb, QuickEntrypointEnum qpoint);

  Function* __workaround();
  Function* __workaroundII();
  // Runtime Plugin Methods
  // * ObjectMethods
  Function* __object_AsMirrorPtr();
  Function* __object_FromMirrorPtr();

  // * art::mcr methods: wrappers to RT methods
#ifdef ART_MCR_ANDROID_6
  Function* __TestSuspend();

  Function* __InvokeWrapper();
  Function* __ResolveClass();
  Function* __ClassInit();
  Function* __InitializeStaticStorageFromCode();
  Function* __InitializeTypeFromCode();

  Function* __ResolveInternalMethod();
  Function* __ResolveExternalMethod();
  Function* __ResolveVirtualMethod();
  Function* __ResolveInterfaceMethod();
  Function* __ResolveString();
#endif
  Function* __InvokeMethodSLOW();
  Function* __DebugInvoke();
  Function* __DebugInvokeJniMethod();
  Function* __InvokeJniMethod();
#ifdef ART_MCR_ANDROID_10
  Function* __AddJniReference();
  Function* __RemoveJniReference();
#elif defined(ART_MCR_ANDROID_6)
  Function* __JNIEnvExt_AddLocalReference();
  Function* __JNIEnv_DeleteLocalRef();
#endif
  Function* __SetJniMethod();
  Function* __AllocObject();
  Function* __VerifyCurrentThreadMethod();
  Function* __VerifyString();
  Function* __VerifyBssObject();
  Function* __EnableDebugLLVM();
  Function* __AllocObjectWithAccessCheck();
  Function* __AllocArray();
  Function* __AllocArrayWithAccessCheck();
  Function* __mirrorStringCompareTo();
  Function* __StringCompareTo();
  Function* __CheckCast();
  Function* __InstanceOf();
#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
  Function* __AddToInvokeHistogram();
#endif
  Function* __GetDeclaringClass();
  Function* __VerifyThread();
  Function* __VerifyArtMethod();
  Function* __VerifyArtObject();
  Function* __VerifyArtClass();
  Function* __ArrayPutObject();

  // * JValue methods
  Function* __jvalue_SetZ();
  Function* __jvalue_GetZ();
  Function* __jvalue_SetB();
  Function* __jvalue_GetB();
  Function* __jvalue_SetC();
  Function* __jvalue_GetC();
  Function* __jvalue_SetS();
  Function* __jvalue_GetS();
  Function* __jvalue_SetI();
  Function* __jvalue_GetI();
  Function* __jvalue_SetJ();
  Function* __jvalue_GetJ();
  Function* __jvalue_SetF();
  Function* __jvalue_GetF();
  Function* __jvalue_SetD();
  Function* __jvalue_GetD();
#ifdef ART_MCR_ANDROID_6
  Function* __jvalue_SetL();
#endif
  Function* __jvalue_GetL();

 private:
  LLVMContext* ctx_;
  Module* mod_;
  art::HGraphFilePrettyPrinter* prt_;

  // CHECK on multithreaded compiler we have to protect these
  // variabes and the functions that call them w/ mutexes
  Function* compare_int_ = nullptr;
  Function* compare_long_ = nullptr;
  Function* compare_double_ = nullptr;
  Function* compare_float_ = nullptr;

  Function* div_zero_check_int_ = nullptr;
  Function* div_zero_check_long_ = nullptr;
  Function* div_zero_failed_static_= nullptr;

  Function* null_check_ = nullptr;
  Function* bounds_check_ = nullptr;
  Function* func_suspend_check_=nullptr;
  Function* func_load_thread_ = nullptr;
  Function* func_load_state_and_flags_ = nullptr;

  std::map<QuickEntrypointEnum, Function*> art_entrypoints_;
  std::map<std::string, Function*> mark_gc_card_;
  std::map<std::string, Function*> check_cast_;
  std::map<std::string, Function*> baker_read_load_;
  std::map<std::string, Function*> instance_of_;
  std::map<std::string, Function*> invoke_virtuals_;
  std::map<std::string, Function*> load_class_;
  std::map<std::string, Function*> load_string_;
  std::map<std::string, Function*> array_get_;
  std::map<std::string, Function*> string_compare_to_;
  Function* class_init_check_ = nullptr;

  void VerifySpeculation(
      LogSeverity severity, IRBuilder* irb, uint32_t idx,
      std::string spec_msg, std::string pretty_method, bool die = false);
  void VerifySpeculation(
      LogSeverity severity, IRBuilder* irb, Value* idx,
      std::string spec_msg, std::string pretty_method, bool die = false);

  static std::string GetCompareName(DataType::Type type);
  static std::string GetDivZeroCheckName(DataType::Type type);
  FunctionType* GetCompareTy(IRBuilder* irb, DataType::Type type);
  FunctionType* GetDivZeroCheckTy(IRBuilder* irb, DataType::Type type);
  void StoreCompareFunction(Function* compare_function, DataType::Type type);
  void StoreDivZeroCheckFunction(Function* div_zero_check_function, DataType::Type type);
  Function* GetCompareFunction(DataType::Type type);
  Function* GetDivZeroCheckFunction(DataType::Type type);

};

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_FUNCTION_HELPER_H_
