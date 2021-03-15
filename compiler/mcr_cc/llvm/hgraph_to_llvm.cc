/**
 * All starts with with ExpandIR. It generates entrypoints to LLVM,
 * and then visits the HGraph instructions to populate them in LLVM IR.
 *
 * InitInner methods are initialization entrypoints to LLVM.
 * These have instructions that initialize e.g. ArtMethods
 *
 * There are two entrypoints
 * live: live or normal /regular invocation of LLVM compiled code.
 * ichf: is used by Capture/Replay Iterative Compilation that is not
 *       part of this project/repository.
 *
 * BasicBlocks and Phi placeholders are initially created.Then
 * the instructions are populated, followed by the Phis.
 * 
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
#include "hgraph_to_llvm.h"
#include "hgraph_to_llvm-inl.h"

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/LegacyPassManager.h>
#include "art_method.h"
#include "asm_arm_thumb.h"
#include "asm_arm64.h"
#include "dex/dex_file_loader.h"
#include "llvm_utils.h"
#include "mcr_cc/analyser.h"
#include "mcr_cc/match.h"
#include "mcr_cc/os_comp.h"
#include "hgraph_printers.h"

#include "entrypoints/quick/quick_entrypoints_enum.h"

#include "llvm_macros_irb_.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

void HGraphToLLVM::ExpandIR() {
  D3LOG(INFO) << GetPrettyMethod()
              << "/" << std::to_string(GetGraph()->GetMethodIdx());
  D4LOG(INFO) << "dex_file: " << GetGraph()->GetDexFile().GetLocation();

  CommonInitialization();

  if (IsOuterMethod()) {
    GenerateEntrypointInit();
    GenerateEntrypointLLVM(true);
  } else {
    MethodReference method_ref(&GetGraph()->GetDexFile(), GetMethodIdx());
    bool same_dex = false;
    // multi-dex code sample:
    // const bool multi_dex = false; // see multi-dex notes
    // if (multi_dex) {
    //   MethodReference proxy_ref = mcr::Analyser::GetProxyRef(method_ref);
    //   method_ref = proxy_ref;
    //   same_dex = true;
    // }

    bool framework_method = mcr::McrRT::IsFrameworkDexLocation(
        GetGraph()->GetDexFile().GetLocation());
    D1LOG(INFO) << "ExpandIR: " << GetGraph()->GetMethodIdx()
      << ": framework_method:" << framework_method
      << ":" << GetPrettyMethod();

    if (framework_method) {
      same_dex = false;
    }

    // if it's multi-dex resolve locally using the proxy
    // (we cached it at runtime. resolving externally should now
    // work too)
    InitializeInitInnerMethods(!framework_method, method_ref);
    PopulateInnerMethod();
    FinalizeInitInnerMethods();
  }
}

void HGraphToLLVM::CommonInitialization() {
  INFO4_;
  CreateGlobalArtMethod();
  CreateGlobalBootImageBegin();
  DefineInitInnerMethods();
  DefineInnerMethod();  // in llvm context
}

void HGraphToLLVM::DefineInnerMethod() {
  D4LOG(INFO) << __func__;
  FunctionType* inner_func_type = fh_->GetInnerFunctionType(dcu_, irb_);

  bool is_static = dcu_.IsStatic();
  std::string name = GetInnerMethodName(is_static, GetInnerSignature());
  inner_func_ =
      Function::Create(
          inner_func_type,
          Function::ExternalLinkage,
          name,
          mod_);
  inner_func_->addFnAttr(Attribute::AlwaysInline);
  inner_func_->setDSOLocal(true);
}

void HGraphToLLVM::PopulateInnerMethod() {
  D2LOG(INFO) << __func__;
  uint32_t shorty_len;
  const char* shorty = dcu_.GetShorty(&shorty_len);
  bool is_static = dcu_.IsStatic();

  llvm_entry_block_ = BasicBlock::Create(
      *ctx_, ENTRY_LLVM, inner_func_);
  SetCurrentMethodEntryBlock(llvm_entry_block_);
  irb_->SetInsertPoint(GetCurrentMethodEntryBlock());

  if (McrDebug::VerifyBasicBlock(GetPrettyMethod())) {
    std::string info = " " + GetPrettyMethod();
    PrintBasicBlockDebug(llvm_entry_block_, "", info);
  }

  Function::arg_iterator arg_iter(inner_func_->arg_begin());
  uint32_t param_index = 0;
  if (!is_static) {
    Argument* receiver = &*arg_iter++;
    receiver->setName(GetTwineParam(param_index));
    addArgument(param_index, receiver);
    param_index++;
  }

  for (uint32_t i = 1; i < shorty_len; i++) {
    char shorty_jty = shorty[i];
    Argument* argument = &*arg_iter++;
    argument->setName(GetTwineParam(param_index));
    addArgument(param_index, argument);

    param_index++;
    if (DataType::Is64BitType(DataType::FromShorty(shorty_jty))) {
      param_index++;
    }
  }

  if (McrDebug::DebugLlvmCode4()) {
    if (McrDebug::VerifyArtMethod()) {
      irb_->AndroidLogPrint(INFO, "INNER: LLVM ICHF:");
      ArtCallVerifyArtMethod(GetLoadedArtMethod());
    }

    if (McrDebug::VerifyArtClass()) {
      irb_->AndroidLogPrint(INFO, "INNER: LLVM ICHF:Class:");
      ArtCallVerifyArtClass(GetArtMethodClass(GetLoadedArtMethod()));
    }
  }

  GenerateBasicBlocksAndPhis();
  GenerateInstructions();
  PopulatePhis();
  LinkEntryBlock();
  SuspendCheckSimplify();
}

void HGraphToLLVM::DefineInitInnerMethods() {
  bool is_static = dcu_.IsStatic();
  FunctionType* initIchfTy = fh_->GetInitInnerFromIchfTy(irb_);
  FunctionType* initInitTy = fh_->GetInitInnerFromInitTy(irb_);
  std::string signature = GetInnerSignature();

  D4LOG(INFO) << "creating init ichf";

  std::string nameInclude = GetCallingMethodName(
      GetPrettyMethod(), signature, is_static, INIT_INCLUDED);
  std::string nameDirect = GetCallingMethodName(
      GetPrettyMethod(), signature, is_static, INIT_DIRECT);

  init_inner_from_ichf_func_=Function::Create(
      initIchfTy, Function::ExternalLinkage, nameInclude, mod_);

  D4LOG(INFO) << "creating init init";
  init_inner_from_init_func_=Function::Create(
      initInitTy, Function::ExternalLinkage, nameDirect, mod_);

  AddToInitializedInnerMethods(init_inner_from_ichf_func_);
  AddToInitializedInnerMethods(init_inner_from_init_func_);
}

/**
 * @brief Creates the ichf_init and the include init main body.
 *
 * @param same_dex
 */
