/**
 * Implementations of this header are scattered over several hgraph_*.cc files
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
#ifndef ART_COMPILER_OPTIMIZING_HGRAPH_TO_LLVM_H_
#define ART_COMPILER_OPTIMIZING_HGRAPH_TO_LLVM_H_

#include <llvm/IR/Argument.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Metadata.h>
#include <map>
#include "debug.h"
#include "dex/dex_file-inl.h"  // needed by dex_compilation_unit
#include "driver/dex_compilation_unit.h"
#include "function_helper.h"
#include "hgraph_printers.h"
#include "intrinsic_helper.h"
#include "ir_builder.h"
#include "llvm_compilation_unit.h"
#include "llvm_compiler.h"
#include "llvm_info.h"
#include "mcr_cc/invoke_histogram.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_rt/invoke_info.h"
#include "mcr_rt/mcr_rt.h"
#include "optimizing/code_generator.h"
#include "optimizing/data_type.h"
#include "optimizing/nodes.h"

#define SUSPEND_CHECK_USE_METHOD

#define MAX_BRWEIGHT ((1U << 20)-1)
#define ENTRY_LLVM "entry_llvm"
#define ENTRY_HGRAPH "entry_hgraph"
#define INIT_INCLUDED "init_incl_"
#define INIT_DIRECT "init_"
#define EXTRA_INTRINSICS

using namespace ::llvm;
namespace art {

class ArtMethod;
class ScopedObjectAccess;

namespace LLVM {

class FunctionHelper;
class IRBuilder;
class IntrinsicHelper;
class LLVMCompilationUnit;

class HGraphToLLVM : public ::art::HGraphVisitor {
 public:
  explicit HGraphToLLVM(art::HGraph* graph,
                        const DexCompilationUnit& dcu,
                        LLVMCompilationUnit* llcu)
      : HGraphVisitor(graph),
        llcu_(llcu),
        dcu_(dcu),
        mod_(llcu->GetModule()),
        ctx_(llcu->GetContext()),
        irb_(llcu->GetIRBuilder()),
        mdb_(llcu->GetMDBuilder()),
        prt_(llcu->GetHGraphPrettyPrinter()),
        fh_(llcu->GetFunctionHelper()),
        ih_(llcu->GetIntrinsicHelper()),
        lblocks_(),
        phis_(),
        args_(),
        regs_(),
        proxy_fixup_(),
        external_fixup_() {
    irb_->setHgraphPrinter(prt_);
    fh_->setHgraphPrinter(prt_);
    ih_->setHgraphPrinter(prt_);
    histogram_complete_ = new InvokeHistogram(graph->GetDexFile().GetLocation().c_str());
  }

  explicit HGraphToLLVM(art::HGraph* graph,
                        const DexCompilationUnit& dcu,
                        LLVMCompilationUnit* llvm_cu_outer,
                        LLVMCompilationUnit* llvm_cu_inner)
      : HGraphToLLVM(graph, dcu,
                     llvm_cu_outer) {
    llvm_cu_inner_ = llvm_cu_inner;
  }

  ~HGraphToLLVM() {
    irb_->setHgraphPrinter(nullptr);
  }

  bool IsOuterMethod() {
    // outer method will have compilation unit of inner as not null
    return llvm_cu_inner_ != nullptr;
  }

  void ExpandIR();

  uint32_t GetMethodIdx() {
    return GetGraph()->GetMethodIdx();
  }

  std::string GetPrettyMethod() {
    return mcr::McrCC::PrettyMethod(GetGraph());
  }
  void DefineInnerMethod();
  void CommonInitialization();
  void CreateGlobalArtMethod();
  void CreateGlobalBootImageBegin();

  Value* LoadGlobalArtMethod();
  Value* LoadGlobalBootImageBegin();

  // INFO: it's not Location (of arm64 codegen registers for example)
  // Instead it is just an instruction, and we use the same name
  // so code will apear similar to optimizing backend
  ALWAYS_INLINE int64_t Int64FromLocation(HInstruction* h);

  // Load operations
  template <bool ptr, bool acquire=false> // Ldr (use X register)
    ALWAYS_INLINE Value* Load(Value* reg, uint32_t offset); // Ldr
   template <bool ptr, bool acquire=false> // Ldr (use X register)
    ALWAYS_INLINE Value* Load(Value* reg, Value* loffset,
        std::string desc="dynamic"); // Ldr
  template <bool ptr, bool acquire=false> // Ldr (use W register)
    ALWAYS_INLINE Value* LoadWord(Value* reg, uint32_t offset); // Ldrw
  template <bool ptr, bool acquire=false> // Ldr (use W register)
    ALWAYS_INLINE Value* LoadWord(
        Value* reg, Value* loffset, std::string desc="Dynamic"); // Ldrw
  template<bool ptr> // Ldrb
    ALWAYS_INLINE Value* LoadByte(Value* reg, uint32_t offset);
  template<bool ptr> // Ldrb
    ALWAYS_INLINE Value* LoadByte(Value* reg, Value* loffset,
        std::string desc="Dynamic");
  template<bool ptr>  // Ldrh
    ALWAYS_INLINE Value* LoadHalfWord(
        Value* reg, Value* loffset, std::string desc="Dynamic");
  template <bool ptr>  // Ldrh
    ALWAYS_INLINE Value* LoadHalfWord(Value* reg, uint32_t offset);

  Value* GenUnsafeGet(HInvoke* invoke, DataType::Type type, bool is_volatile);
  
  // Quick Entrypoints
  ALWAYS_INLINE Value* LoadThread();
  ALWAYS_INLINE Value* GetLoadedThread();
  ALWAYS_INLINE Value* GetLoadedJNIEnv();
  ALWAYS_INLINE Value* LoadJNIEnv();
  ALWAYS_INLINE void CacheThreadForCurrentMethod(Value* param_thread);
  ALWAYS_INLINE Value* GetQuickEntrypoint(QuickEntrypointEnum qpoint);
  ALWAYS_INLINE Value* GetQuickEntrypointASM(QuickEntrypointEnum qpoint);

  void ArtCallPushQuickFrame(Value* fragment);
  void ArtCallPopQuickFrame(Value* fragment);
  void ArtCallClearTopOfStack();

  void ArtCallVerifyArtMethod(Value* art_method);
  void ArtCallVerifyArtClass(Value* art_class);
  void ArtCallVerifyArtObject(Value* object);
  void ArtCallVerifyStackFrameCurrent();
  Value* ArtCallResolveType__(QuickEntrypointEnum qpoint,
      Value* caller, uint32_t type_idx, Value* llvm_bss_slot);
  Value* ArtCallInstanceOfNonTrivial(Value* lobj, Value* lclass);
  void ArtCallCheckInstanceOf(Value* lobj, Value* lclass);
  void ArtCallInitializeStaticStorage(Value* klass);

  // exceptions
  void ArtCallDeliverException(Value* lexception);

  Value* ArtCallAllocObject__(QuickEntrypointEnum qpoint, Value* klass);
  Value* ArtCallAllocArray__(
      QuickEntrypointEnum qpoint, Value* klass, Value* length);
  void ArtCallAputObject(Value* array, Value* index, Value* storeObj);
  Value* ArtCallGetObjInstance(uint32_t field_idx, Value* lobj, Value* lref);
  Value* ArtCallGetObjStatic(uint32_t field_idx, Value* lref);

  void ArtCallJValueSetL(Value* jvalue, Value* obj);

  Value* ArtCallReadBarrierSlow(
      Value* referrer, Value* object, Value* member_offset);
  Value* ArtCallResolveExternalMethod(
      Value* referrer, Value* dex_filename, Value* dex_location,
      Value* ldex_method_idx, Value* linvoke_type);
  Value* ArtCallResolveExternalMethod(
      Value* referrer, Value* dex_filename, Value* dex_location,
      uint32_t dex_method_idx, InvokeType invoke_type);

  Value* ArtCallResolveInternalMethod(
      Value* referrer, Value* ldex_method_idx, Value* linvoke_type);
  Value* ArtCallResolveInternalMethod(
      Value* referrer, uint32_t dex_method_idx, InvokeType invoke_type);

  Value* ArtCallResolveVirtualMethod(Value* receiver, Value* referrer); 

  Value* ResolveInterfaceMethod(HInvokeInterface *hinvoke, Value* receiver);
  Value* ArtCallResolveInterfaceMethod(Value* receiver, Value* referrer);
  Value* ArtCallResolveString(Value* caller, uint32_t string_idx,
      Value* llvm_bss_slot);

  void ArtCallMonitorOperation(Value* lobj, bool is_lock);
  void ArtCallTestSuspend();
  void ArtCallQuickTestSuspend();
  void ArtCallLLVMTestSuspend();
  void ArtCallInvokeQuick__(HInvoke* invoke, Value* art_method, Value* qargs,
      Value* qargs_size, Value* jvalue, Value* shorty,
      bool is_static, bool use_wrapper);
  
  GlobalVariable* GetGlobalBssSlot(std::string name);
  Value* LoadGlobalBssSlot(std::string name);

  GlobalVariable* GetGlobalArtMethod(std::string global_name, bool define);
  GlobalVariable* GetGlobalArtMethod(std::string pretty_method,
                                             HInvoke* invoke, bool define,
                                             std::string postfix = "");
  GlobalVariable* GetGlobalVariable(
      std::string global_name, DataType::Type type, bool define);
  GlobalVariable* GetGlobalVariable(
      std::string global_name, Type* type, bool define);
  void AddToInitializedInnerMethods(Function* func);
  bool InitializedInnerMethod(Function* func);
  void PopulateInnerMethod();
  void DefineInitInnerMethods();
  void InitializeInitInnerMethods(bool same_dex, MethodReference method_ref);
  void InitializeInitInnerMethodsCommon(
      Function* init_func,
      BasicBlock*& block);

  void AddToInitInnerMethods(std::string pretty_method, HInvoke* invoke,
                             uint32_t resolved_dex_method_idx,
                             bool is_hot, bool is_native, bool is_abstract,
                             std::string signature, std::string dex_filename,
                             std::string dex_location, bool resolve_ext);
  void FinalizeInitInnerMethods();
  void GenerateEntrypointInit();
  void GenerateEntrypointLLVM(bool is_live);
  void SetThreadRegister(Value* thread_self);

  void LinkBasicBlocks(BasicBlock* src,
                       BasicBlock* trgt);

  ArtMethod* ResolveSpeculativeMethod(mcr::InvokeInfo invoke_info);
  ArtMethod* ResolveLocalMethod(uint32_t dex_method_idx,
                                InvokeType optimized_invoke_type)
      SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
  ALWAYS_INLINE ArtMethod* ResolveMethod(const DexFile* dex_file,
                                         uint32_t dex_method_idx,
                                         InvokeType invoke_type);
  ALWAYS_INLINE mirror::Class* ResolveClass(const DexFile* dex_file,
                                            uint16_t dex_type_idx);

  static ALWAYS_INLINE bool IsShiftOperation(HInstruction* h);
  ALWAYS_INLINE Value* GetArtMethodClassSLOW(Value* art_method);
  ALWAYS_INLINE Value* GetArtMethodClass(Value* art_method);
  ALWAYS_INLINE bool FieldValueCanBeNull(HInstruction* h);

  // OPTIMIZE_LLVM this will significantly improve time.
  // HGraph version relies on link-time patching that we currently cannot do
  // Also they are in the same binary with the .bss object cache
  // (not the standard C runtime bss cache), so they can do a label resolution
  // to get the value they need. We generate a separate object file, so we
  // cannot do this unless we integrate into the OAT file.
  // 
  // Value* GenerateGcRootFieldLoad(
  //     HInstruction* instruction,
  //     Value* obj,
  //     uint32_t offset,
  //     Value* fixup_label, // mostly for compatibility
  //     ReadBarrierOption read_barrier_option);

  Value* GenerateReferenceLoad(
      IRBuilder* irb,
      HInstruction* instruction,
      Value* lobj,
      Value* loffset,
      ReadBarrierOption read_barrier_option);

  ALWAYS_INLINE Value* GetClassTypeIdx(Value* klass);
  ALWAYS_INLINE Value* GetObjectClassDirect(Value* obj);
  ALWAYS_INLINE Value* GenerateReferenceObjectClass(
      HInstruction *h, Value* lobj, ReadBarrierOption read_barrier_option);
  ALWAYS_INLINE Value* GetStringLength(Value* str_obj);

  Function* GetInnerFunction() const {
    return inner_func_;
  }

  Value* CallMethod(
      bool call_directly, bool is_hot, IRBuilder* irb,
      ArtMethod* art_method, ArtMethod* spec_art_method,
      HInvoke* invoke, std::string signature, const char* shorty, uint32_t shorty_len,
      bool is_native, bool is_static, DataType::Type ret_type,
      Value* receiver, std::vector<Value*> callee_args,
      bool init_inner, Value* rt_resolved_method, bool in_miss_block);

  /**
   * @brief We need to pass receiver separately because the RT InvokeMethod
   *        needs it seperated from the rest of the arguments
   *        
   *        Regardless of whether method is not, it will be called through RT.
   *        However sometimes we might dynamically resovle methods that are hot,
   *        and we are still calling them cold (throughRT).
   *        is_hot will make sure we get the global ArtMethod of our initialization 
   *        process, to be more consistent.
   *        
   *        INFO this migh not be necessary anymore after the generation of
   *        histogram w/ ReplayInstrumentation
   */
  Value* CallColdMethod(
      bool is_hot, IRBuilder* irb,
      ArtMethod* art_method, ArtMethod* spec_art_method,
      HInvoke* invoke, std::string signature, const char* shorty, uint32_t shorty_len,
      bool is_native, bool is_static, DataType::Type ret_type,
      Value* receiver, std::vector<Value*> args_method_call,
      bool init_inner, Value* rt_resolved_method = nullptr,
      bool in_miss_block = false) {
    return CallMethod(false, is_hot, irb, art_method, spec_art_method, invoke,
                      signature, shorty, shorty_len, is_native, is_static, ret_type,
                      receiver, args_method_call,
                      init_inner, rt_resolved_method, in_miss_block);
  }

  /**
   * @brief  We are passing receiver as null, because if there must be a
   *         one (virtual/direct) will be on the 1st element of args_method_call
   */
  Value* CallHotMethod(
      IRBuilder* irb, ArtMethod* art_method, ArtMethod* spec_art_method,
      HInvoke* invoke, std::string signature, const char* shorty, uint32_t shorty_len,
      bool is_native, bool is_static, DataType::Type ret_type,
      std::vector<Value*> args_method_call, bool init_inner) {
    return CallMethod(true, true, irb, art_method, spec_art_method, invoke,
                      signature, shorty, shorty_len, is_native, is_static, ret_type,
                      irb->getJNull(), args_method_call, init_inner, nullptr, false);
  }

  void addValue(HInstruction* h, Value* value);
  Value* getValue(HInstruction* h);
 
  ALWAYS_INLINE std::string GetMethodName(
      std::string method_name, bool is_static, std::string signature);
  ALWAYS_INLINE static std::string EscapeString(std::string pretty_method);
  ALWAYS_INLINE Value* GetPointerFromHandle(Value* ptr);
  ALWAYS_INLINE Value* GetHandleFromPointer(Value* ptr);

  ALWAYS_INLINE void CallClassInitializationCheck(Value* klass);
  ALWAYS_INLINE bool IsFieldStatic(HInstruction* h);

  bool SpeculativeInSameDexFile(mcr::InvokeInfo invoke_info);

  LLVMCompilationUnit* GetLlvmCompilationUnit() const {
    return llcu_;
  }

  IntrinsicHelper* GetIntrinsicHelper() const {
    return ih_;
  }

  FunctionHelper* GetFunctionHelper() const {
    return fh_;
  }

  ALWAYS_INLINE LoadInst* LoadFromObjectOffset(Value* object_addr,
      int64_t offset,
      Type* type);
  ALWAYS_INLINE Value* LoadFromAddress16(
      Value* addr, uint16_t offset, Type* type);
  ALWAYS_INLINE Value* LoadFromAddress(
      Value* addr, uint32_t offset, Type* type);
  ALWAYS_INLINE void StoreToObjectOffset(Value* addr, uint32_t offset,
                                         Value* new_value);

  BasicBlock* GetCurrentMethodEntryBlock() {
    return cur_method_entry_block_;
  }

  void SetCurrentMethodEntryBlock(BasicBlock* bb) {
    cur_method_entry_block_ = bb;
  }

  /**
   * @brief Restores from FunctionHelper method wrappers entry block
   * the (ICHF) compiled method.
   */
  void RestoreLlvmEntryBlock() {
    cur_method_entry_block_ = llvm_entry_block_;
  }

  const CompilerOptions& GetCompilerOptions() const {
    return llcu_->GetCodeGen()->GetCompilerOptions();
  }

  // Intrinsic simplifications
  void SimplifyReturnThis(HInvoke* invoke);
  bool SimplifyAllocationIntrinsic(HInvoke* invoke);

  ALWAYS_INLINE std::string GetTwine(HInstruction* h);
  ALWAYS_INLINE InstructionSet GetISA();

  // Runtime Offsets
  uint32_t GetBootImageOffset(HLoadClass* load_class);
  uint32_t GetBootImageOffset(HLoadString* load_string); 
  uint32_t GetThreadFlagsOffset();
  uint32_t GetThreadExceptionOffset();
  uint32_t GetThreadJNIEnvOffset();
  uint32_t GetArtMethodEntryPointFromJniOffset();
  uint32_t GetThreadTopShadowFrameOffset();
  uint32_t GetThreadTopOfManagedStackOffset();

  Value* MaybeGenerateReadBarrierSlow(
      HInstruction* instruction,
      Value* ref, Value* obj,
      Value* offset, Value* index, bool is_volatile);

  Value* GenerateReadBarrierSlow(
      HInstruction* instruction,
      Value* ref, Value* obj,
      Value* offset,
      Value* index);

  Value* GetDynamicOffset(HInstruction* index, size_t shift, uint32_t offset);
  Value* GetDynamicOffset(Value* lindex, size_t shift, uint32_t offset);
  Value* GetDynamicOffset(Value* index, size_t shift, Value* loffset);

  void MaybeGenerateMarkingRegisterCheck(int code);

  Value* CastForStorage(Value* to_store_val,
      DataType::Type type,
      Type* storeTy);

  ALWAYS_INLINE Value* InputAtOrZero(HInstruction* instr, int index);
  ALWAYS_INLINE bool InputAtIsZero(
      HInstruction* instr, int index);
  ALWAYS_INLINE HInstruction* GetArrayIndex(HInstruction* h);
  ALWAYS_INLINE void CallMarkGCCard(Value* obj, Value* val,
      DataType::Type type, bool value_can_be_null);

  void AnalyseSuspendChecks();
  void SuspendCheckSimplify();

  // void runOnModule(Module& M);
  uint32_t GetDexMethodIndex() { return dcu_.GetDexMethodIndex(); }

  // TODO optimize all gbl_art_method_ usages..
  Value* GetLoadedArtMethod(Function* callsite = nullptr);
  Value* _LoadForFieldGet(HInstruction *h, Value* lobj, Value* offset,
      bool is_volatile);

  MDBuilder* MDB() { return mdb_; }

  Value* GenIsInfinite(Value* input, bool is64bit);
  
 private:
  // Calling ART's Quick entrypoints
  ALWAYS_INLINE Value* artCall(QuickEntrypointEnum qpoint, Type* retTy);
  ALWAYS_INLINE Value* artCall(QuickEntrypointEnum qpoint, Type* retTy,
      std::vector<Type*> params, std::vector<Value*> args);

  LLVMCompilationUnit* llcu_;
  LLVMCompilationUnit* llvm_cu_inner_ = nullptr;  // used only in outer
  const DexCompilationUnit& dcu_;
  Module* mod_;
  LLVMContext* ctx_;
  IRBuilder* irb_;
  MDBuilder* mdb_;
  ::art::HGraphFilePrettyPrinter* prt_;
  FunctionHelper* fh_;
  IntrinsicHelper* ih_;
  Function* inner_func_ = nullptr;
  Function* init_inner_from_ichf_func_ = nullptr;
  Function* init_inner_from_init_func_ = nullptr;
  BasicBlock* init_inner_from_ichf_block_ = nullptr;
  BasicBlock* init_inner_from_init_block_ = nullptr;
  BasicBlock* llvm_entry_block_ = nullptr;
  BasicBlock* cur_method_entry_block_ = nullptr;

  Function* init_= nullptr;
  // currently visiting lblock
  BasicBlock* cur_lblock_ = nullptr;
  std::map<HBasicBlock*, BasicBlock*> lblocks_;
  std::map<HPhi*, PHINode*> phis_;
  std::map<uint32_t, Argument*> args_;
  std::map<HInstruction*, Value*> regs_;
  std::map<BasicBlock*, BasicBlock*> sc_preds_;

  // extra inputs to PHIs from SuspendCheck blocks
  // its: original BB, that has a SuspendCheck BB
  // e.g., block5 has SuspendCheck_block5 (which is only an LLVM block)
  std::map<BasicBlock*, BasicBlock*> phi_sc_additions_;

  std::map<std::string, GlobalVariable*> art_method_globals_;
  // INFO not in use anymore?
  std::map<std::string, GlobalVariable*> bss_cache_;

  std::set<std::string> initialized_art_method_globals_;
  std::set<Function*> initialized_inner_methods_;
  std::map<Function*, Value*> loaded_thread_;
  std::map<Function*, Value*> loaded_art_methods_;
  std::map<Function*, Value*> jnienv_;

  GlobalVariable* gbl_art_method_ = nullptr;
  GlobalVariable* gbl_inner_inited_ = nullptr;
  Value* art_method_ichf_ = nullptr;
  Value* art_method_init_ = nullptr;

  // Runtime caches
  GlobalVariable* gbl_boot_image_begin_ = nullptr;

  std::map<uint32_t, uint32_t> proxy_fixup_;
  std::set<std::string> external_fixup_;

  InvokeHistogram* histogram_complete_;

  void AddToInitializedArtMethods(std::string method);
  bool IsArtMethodInitialized(std::string method);

  void addBasicBlock(HBasicBlock* hbb, BasicBlock* lbb);
  void addArgument(uint32_t index, Argument* var);
  void addRegister(HInstruction* h, Value* value);
  void updateRegister(HInstruction* h, Value* value);
  void addPhi(HPhi* hphi, PHINode* lphi);
  void replaceRegisterTmpInstruction(
      HInstruction* tmph, HInstruction* h);

  BasicBlock* getBasicBlock(HBasicBlock* hblock);
  HBasicBlock* getHBasicBlock(BasicBlock* Lblock);
  Argument* getArgument(HParameterValue* h);
  Value* getRegister(HInstruction* h);
  PHINode* getPhi(HPhi* hphi);

  ALWAYS_INLINE std::string GetInnerMethodName(bool is_static, std::string signature);
  ALWAYS_INLINE std::string GetTwineParam(uint32_t index);
  ALWAYS_INLINE std::string GetTwineArg(uint32_t index);
  ALWAYS_INLINE std::string GetTwine(uint32_t index, std::string prefix);
  ALWAYS_INLINE void PrintBasicBlockDebug(BasicBlock* lblock,
      std::string nameOverride="", std::string extra="");
  ALWAYS_INLINE void PrintBasicBlockDebug(HBasicBlock* hblock,
      std::string nameOverride="", std::string extra="");
  ALWAYS_INLINE std::string GetBasicBlockName(HBasicBlock* hblock);
  ALWAYS_INLINE Value* LoadArgument(
      BasicBlock* lblock, const char shortyi,
      Value* args, uint32_t args_index);

  ALWAYS_INLINE void UnimplementedVolatile(HInstruction* h);

  ALWAYS_INLINE std::string GetInnerSignature();
  ALWAYS_INLINE std::string GetSignature(HInvoke* invoke);
  ALWAYS_INLINE std::string GetSignature(ArtMethod* method);

  ALWAYS_INLINE std::string GetCallingMethodName(std::string pretty_method,
                                                 HInvoke* invoke,
                                                 std::string pre_fix);

  ALWAYS_INLINE std::string GetCallingMethodName(std::string pretty_method,
                                                 std::string signature,
                                                 bool is_static,
                                                 std::string pre_fix);

  // Helper methods
  void UnpackArguments(BasicBlock* lblock,
                       Value* args,
                       bool is_static,
                       std::vector<Value*>& args_inner_method,
                       const char* shorty, uint32_t shorty_len);
  Value* UnpackArgument(BasicBlock* lblock,
                                Value* args,
                                uint32_t& args_index,
                                const char shortyi);
 
  bool MethodPrototypeMatches(HInvoke* invoke,
      const char* shorty,
                              bool is_static,
                              std::string unverified_callee_name);

  void GenerateBasicBlocksAndPhis();
  void GenerateInstructions() {
    // CHECK this was causing use-before-set issues on llvm registers
    // Are now all problems fixed?
    // VisitInsertionOrder();
    VisitReversePostOrder();
  }
  BasicBlock* GenerateBasicBlock(HBasicBlock* hblock);
  void LinkEntryBlock();
  void VisitBasicBlock(HBasicBlock* hblock) override;
  // Instructions
  void VisitInstruction(HInstruction* h) override;
  void VisitCurrentMethod(HCurrentMethod* h) override;

  void VisitParameterValue(HParameterValue* h) override;

  // Architectural specific ops (arm/arm64)
  void VisitDataProcWithShifterOp(HDataProcWithShifterOp*) override;
  void VisitMultiplyAccumulate(HMultiplyAccumulate*) override;
  void VisitMonitorOperation(HMonitorOperation* instruction) override;
  void VisitPackedSwitch(HPackedSwitch*) override;

  void VisitPhi(HPhi* h) override;
  void PopulatePhis();
  void PopulatePhi(HPhi* hphi);
  void VisitIf(HIf* h) override;
  void VisitSelect(HSelect* h) override;
  void VisitExit(HExit* h) override;
  void VisitGoto(HGoto* gota) override;
  void HandleGoto(HInstruction* got, HBasicBlock* successor);
  void GenerateGoto(HBasicBlock* htarget);
  void GenerateGotoForSuspendCheck(
    BasicBlock* lfrom, BasicBlock *lfromSC, HBasicBlock* htarget);

  void HandleGotoMethod(HInstruction* got, HBasicBlock* successor);
  void GenerateSuspendCheckMethod(HSuspendCheck* h,
      HBasicBlock* successor);

  void VisitTryBoundary(HTryBoundary* try_boundary) override;
  void VisitReturnVoid(HReturnVoid* h) override;
  void VisitReturn(HReturn* h) override;

  // Constant Ops
  void VisitFloatConstant(HFloatConstant* h) override;
  void VisitDoubleConstant(HDoubleConstant* h) override;
  void VisitLongConstant(HLongConstant* h) override;
  void VisitIntConstant(HIntConstant* h) override;
  void GenerateConstant(HConstant* h, Constant* constant);
  void VisitNullConstant(HNullConstant* h) override;

  // Unary Ops
  void VisitBooleanNot(HBooleanNot* h) override;
  void VisitNeg(HNeg* h) override;
  void VisitNot(HNot* h) override;

  // Binary Ops
  void HandleBinaryOp(HBinaryOperation* h);

  // instruction_simplifier.cc: Intrinsics that became binops
  void VisitAbs(HAbs* h) override;

