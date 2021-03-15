/**
 * Passes for simplifying some of the SuspendCheck cases. This, however, is
 * currently not in use. It may be conditionally enabed through McrDebug.
 * Then enabled a warning will be emited.
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

#include <llvm/IR/Module.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "ir_builder.h"
#include "llvm_compiler.h"
#include "llvm_info.h"
#include "llvm_utils.h"

#include "llvm_macros_irb.h"

using namespace ::llvm;

namespace art {
namespace LLVM {

void HGraphToLLVM::AnalyseSuspendChecks() {
  if (!McrDebug::SuspendCheckSimplify()) return;

  BasicBlock* startBB= irb_->GetInsertBlock();
  D2LOG(INFO) << "Analyzing: BB: " << Pretty(startBB);
  std::queue<BasicBlock*> q;
  std::set<BasicBlock*> discovered;
  discovered.insert(startBB);
  q.push(startBB);
  while(q.size()>0) {
    BasicBlock* BB = q.front(); q.pop();
    uint32_t predsz = pred_size(BB);
    D5LOG(INFO) << "VISITING: " << Pretty(BB) << ": Preds: " << predsz;
    if(predsz == 0) { break; }

    if(BB != startBB) {
      // iterate over instructions
      for (::llvm::Instruction &I : *BB) {
        D5LOG(INFO) << "Inst: " << Pretty(BB) << ": " << Pretty(I);

        if(isa<CallInst>(I)) {
          D4LOG(INFO) << "CALL: " << Pretty(I)
            << " FROM: " << Pretty(startBB);
          CallInst &callInst = cast<CallInst>(I);
          Function *F = callInst.getCalledFunction();
          if(F!=nullptr) {
            if(fh_->IsSuspendCheck(F)) { 
              D2LOG(WARNING) << "SuspendCheck: " << Pretty(startBB)
                << " and pred: " << Pretty(BB) ;
              sc_preds_.insert(std::pair<BasicBlock*, BasicBlock*>(
                    startBB, BB));
            }
          }
        }
      }
    }

    std::string added_preds="";
    for (BasicBlock *pred : predecessors(BB)) {
      if(discovered.find(pred)==discovered.end()) {
        D2LOG(INFO) << "DFS: add predeseccor: " << Pretty(pred);
        added_preds+=" "+Pretty(pred);
        discovered.insert(pred);
        q.push(pred);
      }
    }
    if(added_preds.size()>0) {
      D4LOG(INFO) << "Predecessors: " << added_preds;
    }
  }
}

void HGraphToLLVM::SuspendCheckSimplify() {
  if(McrDebug::SuspendCheckSimplify() && sc_preds_.size()>0) {
    D2LOG(INFO) << __func__;
    std::vector<BasicBlock*> to_remove;
    D2LOG(WARNING) << __func__ << " REPORT: " << sc_preds_.size();
    for (auto const& sc: sc_preds_) {
      D3LOG(INFO) << "SC: " << Pretty(sc.first)
        << " pred: " << Pretty(sc.second);

      // simple rule if appears both as key and value, then remove
      for (auto const& sc2: sc_preds_) {
        if(sc2.first == sc.first) continue;
        if(sc2.second == sc.first) {
          to_remove.push_back(sc.first);
        }
      }
    }

    for (BasicBlock* BB: to_remove) {
      DLOG(WARNING) << "Remove SC from: " << Pretty(BB);
      // iterate over instructions
      llvm::Instruction *toDelete=nullptr;
      for (::llvm::Instruction &I : *BB) {
        if(isa<CallInst>(I)) {
          CallInst &callInst = cast<CallInst>(I);
          Function *F = callInst.getCalledFunction();
          if(F!=nullptr) {
            if(fh_->IsSuspendCheck(F)) { 
              // BasicBlock* parentBB = I.getParent();
              toDelete=&I;
              break;
            }
          }
        }
      }
      if(toDelete == nullptr) {
        DLOG(ERROR) << __func__ << ": failed to find instruction";
      } else {
        toDelete->eraseFromParent();
      }
    }
  }
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace LLVM