void HGraphToLLVM::InitializeInitInnerMethods(
    bool framework_method, MethodReference method_ref) {
  CHECK(init_inner_from_ichf_block_ == nullptr);
  CHECK(init_inner_from_init_block_ == nullptr);

  gbl_inner_inited_ =
    new GlobalVariable(*mod_, irb_->getCBoolTy(), false,
        GlobalVariable::PrivateLinkage,
        nullptr, "inited_" + inner_func_->getName().str());
  gbl_inner_inited_->setInitializer(irb_->getCBool(0));

  // Initialize method when entering from ichf
  Function::arg_iterator arg_iter = init_inner_from_ichf_func_->arg_begin();
  art_method_ichf_ = &*arg_iter++;
  art_method_ichf_->setName("art_method");

  InitializeInitInnerMethodsCommon(
      init_inner_from_ichf_func_, init_inner_from_ichf_block_);

  // simply store the art_method
  irb_->SetInsertPoint(init_inner_from_ichf_block_);
  irb_->CreateStore(art_method_ichf_, gbl_art_method_);
  ih_->LlvmInvariantStart(gbl_art_method_);

  // Initialize method when entering from init
  arg_iter = init_inner_from_init_func_->arg_begin();
  Value* referrer_art_method = &*arg_iter++;
  referrer_art_method->setName("referrer");

  Value* dex_method_idx = &*arg_iter++;
  dex_method_idx->setName("dex_method_idx");
  Value* orig_invoke_type = &*arg_iter++;
  orig_invoke_type->setName("orig_invoke_type");

  InitializeInitInnerMethodsCommon(
      init_inner_from_init_func_,
      init_inner_from_init_block_);
  irb_->SetInsertPoint(init_inner_from_init_block_);

  std::vector<Value*> args;
  std::string extra;
  Function* F = nullptr;
  if (!framework_method) {
    const DexFile* dex_file = method_ref.dex_file;
    std::string dex_loc = dex_file->GetLocation();
    Value* ldex_filename = irb_->mCreateGlobalStringPtr(
        DexFileLoader::GetBaseLocation(dex_loc));
    Value* ldex_location = irb_->mCreateGlobalStringPtr(dex_loc);
#if defined(ART_MCR_ANDROID_10)
    UNUSED(F);
    art_method_init_ = ArtCallResolveExternalMethod(referrer_art_method, 
        ldex_filename,
        ldex_location,
        dex_method_idx,
        orig_invoke_type);
#elif defined(ART_MCR_ANDROID_6)
    args.push_back(referrer_art_method);
    args.push_back(ldex_filename);
    args.push_back(ldex_location);
    args.push_back(dex_method_idx);
    args.push_back(orig_invoke_type);
    F = fh_->__ResolveExternalMethod();
#endif
    extra = "_ext";
    D3LOG(INFO) << "InitInner: ResolveExternalMethod: " << method_ref.PrettyMethod()
      << "\nDexFile: " << dex_loc << " [InitializeInitInnerMethods]";
  } else {
#if defined(ART_MCR_ANDROID_10)
    art_method_init_ = ArtCallResolveInternalMethod(
        referrer_art_method, dex_method_idx, orig_invoke_type);
      // Arm64::art_llvm_resolve_internal_method(
      //     irb_, referrer_art_method, dex_method_idx, orig_invoke_type);
#elif defined(ART_MCR_ANDROID_6)
    args.push_back(referrer_art_method);
    args.push_back(dex_method_idx);
    args.push_back(orig_invoke_type);
    F = fh_->__ResolveInternalMethod();
#endif
    D3LOG(INFO) << "InitInner: ResolveInternalMethod: "
      << method_ref.PrettyMethod()
      << " [InitializeInitInnerMethods]";
  }

#if defined(ART_MCR_ANDROID_6)
  art_method_init_ = irb_->CreateCall(F, args);
#endif
  art_method_init_->setName("art_method" + extra);

  irb_->CreateStore(art_method_init_, gbl_art_method_);
  ih_->LlvmInvariantStart(gbl_art_method_);
}