#define FOR_EACH_BINOP_INSTRUCTION(M)                          \
  M(Max)                                                       \
  M(Min)                                                       \
  M(Shl)                                                       \
  M(Shr)                                                       \
  M(UShr)                                                      \
  M(Rem)                                                       \
  M(Xor)                                                       \
  M(Ror)                                                       \
  M(Add)                                                       \
  M(Sub)                                                       \
  M(Mul)                                                       \
  M(Div)                                                       \
  M(And)                                                       \
  M(Or)
#define DEFINE_BINOP_VISITORS(Name)                             \
  void Visit##Name(H##Name* h) override { HandleBinaryOp(h); }
FOR_EACH_BINOP_INSTRUCTION(DEFINE_BINOP_VISITORS)
#undef DEFINE_BINOP_VISITORS
#undef FOR_EACH_BINOP_INSTRUCTION

#define FOR_EACH_CONDITION_INSTRUCTION(M)                             \
  M(Equal)                                                            \
  M(NotEqual)                                                         \
  M(LessThan)                                                         \
  M(LessThanOrEqual)                                                  \
  M(GreaterThan)                                                      \
  M(GreaterThanOrEqual)                                               \
  M(Below)                                                            \
  M(BelowOrEqual)                                                     \
  M(Above)                                                            \
  M(AboveOrEqual)
