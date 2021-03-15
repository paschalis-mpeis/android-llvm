/**
 * Operations related to mapping (key, value) HGraph components
 * to LLVM components.
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
#include "hgraph_to_llvm-inl.h"
#include "hgraph_to_llvm.h"

#include "llvm_macros_irb_.h"
#include "llvm_utils.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

void HGraphToLLVM::addBasicBlock(HBasicBlock* hbb,
                                 BasicBlock* lbb) {
  lblocks_.insert(std::pair<HBasicBlock*, BasicBlock*>(hbb, lbb));
}

void HGraphToLLVM::addArgument(uint32_t index,
                               Argument* var) {
  D4LOG(INFO) << "addArgument: " << std::to_string(index)
              << ": " << var->getName().str();
  args_.insert(std::pair<uint32_t, Argument*>(index, var));
}

void HGraphToLLVM::addValue(HInstruction* h, Value* value) {
  value->setName(GetTwine(h));
  if (h->IsPhi()) {
    addPhi(h->AsPhi(), cast<PHINode>(value));
  } else {
    addRegister(h, value);
  }
}

void HGraphToLLVM::addRegister(HInstruction* h, Value* value) {
  regs_.insert(std::pair<HInstruction*, Value*>(h, value));
}

void HGraphToLLVM::addPhi(HPhi* hphi, PHINode* lphi) {
  phis_.insert(std::pair<HPhi*, PHINode*>(hphi, lphi));
}

void HGraphToLLVM::replaceRegisterTmpInstruction(
    HInstruction* tmph, HInstruction* h) {
  Value* value = getRegister(tmph);
  // delete the temporary instruction
  regs_.erase(tmph);
  // add value again with permanent instruction
  addRegister(h, value);
}

void HGraphToLLVM::updateRegister(HInstruction* h, Value* newvalue) {
  newvalue->setName(GetTwine(h));
  regs_.erase(h);
  addRegister(h, newvalue);
}

BasicBlock* HGraphToLLVM::getBasicBlock(HBasicBlock* hblock) {
  BasicBlock* lblock = lblocks_[hblock];
  CHECK(lblock != nullptr) << "Failed to find block: "
                           << GetBasicBlockName(hblock);
  return lblock;
}

HBasicBlock* HGraphToLLVM::getHBasicBlock(BasicBlock* lblock) {
  for(auto it= lblocks_.begin(); it!=lblocks_.end(); ++it) {
    if(it->second==lblock) {
      return it->first;
    }
  }

  DLOG(WARNING) << __func__
    << ": failed to get HBasicBlock for lblock: " << Pretty(lblock);
  return nullptr;
}

Value* HGraphToLLVM::LoadGlobalArtMethod() {
  CHECK(gbl_art_method_ != nullptr);
  VERIFIED_;
  Value* art_method = irb_->CreateLoad(gbl_art_method_, true);
  art_method->setName("ArtMethodHF");
  return art_method;
}

Value* HGraphToLLVM::LoadGlobalBootImageBegin() {
  return irb_->CreateLoad(gbl_boot_image_begin_, true);
}

Value* HGraphToLLVM::GetLoadedArtMethod(Function* F) {
  const bool innerFunc = (F== nullptr);
  std::string FOR="";
  
  if(innerFunc) {
    F= GetInnerFunction();
    FOR="INNER";
  } else {
    FOR=Pretty(F);
  }

  Value* value=nullptr;
  if(loaded_art_methods_.find(F) != loaded_art_methods_.end()) {
    value=loaded_art_methods_[F];
  } else {
    // find appropropriate code insertion point
    BasicBlock* pinsert_point = irb_->GetInsertBlock();
    if(innerFunc) {
      irb_->SetInsertPoint(cur_method_entry_block_);
    } else {  // inside an inner function
      BasicBlock* entry_block=&F->getEntryBlock();
      // if the last instruction is a branch,
      // insert the next instruction before it
      ::llvm::Instruction* pinst = &*(entry_block->rbegin());
      if(isa<BranchInst>(pinst)) {
        irb_->SetInsertPoint(pinst);
      } else {
        irb_->SetInsertPoint(entry_block);
      }
    }
    // Load from global variable
    value=LoadGlobalArtMethod();
    loaded_art_methods_[F]=value;

    irb_->SetInsertPoint(pinsert_point);
  }

  return value;
}

/**
 * @param postfix Use in FunctionHelper::InvokeVirtual.
 *        In the case of a speculation miss, we need the global of the unresolved
 *        ArtMethod. We then resolve it, and call it. Because the resolved ArtMethod
 *        can be different from the unresolved, we always go through the RT,
 *        regardless of whether the unresovled method was hot or not.
 *        Now there is a case that the global of the unresolved ArtMethod is used 
 *        elsewhere, as a direct or a speculative call.
 *        In order not to mess up with our globals and initializations, we will
 *        have an extra copy of that method with the "_MISS" postfix
 *        
 */