/**
 * @brief Creates check_inited, and already inited_blocks for both
 * ichf_init and include init
 *
 * @param init_func
 * @param block
 */
void HGraphToLLVM::InitializeInitInnerMethodsCommon(
    Function* init_func,
    BasicBlock*& block) {
  BasicBlock* entry_block =
    BasicBlock::Create(*ctx_, "check_init", init_func);
  irb_->SetInsertPoint(entry_block);
  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "InitInner: check_init: " + init_func->getName().str() + "\n");
  }

  block = BasicBlock::Create(*ctx_, "initialize", init_func);

  BasicBlock* already_inited =
    BasicBlock::Create(*ctx_,
        "already_inited", init_func);
  Value* loaded_is_inited = irb_->CreateLoad(gbl_inner_inited_);
  irb_->CreateCondBr(loaded_is_inited, already_inited, block);

  irb_->SetInsertPoint(block);

  irb_->CreateStore(irb_->getCBool(1), gbl_inner_inited_);
  ih_->LlvmInvariantStart(gbl_inner_inited_);

  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "InitInner: init: " + init_func->getName().str());
  }

  irb_->SetInsertPoint(already_inited);

  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "InitInner: [ret] was-inited:  " +
        init_func->getName().str() + "\n");
  }

  irb_->CreateRetVoid();
}

/**
 * @brief Adds code to resolve an ArtMethod in the inner init function
 *        This method is generated as we traverse the HGraph, for each
 *        invoke command
 * @param is_hot if it's true, doesn't mean that the method will be actually
 *        get called directly (in case of speculation miss will go through RT)
 */