#define DEFINE_CONDITION_VISITORS(Name)                               \
  void Visit##Name(H##Name* comp) override { HandleCondition(comp); }
FOR_EACH_CONDITION_INSTRUCTION(DEFINE_CONDITION_VISITORS)
#undef DEFINE_CONDITION_VISITORS
#undef FOR_EACH_CONDITION_INSTRUCTION

  void HandleCondition(HCondition* h);
  Value* GetShiftOperation(
      HBinaryOperation* h, Value* lhs, Value* rhs);

  // assembly binary ops
  void VisitBitwiseNegatedRight(HBitwiseNegatedRight* instr) override;

  void VisitCompare(HCompare* h) override;

  // SIMD operations
  Value* VecAddress(
      HVecMemoryOperation* instruction, size_t size, bool is_string_char_at);
  void VisitVecReplicateScalar(HVecReplicateScalar* hvec) override;
  void VisitVecLoad(HVecLoad* instruction) override;
  void VisitVecStore(HVecStore* instruction) override;
  // void VisitVecSetScalars(HVecSetScalars* h) override;

  // Vec binops
  void HandleVecBinaryOp(HVecBinaryOperation* h);
#define FOR_EACH_VEC_BINOP_INSTRUCTION(M)                          \
  M(Add)                                                           \
  M(Sub)                                                           \
  M(Mul)                                                           \
  M(Div)                                                           \
  M(And)                                                           \
  M(Or)                                                            \
  M(Xor)                                                           \
  M(Min)                                                           \
  M(Max)
