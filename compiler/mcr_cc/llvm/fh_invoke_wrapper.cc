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

#include <regex>
#include <llvm/IR/GlobalVariable.h>
#include "art_method-inl.h"
#include "art_method.h"
#include "asm_arm64.h"
#include "asm_arm_thumb.h"
#include "dex/dex_file_loader.h"
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"
#include "ir_builder.h"
#include "llvm_compiler.h"
#include "llvm_utils.h"
#include "mcr_cc/os_comp.h"
#include "mcr_cc/analyser.h"
#include "mcr_cc/invoke_histogram.h"
#include "mcr_rt/invoke_info.h"

#include "llvm_macros_irb.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

inline FunctionType* GetTy(
    IRBuilder* irb, DataType::Type ret_type, HInvoke* invoke) {
  Type* lret_type = irb->getType(ret_type);
  std::vector<Type*> args_type;
  for (u_int32_t i = 0; i < invoke->GetNumberOfArguments(); i++) {
    args_type.push_back(irb->getType(invoke->InputAt(i)));
  }
  return FunctionType::get(lret_type, args_type, false);
}

inline std::string getInvokeTypeStr(HInvoke* h) {
  if(h->IsInvokeStaticOrDirect()) {
    if(h->AsInvokeStaticOrDirect()->IsStatic()) {
      return "Static";
    } else {
      return "Direct";
    }
  } else {
    std::stringstream ss;
    ss << h->GetKind();
    return std::regex_replace(ss.str(), std::regex("Invoke"), "");
  }
}

/**
 * @brief Calculate name for invoke virtual according to the speculation
 * that we will do.
 */
inline std::string GetSpecName(InvokeHistogram* histogram,
    HInvoke* invoke, HGraphToLLVM* HL,
    std::string orig_pretty_callee,
    std::string signature, bool is_native) {

  std::stringstream ss;
  ss << "Invoke";
  ss << (is_native ? "JNI" : "");
  ss << getInvokeTypeStr(invoke);
  ss << "." << HL->GetMethodName(orig_pretty_callee, false, signature);

#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
  if (LlvmCompiler::GenerateInvokeHistogram()) {
    ss << ".GenHist";
  }
#endif

  if (histogram != nullptr) {
    ss << ".Spec";
    D1LOG(WARNING) << mcr::McrCC::PrettyHistogramHeader();
    int cnt=1;
    for (SortedSpeculationIterator it(histogram->begin());
        it != histogram->end(); it++, cnt++) {
      ss << "." << std::to_string(it->GetSpecClassIdx());
      ss << "m" << std::to_string(it->GetSpecMethodIdx());
      DLOG(INFO) << mcr::McrCC::PrettyHistogramLine(*it, cnt);
    }
  }

  return ss.str();
}

inline void CreateBasicBlocks(IRBuilder* irb, InvokeHistogram* histogram,
    Function* invoke_virtual, BasicBlock*& entry_block, BasicBlock*& spec_miss,
    bool is_native, std::map<std::string, BasicBlock*>& blocks) {
  entry_block = BasicBlock::Create(
      irb->getContext(), "entry_iv", invoke_virtual);
  if (histogram != nullptr) {
    for (SortedSpeculationIterator it(histogram->begin());
         it != histogram->end(); it++) {
      std::string sidx = std::to_string(it->GetSpecClassIdx());

      std::string block_name = "class" + sidx +"_";
      std::pair<std::string, BasicBlock*>
          hit_pair(block_name,
                   BasicBlock::Create(
                       irb->getContext(), block_name, invoke_virtual));
      blocks.insert(hit_pair);
    }
  }
  // && !is_native
  std::string miss_block_name =
#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
      ((LlvmCompiler::GenerateInvokeHistogram())
           ? (histogram == nullptr ? "gen_hist" : "miss") : "invoke");
#else
      "invoke";
#endif

  spec_miss = BasicBlock::Create(
      irb->getContext(), miss_block_name, invoke_virtual);
}

#define DBG_BB(BB)  \
  if (dbgbb) { \
    irb->AndroidLogPrint(INFO, "BB: " + Pretty(BB) + "\n"); \
  }

/**
 * @brief Handles super and interface calls as well
 */