void HGraphToLLVM::AddToInitInnerMethods(
    std::string callee_name, HInvoke* invoke,
    uint32_t resolved_dex_method_idx,
    bool is_hot, bool is_native, bool is_abstract,
    std::string signature,
    std::string dex_filename,
    std::string dex_location,
    bool resolve_ext) {
  D1LOG(INFO) << __func__ << ": "
    << (is_hot ? "HOT " : "COLD ") << callee_name
    << ":" << resolved_dex_method_idx
    << ":" << invoke->GetInvokeType()
    << "\nResolve:" << (resolve_ext ? "external" : "internal")
    << "\ndex_loc: " << dex_location;
  if (IsArtMethodInitialized(callee_name)) return;

  BasicBlock* prev_block = irb_->GetInsertBlock();

  if (!is_hot && !is_native) {
    if(mcr::McrRT::IsFrameworkDexLocation(dex_location)) {
      mcr::Analyser::AddColdMethodInternal(callee_name);
    } else if(!is_abstract && !mcr::Match::IsMarkedNotHot(callee_name)) {
      mcr::Analyser::AddColdMethod(callee_name);
    }
  }

  D3LOG(INFO) << __func__ << ": "
    << (is_hot ? "HOT " : "COLD ") << callee_name;

  // we need original invoke type here, because
  // InitArtMethodLocally needs original invoke type.
  InvokeType invoke_type = invoke->GetInvokeType();

  if (is_hot) {
    bool is_static = false;
    if (invoke->IsInvokeStaticOrDirect()) {
      is_static = invoke->AsInvokeStaticOrDirect()->IsStatic();
    }
    std::string callee_uniq_name =
      GetCallingMethodName(callee_name, signature,
          is_static, INIT_DIRECT);
    DLOG(INFO) << __func__ << ": callee_uniq_name: " << callee_uniq_name;
    FunctionType* ty = fh_->GetInitInnerFromInitTy(irb_);
    Function* init_inner_func =
      cast<Function>(mod_->getOrInsertFunction(callee_uniq_name, ty).getCallee());

    if (!InitializedInnerMethod(init_inner_func)) {
      // Call from ichf to init of inner
      CallInitInner(init_inner_func, callee_name, invoke, art_method_ichf_,
          init_inner_from_ichf_block_, resolved_dex_method_idx,
          invoke_type);

      // Call from init to init of inner
      CallInitInner(init_inner_func, callee_name, invoke, art_method_init_,
          init_inner_from_init_block_, resolved_dex_method_idx,
          invoke_type);
    }
  } else {  // initialize non hot methods
    // init locally in ichf init
    InitArtMethodLocally(callee_name, invoke, invoke->GetDexMethodIndex(),
        art_method_ichf_, init_inner_from_ichf_block_,
        invoke_type, dex_filename, dex_location, resolve_ext);
    // init locally in init
    InitArtMethodLocally(callee_name, invoke, invoke->GetDexMethodIndex(),
        art_method_init_, init_inner_from_init_block_,
        invoke_type, dex_filename, dex_location, resolve_ext);
  }

  irb_->SetInsertPoint(prev_block);
  AddToInitializedArtMethods(callee_name);
}

/**
 * @brief Called for non hot methods
 */
void HGraphToLLVM::InitArtMethodLocally(
    std::string callee_name, HInvoke* invoke,
    uint32_t resolved_dex_method_idx,
    Value* art_method, BasicBlock* block,
    InvokeType invoke_type,
    std::string dex_filename, std::string dex_location,
    bool resolve_ext) {
  D3LOG(INFO) << __func__ << ": " << callee_name;
  irb_->SetInsertPoint(block);

  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "InitInner: call init-locally: " + callee_name + "\n");
  }

  GlobalVariable* gbl_art_method =
    GetGlobalArtMethod(callee_name, invoke, true);
  Value* resolved_method = nullptr;
  std::string extra;
  std::vector<Value*> args;
  Function* F = nullptr;
  // INFO if it's OS method, but we are compiling an OS method (meaning both methods
  // are in e.g. core-libart.jar, then it will be resolved as internal method)
  if (resolve_ext) {
    D3LOG(INFO) << "ResolveExternalMethod: " << resolved_dex_method_idx
      << " hinvoke:" << invoke->GetDexMethodIndex()
      << "\nDexFile: " << dex_location << " [InitArtMethodLocally]";

    Value* ldex_filename = irb_->mCreateGlobalStringPtr(dex_filename);
    Value* ldex_location = irb_->mCreateGlobalStringPtr(dex_location);
#if defined(ART_MCR_ANDROID_10)
    UNUSED(F);
    resolved_method = ArtCallResolveExternalMethod(art_method, ldex_filename,
        ldex_location, resolved_dex_method_idx, invoke_type);
#elif defined(ART_MCR_ANDROID_6)
    args.push_back(art_method);
    args.push_back(ldex_filename);
    args.push_back(ldex_location);
    args.push_back(irb_->getJUnsignedInt(resolved_dex_method_idx));
    args.push_back(irb_->getJUnsignedInt(
          static_cast<uint32_t>(invoke->GetInvokeType())));
    F = fh_->__ResolveExternalMethod();
#endif
    extra = "_ext";
  } else {
    D3LOG(INFO) << "ResolveInternalMethod:" << resolved_dex_method_idx
      << " hinvoke:" << invoke->GetDexMethodIndex()
      << " [InitArtMethodLocally]";
#if defined(ART_MCR_ANDROID_10)
    resolved_method = ArtCallResolveInternalMethod(
        art_method, resolved_dex_method_idx, invoke_type);
#elif defined(ART_MCR_ANDROID_6)
    args.push_back(art_method);
    args.push_back(irb_->getJUnsignedInt(resolved_dex_method_idx));
    args.push_back(irb_->getJUnsignedInt(
          static_cast<uint32_t>(invoke_type)));
    F = fh_->__ResolveInternalMethod();
#endif
  }