#define DEFINE_VEC_BINOP_VISITORS(Name)                             \
  void VisitVec##Name(HVec##Name* h) override { HandleVecBinaryOp(h); }
FOR_EACH_VEC_BINOP_INSTRUCTION(DEFINE_VEC_BINOP_VISITORS)
#undef DEFINE_VEC_BINOP_VISITORS
#undef FOR_EACH_VEC_BINOP_INSTRUCTION

  // Vec unops
  void HandleVecUnaryOp(HVecUnaryOperation* h);
#define FOR_EACH_VEC_UNOP_INSTRUCTION(M)                           \
  M(Neg)                                                           \
  M(Cnv)                                                           \
  M(Abs)                                                           \
  M(Not)
#define DEFINE_VEC_UNOP_VISITORS(Name)                             \
  void VisitVec##Name(HVec##Name* h) override { HandleVecUnaryOp(h); }
FOR_EACH_VEC_UNOP_INSTRUCTION(DEFINE_VEC_UNOP_VISITORS)
#undef DEFINE_VEC_UNOP_VISITORS
#undef FOR_EACH_VEC_UNOP_INSTRUCTION

  // Expressions
  void VisitTypeConversion(HTypeConversion* h) override;

  // Object Ops
  void VisitLoadClass(HLoadClass* h) override;
  void VisitLoadString(HLoadString* h) override;
  void VisitInstanceFieldGet(HInstanceFieldGet* h) override;
  void VisitStaticFieldGet(HStaticFieldGet* h) override;
  void VisitConstructorFence(HConstructorFence* constructor_fence) override;
  Value* GenerateReferenceLoad(
      IRBuilder* irb, HInstruction* instruction,
      Value* lobj, uint32_t offset,
      ReadBarrierOption read_barrier_option); 
  void HandleFieldGet(HInstruction* h);
  void HandleFieldSet(HInstruction* h);

  void VisitInstanceFieldSet(HInstanceFieldSet* h) override;
  void VisitStaticFieldSet(HStaticFieldSet* h) override;

  // Object Ops: Arrays
  void VisitArrayLength(HArrayLength* h) override;
  void VisitArrayGet(HArrayGet* h) override;
  void VisitArraySet(HArraySet* h) override;

  // Checks
  void VisitClinitCheck(HClinitCheck* h) override;
  void VisitNullCheck(HNullCheck* h) override;
  void VisitBoundsCheck(HBoundsCheck* h) override;
  void VisitSuspendCheck(HSuspendCheck* h) override;
  void VisitDivZeroCheck(HDivZeroCheck* h) override;
  void GenerateSuspendCheck(HSuspendCheck* h ATTRIBUTE_UNUSED,
                            HBasicBlock* successor ATTRIBUTE_UNUSED);
  void VisitCheckCast(HCheckCast* h) override;
  void VisitInstanceOf(HInstanceOf* h) override;

  void VisitBoundType(HBoundType* h) override;

  // Exceptions
  void VisitThrow(HThrow* h) override;
  void VisitLoadException(HLoadException* instruction) override;
  void VisitClearException(HClearException* clear) override;

  // Invokes
  Value* GetArtMethodStaticOrDirect(HInvokeStaticOrDirect* invoke);
  void InitArtMethodLocally(std::string callee_name, HInvoke* invoke,
                            uint32_t resolved_dex_method_idx,
                            Value* art_method,
                            BasicBlock* block,
                            InvokeType invoke_type,
                            std::string dex_filename, std::string dex_location,
                            bool resolve_ext);

  void CallInitInner(
      Function* init_inner_func, std::string callee_name,
      HInvoke* invoke, Value* referrer_art_method,
      BasicBlock* block, uint32_t dex_method_index,
      InvokeType invoke_type);

  void HandleInvoke(HInvoke* invoke);

  void VisitParallelMove(HParallelMove* hpmove) override {
    UNUSED(hpmove);
  }

  void VisitInvokeStaticOrDirect(HInvokeStaticOrDirect* invoke) override {
    HandleInvoke(invoke);
  }

  void VisitInvokeVirtual(HInvokeVirtual* invoke) override {
    HandleInvoke(invoke);
  }

  void VisitInvokeInterface(HInvokeInterface* invoke) override {
    D2LOG(INFO) << "VisitInvokeInterface: " << prt_->GetInstruction(invoke);
    HandleInvoke(invoke);
  }

  void VisitInvokePolymorphic(HInvokePolymorphic* invoke) override {
  DLOG(FATAL) << __func__;
    // HandleInvoke(invoke);
  }

  void VisitInvokeCustom(HInvokeCustom* invoke) override {
    DLOG(FATAL) << __func__;
    // HandleInvoke(invoke);
  }

  void VisitInvokeUnresolved(HInvokeUnresolved* invoke) override {
    DLOG(FATAL) << __func__;
    // HandleInvoke(invoke);
  }

  // Object creation
  void VisitNewInstance(HNewInstance* instruction) override;
  void VisitNewArray(HNewArray* instruction) override;

  // Assembly
  void VisitMemoryBarrier(HMemoryBarrier* memory_barrier) override;
  void VisitDeoptimize(HDeoptimize* deoptimize) override;
};

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_HGRAPH_TO_LLVM_H_