Function* FunctionHelper::InvokeWrapper(
    ArtMethod* art_method, bool is_native,
    HGraphToLLVM* HL, IRBuilder* irb, IntrinsicHelper *IH,
    HInvoke* hinvoke, std::string signature,
    const char* shorty, uint32_t shorty_len,
    DataType::Type ret_type) {
  CHECK(!hinvoke->IsInvokeStaticOrDirect())
    << __func__ << ": must be interface, virtual, or super";

  D3LOG(INFO) << __func__ << ": " << art_method->PrettyMethod()
    << ": dIDX" << art_method->GetDexMethodIndex();

  CHECK(!(hinvoke->IsInvokeUnresolved() ||
        hinvoke->IsInvokePolymorphic() ||
        hinvoke->IsInvokeCustom()))
    << __func__ << ": Implement invoke instruction:"
    << HL->GetTwine(hinvoke);

  std::string orig_pretty_callee = art_method->PrettyMethod();
  InvokeHistogram* histogram =
    new InvokeHistogram(HL->GetMethodIdx(), hinvoke->GetDexPc(),
        HL->GetGraph()->GetDexFile().GetLocation().c_str());

  if (!histogram->HasSpeculation()) {
    delete histogram;
    histogram = nullptr;
  }

  // calculate the virtual invoke wrapper method's name
  std::string invoke_method_name = GetSpecName(
        histogram, hinvoke, HL, orig_pretty_callee, signature, is_native);

  // method already defined, so return it
  if (invoke_virtuals_.find(invoke_method_name) != invoke_virtuals_.end()) {
    return invoke_virtuals_[invoke_method_name];
  }

  // create (define) the llvm method
  D2LOG(INFO) << "Creating function: " << invoke_method_name;

  BasicBlock* pinsert_point = irb->GetInsertBlock();

  // create and store method
  FunctionType* invoke_virtualTy = GetTy(irb, ret_type, hinvoke);
  Function* invoke_virtual =
      Function::Create(invoke_virtualTy, Function::LinkOnceODRLinkage,
          invoke_method_name, irb->getModule());
  invoke_virtuals_.insert(std::pair<std::string, Function*>(
      invoke_method_name, invoke_virtual));
  // invoke_virtual->addFnAttr(Attribute::AlwaysInline);
  invoke_virtual->setDSOLocal(true);
  AddAttributesCommon(invoke_virtual);

  // process arguments
  Function::arg_iterator arg_iter(invoke_virtual->arg_begin());
  Value* receiver = nullptr;
  std::vector<Value*> callee_args;
  for (int i = 0; arg_iter != invoke_virtual->arg_end(); arg_iter++, i++) {
    Argument* arg = &*arg_iter;
    D3LOG(INFO) << "Arg: " << Pretty(arg->getType());
    if (i == 0) {  // 1st should always be the receiver
      receiver = arg;
      receiver->setName("receiver");
    } else {
      arg->setName("arg" + std::to_string(i));
      callee_args.push_back(arg);
    }
  }

  // Create all necessary basic blocks
  BasicBlock *entry_block = nullptr, *spec_miss = nullptr;
  std::map<std::string, BasicBlock*> blocks;
  CreateBasicBlocks(irb, histogram, invoke_virtual,
      entry_block, spec_miss, is_native, blocks);
  irb->SetInsertPoint(entry_block);
  HL->SetCurrentMethodEntryBlock(entry_block);

  const bool dbg = McrDebug::VerifyLlvmInvokeWrapper();
  const bool dbgbb = dbg && McrDebug::VerifyBasicBlock() && false;
  if (dbg) {
    irb->AndroidLogPrint(INFO, "LLVM:InvokeWrapper: " + invoke_method_name);
  }
  DBG_BB(entry_block);

  const bool use_histogram =
    histogram != nullptr && McrDebug::SpeculativeDevirt();

if(histogram == nullptr && !McrDebug::SpeculativeDevirt()) {
    irb->AndroidLogPrint(WARNING, "Histogram is null");
  }

  if(histogram != nullptr && !McrDebug::SpeculativeDevirt()) {
    irb->AndroidLogPrint(WARNING, "SpeculativeDevirt disabled!");
  }

  Value* obj_class_idx = nullptr;
  if (use_histogram) {
    ReadBarrierOption RBO = ReadBarrierOption::kWithoutReadBarrier;
    Value* obj_class =
      HL->GenerateReferenceObjectClass(hinvoke, receiver, RBO);
    obj_class->setName("obj_class");
    obj_class_idx = HL->GetClassTypeIdx(obj_class);
    obj_class_idx->setName("obj_classIDX");

    if(McrDebug::VerifyArtMethod() && McrDebug::DebugLlvmCode4()) {
      HL->ArtCallVerifyArtObject(receiver);
      irb->AndroidLogPrintHex(INFO, "cls: ", obj_class);
      HL->ArtCallVerifyArtClass(obj_class);
    }
  }

  if (histogram != nullptr && is_native && McrDebug::DebugLlvmCode4()) {
    std::string msg = "Native with a histogram: " + art_method->PrettyMethod();
    D2LOG(ERROR) << msg;
    irb->AndroidLogPrint(ERROR, msg + "\n");
  }

  std::vector<uint32_t> weights;
  if (use_histogram) {
    SwitchInst* Switch = SwitchInst::Create(
        obj_class_idx, spec_miss, histogram->GetSize(), irb->GetInsertBlock());

    // populate instructions for all blocks
    for (SortedSpeculationIterator it(histogram->begin());
        it != histogram->end(); it++) {
      mcr::InvokeInfo invoke_info = *it;
      // const bool is_first = it == histogram->begin();
      // const bool is_last = std::next(it) == histogram->end();
      const uint32_t class_idx = invoke_info.GetSpecClassIdx();

      // OPTIMIZE_LLVM recursive virtual calls:
      // if class_idx == HL->GetArtMethod()->GetClass:
      // then mark the call as tail

      uint32_t W = MAX_BRWEIGHT;
      if(histogram->GetSize() > 1) {
        W = std::min(MAX_BRWEIGHT, invoke_info.GetSpecInvokeType()*100);
        DLOG(WARNING) << "Histogram with multiple entries!";
      }
      weights.push_back(W);

      // a method: loop iterator... and then set full weight..
      std::string sidx = std::to_string(class_idx);

      BasicBlock* block = blocks["class" + sidx + "_"];
      CHECK(block != nullptr);
      CHECK(obj_class_idx !=nullptr);

      Switch->addCase(irb->getJUnsignedInt(class_idx), block);
      ArtMethod* spec_art_method = HL->ResolveSpeculativeMethod(invoke_info);

      // Update stuff that will be used by hit block
      hinvoke->SetSpeculation(&invoke_info);
      MethodReference spec_method_ref(spec_art_method->GetDexFile(),
          spec_art_method->GetDexMethodIndex());

      bool is_hot = mcr::McrCC::isHot(spec_method_ref);
      D2LOG(INFO) << "HOTspec:" << is_hot << " " 
        << spec_method_ref.index << ":" << spec_method_ref.PrettyMethod();

      // INFO speculation hit
      irb->SetInsertPoint(block);
      DBG_BB(block){
        if (McrDebug::VerifySpeculation()) {
          CHECK(obj_class_idx!=nullptr) << "spec-hit: class idx null";
          VerifySpeculation(INFO, irb, obj_class_idx, "hit", spec_method_ref.PrettyMethod());
        }

        Value* call_result;
        if (is_hot) {
          std::vector<Value*> args_call_directly;
          args_call_directly.push_back(receiver);
          args_call_directly.insert(args_call_directly.end(),
              callee_args.begin(), callee_args.end());
          call_result = HL->CallHotMethod(
              irb, art_method, spec_art_method, hinvoke, signature, shorty,
              shorty_len, is_native, false, ret_type, args_call_directly, true);
        } else {
          // is_hot is false anyway here, so we send false.
          D3LOG(INFO) << __func__ << ": CallColdMethod:" << hinvoke->GetDexMethodIndex();
          call_result = HL->CallColdMethod(
              false, irb, art_method, spec_art_method, hinvoke, signature, shorty,
              shorty_len, is_native, false, ret_type, receiver, callee_args, true);
        }

        // same on the other as well..
        if (ret_type != DataType::Type::kVoid) {
          if(call_result != nullptr) {
            call_result->setName("result");
            irb->CreateRet(call_result);
          } else { // invoke with no uses returns null
            irb->CreateRet(irb->getJZero(ret_type));
          }
        } else {
          irb->CreateRetVoid();
        }
      }  // spec-hit
    }  // for each speculation

    // Zero weight fro miss case
    weights.push_back(0);

    MDNode *N=HL->MDB()->createBranchWeights(weights);
    Switch->setMetadata(LLVMContext::MD_prof, N);
  } else {
    irb->CreateBr(spec_miss);
  }

  // SPEC MISS: load from RT and call. generate histogram if option enabled
  irb->SetInsertPoint(spec_miss);
  DBG_BB(spec_miss);
  {
    hinvoke->UnsetSpeculation();
    MethodReference orig_method_ref(
        art_method->GetDexFile(), art_method->GetDexMethodIndex());
    const bool skip_addto_histogram = 
      IH->ExcludeFromHistogram(hinvoke->GetIntrinsic())
      || art_method->IsNative()
      || OsCompilation::IsOsMethodsBlocklisted(orig_pretty_callee);

    if (McrDebug::VerifySpeculationMiss() && !skip_addto_histogram) {
      if(use_histogram) {
        CHECK(obj_class_idx!=nullptr) << "spec-miss : class idx null";
        VerifySpeculation(WARNING, irb,
            obj_class_idx,
            "miss_hist", orig_pretty_callee, McrDebug::DieOnSpeculationMiss());
      } else {
        VerifySpeculation(ERROR, irb,
            art_method->GetDeclaringClass()->GetDexTypeIndex().index_,
            "miss_no_hist", orig_pretty_callee, McrDebug::DieOnSpeculationMiss());
      }
    }

    // INFO regardless whether the method is hot or not, it will be
    // called through RT. This is because we will resolve it using receiver
    // right before the call, meaning that the resolve ArtMethod can be
    // different.
    // we still use is_hot for getting the global ArtMethod variable though,
    // so we can be consistent (and correct) with our initialization process.

    bool is_hot = mcr::McrCC::isHot(orig_method_ref);
    GlobalVariable* callee_art_method =
        HL->GetGlobalArtMethod(orig_pretty_callee, hinvoke, !is_hot);
    Value* lart_method = irb->CreateLoad(callee_art_method);
    lart_method->setName("art_method");

#if defined(ART_MCR_ANDROID_6)
    std::vector<Value*> args_resolve_virtual;
    args_resolve_virtual.push_back(receiver);
    args_resolve_virtual.push_back(lart_method);
    Function* F;
#endif

    Value* rt_resolved_vmethod =nullptr;
    uint32_t imt_index;
    if (hinvoke->IsInvokeInterface()) { 
#if defined(ART_MCR_ANDROID_10)
      imt_index= static_cast<uint32_t>(ImTable::OffsetOfElement(
            hinvoke->AsInvokeInterface()->GetImtIndex(),
            kArm64PointerSize));
      rt_resolved_vmethod=
        HL->ArtCallResolveInterfaceMethod(receiver, lart_method);
      if(McrDebug::DebugLlvmCode4()) {
        LOGLLVM4val(WARNING, "ResolveInterface: RT", rt_resolved_vmethod);
        HL->ArtCallVerifyArtMethod(rt_resolved_vmethod);
        // TODO: use directly LLVM, but have to do the
        // runtime method check, then the conflict
        // Value* cc_resolved_vmethod=
        //   HL->ResolveInterfaceMethod(hinvoke->AsInvokeInterface(), receiver);
      }

#elif defined(ART_MCR_ANDROID_6)
      // mirror::Class::kImtSize
      imt_index = hinvoke->AsInvokeInterface()->GetImtIndex() % 1;
      args_resolve_virtual.push_back(irb->getJUnsignedInt(imt_index));
      F = __ResolveInterfaceMethod();
#endif
      D2LOG(INFO) << "ResolveInterfaceMethod: imt_index: "
        << std::to_string(imt_index);
    } else {
#if defined(ART_MCR_ANDROID_10)
      rt_resolved_vmethod =
        HL->ArtCallResolveVirtualMethod(receiver, lart_method);
#elif defined(ART_MCR_ANDROID_6)
      F = __ResolveVirtualMethod();
#endif
    }
#if defined(ART_MCR_ANDROID_6)
    rt_resolved_vmethod =
        irb->CreateCall(F, args_resolve_virtual);
#endif
    rt_resolved_vmethod->setName("rt_resolved_vmethod");

// INFO here we generate invoke histogram in miss block
#ifdef MCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
    if (LlvmCompiler::GenerateInvokeHistogram() && !skip_addto_histogram) {
      // VERIFY_LLVM("GenerateInvokeHistogram");
      std::vector<Value*> args;
      // I must do the optimization with the entry block here as well
      Value* ichf_method = HL->LoadGlobalArtMethod();
      ichf_method->setName("caller");
      args.push_back(receiver);
      args.push_back(ichf_method);
      args.push_back(irb->getJUnsignedInt(hinvoke->GetDexPc()));
      args.push_back(rt_resolved_vmethod);
      irb->CreateCall(__AddToInvokeHistogram(), args);
    }
#endif

    D3LOG(INFO) << __func__ << ": spec_miss: CallColdMethod:" << hinvoke->GetDexMethodIndex();
    Value* call_result =
        HL->CallColdMethod(is_hot, irb, art_method, nullptr, hinvoke,
                           signature, shorty, shorty_len, is_native,
                           false, ret_type, receiver,
                           callee_args, true, rt_resolved_vmethod, true);
    if (ret_type != DataType::Type::kVoid) {
      if(call_result != nullptr) {
        call_result->setName("result");
        irb->CreateRet(call_result);
      } else { // invoke with no uses returns null
        irb->CreateRet(irb->getJZero(ret_type));
      }
    } else {
      irb->CreateRetVoid();
    }
  }  // spec-miss

  irb->SetInsertPoint(pinsert_point);
  HL->RestoreLlvmEntryBlock();
  invoke_virtual->addFnAttr(Attribute::AlwaysInline);
  return invoke_virtual;
}

#undef DBG_BB

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