#if defined(ART_MCR_ANDROID_6)
  resolved_method = irb_->CreateCall(F, args);
#endif

  resolved_method->setName("resolve_method_non_hot" + extra);
  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "InitInner: ret  init-locally: " +
        resolved_method->getName().str() + "\n");
    ArtCallVerifyArtMethod(resolved_method);
  }

  irb_->CreateStore(resolved_method, gbl_art_method);
  ih_->LlvmInvariantStart(gbl_art_method);
}

void HGraphToLLVM::CallInitInner(
    Function* init_inner_func, std::string callee_name,
    HInvoke* invoke, Value* referrer_art_method,
    BasicBlock* block, uint32_t dex_method_index,
    InvokeType invoke_type) {
  irb_->SetInsertPoint(block);

  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "InitInner: call init-inner: " +
        init_inner_func->getName().str() + "\n");
  }
  Value* dex_method_idx = irb_->getJUnsignedInt(dex_method_index);
  Value* linvoke_type = irb_->getJUnsignedInt(static_cast<uint32_t>(
        invoke_type));
  std::vector<Value*> args;
  args.push_back(referrer_art_method);
  args.push_back(dex_method_idx);
  args.push_back(linvoke_type);
  irb_->CreateCall(init_inner_func, args);

  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "InitInner: ret  init-inner: " +
        init_inner_func->getName().str() + "\n");
  }
}

/**
 * @brief add a return void to init_*
 */
void HGraphToLLVM::FinalizeInitInnerMethods() {
  BasicBlock* prev_block = irb_->GetInsertBlock();
  irb_->SetInsertPoint(init_inner_from_ichf_block_);
  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "ret  init: " +
        init_inner_from_ichf_func_->getName().str() + "\n");
  }
  irb_->CreateRetVoid();
  irb_->SetInsertPoint(init_inner_from_init_block_);
  if (McrDebug::VerifyInitInner()) {
    irb_->AndroidLogPrint(INFO, "ret  init: " +
        init_inner_from_init_func_->getName().str() + "\n");
  }
  irb_->CreateRetVoid();
  irb_->SetInsertPoint(prev_block);
}

/**
 * @brief Create all basic blocks. Initially will be empty. We need to
 *        create them as a separate pass so we can later on link the
 *        predecessors and successors.
 */
void HGraphToLLVM::GenerateBasicBlocksAndPhis() {
  D3LOG(INFO) << "GenerateBasicBlocksAndPhis";

  HBasicBlock* hblock_exit = nullptr;
  for (HBasicBlock* hblock : GetGraph()->GetBlocks()) {
    // some blocks might be removed by hgraph optimizations passes.
    // They remain in the iterator as null references though, despite
    // the fact that all art::Optimizing passes have finished before
    // invoking our HGraph to LLVM IR frontend
    // HGraph visitors cope with this issue.
    if (hblock == nullptr) continue;

    // place the at the end
    if(hblock->IsExitBlock()) {
      hblock_exit = hblock;
      continue;
    }

    BasicBlock* lblock = GenerateBasicBlock(hblock);
    irb_->SetInsertPoint(lblock);

    // INFO phi is the first instruction of the block..
    for (HInstructionIterator inst_it(hblock->GetPhis());  // each phi
        !inst_it.Done(); inst_it.Advance()) {
      HPhi* hphi = inst_it.Current()->AsPhi();
      uint32_t reg_num = hphi->GetRegNumber();
      if (hphi->GetRegNumber() == kNoRegNumber) {
        D3LOG(WARNING) << "NoRegNumber: " << prt_->GetInstruction(hphi);
        reg_num = 0;
      }
      if (hphi->IsDead()) {
        DLOG(ERROR) << "Dead PHI: " << prt_->GetInstruction(hphi)
          << " reg_num: " << reg_num << " (skipped)"
          << "\nMethod: " << GetPrettyMethod();
      }

      D3LOG(INFO) << "CreatingPhi: " << prt_->GetInstruction(hphi)
        << " regnum: " << reg_num;

      // here it actually doesn't matters if it's remapped or exact type,
      // because Phi's (as w/ ConstantInt) use i32 as smaller integral type
      Type* phiTy = irb_->getType(hphi);
      PHINode* lphi = irb_->CreatePHI(phiTy, reg_num, GetTwine(hphi));
      addValue(hphi, lphi);
    }
  }

  if(hblock_exit == nullptr) {
    std::stringstream ss;
    ss << "Not an exit block. Is there an infinite loop? "
      << "\nMethod: " << GetPrettyMethod();
    DLOG(ERROR) << ss.str();
    LlvmCompiler::LogError(ss.str());
  } else {
    // add exit block at the end
    CHECK(hblock_exit->GetPhis().IsEmpty()) << "No phis on exit block";
    BasicBlock* lblock_exit = GenerateBasicBlock(hblock_exit);
    irb_->SetInsertPoint(lblock_exit);
  }
}