GlobalVariable* HGraphToLLVM::GetGlobalArtMethod(
    std::string pretty_method, HInvoke* invoke, bool define,
    std::string postfix) {
  std::string name = "art_method_" + postfix;
  // TODO cache this as well (with an std::map)
  return GetGlobalArtMethod(
      GetCallingMethodName(pretty_method, invoke, name), define);
}

GlobalVariable* HGraphToLLVM::GetGlobalArtMethod(
    std::string global_name, bool define) {
  return GetGlobalVariable(global_name, DataType::Type::kReference, define);
}

GlobalVariable* HGraphToLLVM::GetGlobalVariable(
    std::string global_name, DataType::Type type, bool define) {

  // TODO OPTIMIZE: put stuff in cur_block (with std::map)
  GlobalVariable* global_var = nullptr;
  if (art_method_globals_.find(global_name) == art_method_globals_.end()) {
    GlobalVariable::LinkageTypes linkage_type;
    if (define) {
      linkage_type = GlobalVariable::LinkOnceODRLinkage;
    } else {
      linkage_type = GlobalVariable::ExternalLinkage;
    }

    global_var = new GlobalVariable(*mod_, irb_->getType(type),
        false, linkage_type, nullptr, global_name);
    if (define) {
      global_var->setInitializer(irb_->getJZero(type));
    }

    art_method_globals_.insert(
        std::pair<std::string, GlobalVariable*>(
            global_name, global_var));
  } else {
    global_var = art_method_globals_[global_name];
  }

  D3LOG(INFO) << "GetGlobalVariable: " << global_var->getName().str();
  return global_var;
}

GlobalVariable* HGraphToLLVM::GetGlobalVariable(
    std::string global_name, Type* type, bool define) {
  // TODO OPTIMIZE: put stuff in cur_block (with std::map)
  GlobalVariable* global_var = nullptr;
  if (art_method_globals_.find(global_name) == art_method_globals_.end()) {
    GlobalVariable::LinkageTypes linkage_type;
    if (define) {
      linkage_type = GlobalVariable::LinkOnceODRLinkage;
    } else {
      linkage_type = GlobalVariable::ExternalLinkage;
    }

    global_var = new GlobalVariable(*mod_, type,
        false, linkage_type, nullptr, global_name);
    if (define) {
      global_var->setInitializer(irb_->getJNull(type));
    }

    art_method_globals_.insert(
        std::pair<std::string, GlobalVariable*>(
            global_name, global_var));
  } else {
    global_var = art_method_globals_[global_name];
  }

  D3LOG(INFO) << "GetGlobalVariable: " << global_var->getName().str();
  return global_var;
}

void HGraphToLLVM::CreateGlobalBootImageBegin() {
  std::string pretty_method = GetPrettyMethod();
  gbl_boot_image_begin_ = GetGlobalVariable("boot_image_begin", DataType::Type::kInt32, true);
}

void HGraphToLLVM::CreateGlobalArtMethod() {
  bool is_static = dcu_.IsStatic();
  std::string pretty_method = GetPrettyMethod();
  std::string gbl_name = GetCallingMethodName(
        pretty_method, GetInnerSignature(), is_static, "art_method_");
  gbl_art_method_ = GetGlobalArtMethod(gbl_name, true);
}

void HGraphToLLVM::AddToInitializedInnerMethods(Function* func) {
  initialized_inner_methods_.insert(func);
}

bool HGraphToLLVM::InitializedInnerMethod(Function* func) {
  return initialized_inner_methods_.find(func) != initialized_inner_methods_.end();
}

Argument* HGraphToLLVM::getArgument(HParameterValue* h) {
  Argument* value = args_[static_cast<uint32_t>(h->GetIndex())];
  CHECK(value != nullptr) << "Failed to find var: "
                          << GetTwine(h);
  return value;
}

Value* HGraphToLLVM::getValue(HInstruction* h) {
  if (h->IsPhi()) {
    return getPhi(h->AsPhi());
  } else {
    return getRegister(h);
  }
}

Value* HGraphToLLVM::getRegister(HInstruction* h) {
  Value* value = regs_[h];
  CHECK(value != nullptr) << "Failed to find reg: "
                          << GetTwine(h)
                          << "\nMethod: " << GetPrettyMethod();
  return value;
}

PHINode* HGraphToLLVM::getPhi(HPhi* hphi) {
  PHINode* lphi = phis_[hphi];
  CHECK(lphi != nullptr) << "Failed to find PHI: " << hphi->GetId();
  return lphi;
}

void HGraphToLLVM::AddToInitializedArtMethods(std::string method) {
  initialized_art_method_globals_.insert(method);
}

bool HGraphToLLVM::IsArtMethodInitialized(std::string method) {
  return initialized_art_method_globals_.find(method) !=
         initialized_art_method_globals_.end();
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