void HGraphToLLVM::PopulatePhis() {
  INFO4_;
  for (HBasicBlock* hblock : GetGraph()->GetBlocks()) {
    if (hblock == nullptr) continue;
    for (HInstructionIterator inst_it(hblock->GetPhis());  // each phi
        !inst_it.Done(); inst_it.Advance()) {
      HPhi* hphi = inst_it.Current()->AsPhi();
      PopulatePhi(hphi);
    }
  }
}

void HGraphToLLVM::LinkEntryBlock() {
  HBasicBlock* entry_hblock = GetGraph()->GetEntryBlock();

  BasicBlock* entry_lblock = &inner_func_->getEntryBlock();
  BasicBlock* entry_to_hgraph = getBasicBlock(entry_hblock);
  entry_to_hgraph->moveAfter(entry_lblock);
  LinkBasicBlocks(entry_lblock, entry_to_hgraph);
}

void HGraphToLLVM::LinkBasicBlocks(BasicBlock* src,
    BasicBlock* trgt) {
  irb_->SetInsertPoint(src);
  irb_->CreateBr(trgt);
}

/**
 * @brief Entrypoint from llvm
 *
 * Entrypoint Paremeters:
 *  - uint32_t*, JValue*, void*, void*, uint32_t
 *
 * @param is_live 
 */
void HGraphToLLVM::GenerateEntrypointLLVM(bool is_live) {
  CHECK(is_live) << "Only live llvm supported in this project";
  Type* ret_type = irb_->getJVoidTy();

  /* Parameters:
   *   uint32_t*      : args
   *   JValue*        : store return result
   *   Thread*        : self
   *   ArtMethod*     : current method
   *   BootImageBegin : Beginning of Boot Image in art::Heap
   */
  std::vector<Type*> args_type {
    irb_->getVoidPointerType(),             // ArtMethod*
      irb_->getMethodArgsTy(),              // args*
      irb_->getJValueTy()->getPointerTo(),  // JValue*
      irb_->getVoidPointerType(),           // Thread*
      irb_->getJIntTy()                     // boot_image_begin
  };
  FunctionType* ty = FunctionType::get(ret_type, args_type, false);

  std::string symbol = (is_live? "llvm_live_" :"llvm_");
  Function* f = Function::Create (ty, Function::ExternalLinkage, symbol, mod_);
  f->setCallingConv(CallingConv::C);

  BasicBlock* lblock =
    BasicBlock::Create(*ctx_, ENTRY_LLVM, f);
  irb_->SetInsertPoint(lblock);

  // enable LLVMDBG when we call back into the runtime
  if(McrDebug::DebugLLVM()) {
    irb_->CreateCall(fh_->__EnableDebugLLVM());
  }

  // hide the entry, show just the exit when verifying that
  // LLVM has ran
  if (McrDebug::DebugLLVM() && McrDebug::VerifyLlvmCalled()) {
    irb_->AndroidLogPrint(INFO, "LLVM entered.");
  }

  if (McrDebug::VerifyBasicBlock(GetPrettyMethod())) {
    std::string info = " entrypoint";
    if (is_live) info+= " [liveLLVM]";
    PrintBasicBlockDebug(lblock, ENTRY_LLVM, info);
  }

  Function::arg_iterator arg_iter(f->arg_begin());
  Value* art_method = &*arg_iter++;
  Value* args = &*arg_iter++;
  Value* jvalue = &*arg_iter++;
  Value* thread = &*arg_iter++;
  Value* boot_image_begin = &*arg_iter++;

  art_method->setName("ArtMethodICHF");
  args->setName("args");
  jvalue->setName("jvalue");
  thread->setName("thread");
  boot_image_begin->setName("boot_image_begin");

  // Store it so we can access for GetQuickEntrypoint outer methods
  CacheThreadForCurrentMethod(thread);
  SetThreadRegister(thread); // VERIFIED_10

  if (McrDebug::VerifyLlvmCalled() && McrDebug::DebugLlvmCode2()) {
    irb_->AndroidLogPrint(INFO, "==> LLVMhf: " + GetPrettyMethod()); 
  }

  if(McrDebug::DebugLlvmCode3()) {
    irb_->AndroidLogPrintHex(INFO, "Thread", thread);
    fh_->VerifyThread(this, irb_);
    irb_->AndroidLogPrint(WARNING, "======== LL0: LLVM entered ======");
    ArtCallVerifyStackFrameCurrent();
    irb_->AndroidLogPrint(WARNING, "=================================");
  }

  irb_->CreateStore(boot_image_begin, gbl_boot_image_begin_);
  // mark the global that won't change from now on
  ih_->LlvmInvariantStart(gbl_boot_image_begin_); // VERIFIED
  if(McrDebug::DebugLlvmCode3()) {
    irb_->AndroidLogPrintHex(INFO, "LLVM: boot_image_begin",
        LoadGlobalBootImageBegin());
  }

  if (McrDebug::VerifyArtMethod()) {
    irb_->AndroidLogPrint(INFO, "llvm:method");
    ArtCallVerifyArtMethod(art_method);

    if (McrDebug::VerifyArtClass()) {
      irb_->AndroidLogPrint(INFO, "llvm:method:class:");
      ArtCallVerifyArtClass(GetArtMethodClass(art_method));
    }
  }

  // Unpack arguments
  uint32_t shorty_len;
  const char* shorty = dcu_.GetShorty(&shorty_len);
  std::vector<Value*> args_inner_method;
  UnpackArguments(lblock, args,
      dcu_.IsStatic(),
      args_inner_method, shorty, shorty_len);

  std::vector<Value*> args_init_inner;
  args_init_inner.push_back(art_method);
  CHECK(init_ != nullptr) << "GenerateEntrypointInit first!";
  irb_->CreateCall(init_, args_init_inner);

  if (McrDebug::VerifyBasicBlock(GetPrettyMethod())) {
    PrintBasicBlockDebug(lblock, ENTRY_LLVM, "[after init_inner]");
  }

  Value* ret = irb_->CreateCall(inner_func_, args_inner_method);
  DataType::Type ret_jt = DataType::FromShorty(shorty[0]);
  if (ret_jt != DataType::Type::kVoid) {
    ret->setName("result");
    irb_->CallStoreReturnValue(jvalue, ret, ret_jt);
  }

  if (McrDebug::VerifyLlvmCalled()) {
    irb_->AndroidLogPrint(INFO, "<== LLVMhf: " + GetPrettyMethod()); 
  }

  irb_->CreateRetVoid();
}

void HGraphToLLVM::GenerateEntrypointInit() {
  INFO4_;
  Type* ret_type = irb_->getJVoidTy();
  std::vector<Type*> args_type;
  args_type.push_back(irb_->getVoidPointerType());
  FunctionType* ty =
    FunctionType::get(ret_type, args_type, false);

  Function* f = Function::Create
    (ty, Function::ExternalLinkage, "init_", mod_);
  f->setCallingConv(CallingConv::C);

  BasicBlock* lblock =
    BasicBlock::Create(*ctx_, ENTRY_LLVM, f);
  irb_->SetInsertPoint(lblock);

  if (McrDebug::VerifyBasicBlock(GetPrettyMethod())) {
    PrintBasicBlockDebug(lblock);
  }

  Function::arg_iterator arg_iter(f->arg_begin());
  Value* art_method = &*arg_iter++;
  art_method->setName("art_method");

  std::vector<Value*> args_init_inner;
  args_init_inner.push_back(art_method);
  irb_->CreateCall(init_inner_from_ichf_func_, args_init_inner);

  if (McrDebug::VerifyBasicBlock(GetPrettyMethod())) {
    PrintBasicBlockDebug(lblock, "", "[after init inner]");
  }

  irb_->CreateRetVoid();

  init_ = f;
}

void HGraphToLLVM::SetThreadRegister(Value* thread_self) {
  InstructionSet isa = llcu_->GetInstructionSet();

  switch (isa) {
    case InstructionSet::kThumb2:
      ArmThumb::SetThreadRegister(irb_, thread_self);
      break;
    case InstructionSet::kArm64:
      Arm64::SetThreadRegister(irb_, thread_self);
      break;
    default:
      DLOG(FATAL) << "Unimplemented for architecture: " << isa;
  }
}

void HGraphToLLVM::UnpackArguments(BasicBlock* lblock,
    Value* args,
    bool is_static,
    std::vector<Value*>& args_inner_method,
    const char* shorty, uint32_t shorty_len) {
  uint32_t args_index = 0;
  if (!is_static) {  // implicit receiver
    D5LOG(INFO) << "Unpacking receiver: idx: " << args_index;
    Value* receiver = UnpackArgument(lblock, args, args_index, 'L');
    receiver->setName("receiver");
    args_inner_method.push_back(receiver);
  }

  for (uint32_t i = 1; i < shorty_len; i++) {  // rest of the arguments
    D5LOG(INFO) << "Unpacking argument: " << args_index;
    args_inner_method.push_back(UnpackArgument(lblock, args, args_index, shorty[i]));
  }
}

Value* HGraphToLLVM::UnpackArgument(BasicBlock* lblock,
    Value* args,
    uint32_t& args_index,
    const char shortyi) {
  D5LOG(INFO) << "UnpackArgument: " << shortyi;
  DataType::Type type = DataType::FromShorty(shortyi);
  CHECK(type != DataType::Type::kVoid) << "Argument cannot be void";

  Value* loaded_arg = LoadArgument(lblock, shortyi, args, args_index);

  // move to next index
  args_index++;
  // 2x slots if long/double
  if (DataType::Is64BitType(type)) args_index++;

  return loaded_arg;
}

Value* HGraphToLLVM::CastForStorage(Value* to_store_val,
    DataType::Type type,
    Type* storeTy) {
  if (storeTy != to_store_val->getType()) {
    bool is_signed = IRBuilder::IsSigned(type);
    CHECK(!DataType::IsFloatingPointType(type))
      << "CastForStorage: FP types should have matched already store type";
    return irb_->CreateIntCast(to_store_val, storeTy, is_signed);
  }
  return to_store_val;
}

Value* HGraphToLLVM::GetDynamicOffset(
    Value* lindex, size_t shift, Value* loffset) {
  Value* shift_num = irb_->getJUnsignedInt(shift);
  Value* shifted = irb_->CreateShl(lindex, shift_num);
  return irb_->mCreateAdd(false, loffset, shifted);
}

Value* HGraphToLLVM::GetDynamicOffset(
    Value* lindex, size_t shift, uint32_t offset) {
  Value* loffset = irb_->getJUnsignedInt(offset);
  return GetDynamicOffset(lindex, shift, loffset);
}

Value* HGraphToLLVM::GetDynamicOffset(
    HInstruction* index, size_t shift, uint32_t offset) {
  Value* lindex = getValue(index);
  Value* loffset = irb_->getJUnsignedInt(offset);
  return GetDynamicOffset(lindex, shift, loffset);
}

bool HGraphToLLVM::SpeculativeInSameDexFile(mcr::InvokeInfo invoke_info) {
  std::string spec_dex_loc = invoke_info.GetDexLocation();
  std::string caller_dex_loc = GetGraph()->GetDexFile().GetLocation();

  return (caller_dex_loc.compare(spec_dex_loc) == 0);
}

ArtMethod* HGraphToLLVM::ResolveSpeculativeMethod(mcr::InvokeInfo invoke_info) {
  std::string dex_filename = invoke_info.GetDexFilename();
  std::string dex_location = invoke_info.GetDexLocation();
  std::string caller_dex_location = GetGraph()->GetDexFile().GetLocation();
  std::string caller_dex_filename =
    DexFileLoader::GetBaseLocation(caller_dex_location.c_str());
  bool same_dex = SpeculativeInSameDexFile(invoke_info);

  D3LOG(INFO) << __func__ << ": "
    << std::to_string(invoke_info.GetSpecMethodIdx())
    << ":" << mcr::McrCC::PrettyDexFile(dex_location) << " "
    << (same_dex ? "(same dex)" : "(different dex)");

  const DexFile* dex_file;
  if (same_dex) {
    dex_file = &GetGraph()->GetDexFile();
  } else {
    dex_file = llcu_->GetDexFile(dex_filename, dex_location);
  }

  D5LOG(INFO) << "calling ResolveMethod: " << invoke_info.GetSpecMethodIdx();
  ArtMethod* method = ResolveMethod(dex_file, invoke_info.GetSpecMethodIdx(),
      invoke_info.GetSpecInvokeType());
  Locks::mutator_lock_->SharedLock(Thread::Current());
  D5LOG(INFO) << "[DONE] ResolveMethod: " << invoke_info.GetSpecMethodIdx()
    << method->PrettyMethod();
  Locks::mutator_lock_->SharedUnlock(Thread::Current());
  return method;
}

ArtMethod* HGraphToLLVM::ResolveLocalMethod(
    uint32_t dex_method_idx, InvokeType invoke_type) {
  D3LOG(INFO) << __func__ << ": " << dex_method_idx;
  const DexFile* dex_file = &GetGraph()->GetDexFile();
  ArtMethod* resolved_method=
    ResolveMethod(dex_file, dex_method_idx, invoke_type);
  CHECK(resolved_method != nullptr) << "Failed to resolve method.";
  return resolved_method;
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
