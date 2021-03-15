/**
 * Some pretty printers for HGraph components.
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
#include "hgraph_printers.h"

namespace art {

std::string HGraphFilePrettyPrinter::GetBasicBlock(HBasicBlock* block) {
  std::string result = "";
  result += "BB ";
  result += std::to_string(block->GetBlockId());
  ArrayRef<HBasicBlock* const> predecessors(block->GetPredecessors());

  if (!predecessors.empty()) {
    result += ", pred: ";

    for (HBasicBlock* predecessor : predecessors) {
      result += std::to_string(predecessor->GetBlockId());
      result += ", ";
    }
    // remove last comma
    result = result.substr(0, result.size()-2);
  }
  ArrayRef<HBasicBlock* const> successors(block->GetPredecessors());
  if (!successors.empty()) {
    result += ", succ: ";

    for (HBasicBlock* successor: successors) {
      result += std::to_string(successor->GetBlockId());
      result += ", ";
    }
    // remove last comma
    result = result.substr(0, result.size()-2);
  }

  return result;
}

std::string HGraphFilePrettyPrinter::GetPreInstruction(HInstruction* instruction) {
  std::string result = "";
  result += "  ";
  result += std::to_string(instruction->GetId());
  result += ": ";
  return result;
}

std::string HGraphFilePrettyPrinter::GetPostInstruction(HInstruction* instruction) {
  std::string result = "";
  if (instruction->InputCount() != 0) {
    result += "(";
    bool first = true;
    HConstInputsRef inputs = instruction->GetInputs();

    for (const HInstruction* input : inputs) {
      if (first) {
        first = false;
      } else {
        result += ", ";
      }
      result += std::to_string(input->GetId());
    }
    result += ")";
  }
  if (instruction->HasUses()) {
    result += " [";
    bool first = true;
    for (const HUseListNode<HInstruction*>& use : instruction->GetUses()) {
      if (first) {
        first = false;
      } else {
        result += ", ";
      }
      result += std::to_string(use.GetUser()->GetId());
    }
    result += "]";
  }
  return result;
}

std::string HGraphFilePrettyPrinter::GetInstruction(HInstruction* h) {
  std::string result = "";
  result += GetPreInstruction(h);
  result += h->DebugName();
  result += GetPostInstruction(h);
  return result;
}

void HGraphFilePrettyPrinter::VisitBasicBlock(HBasicBlock* block) {
  HPrettyPrinter::VisitBasicBlock(block);
}

void HGraphFilePrettyPrinter::VisitGoto(HGoto* gota) {
  PrintString("  ");
  PrintInt(gota->GetId());
  PrintString(": Goto ");
  PrintInt(gota->GetBlock()->GetSuccessors()[0]->GetBlockId());
  PrintNewLine();
}

void HGraphFilePrettyPrinter::PrintGraphInfo() {
  const bool NL = true;
  PrintString("Method: " + hf_, NL);
  PrintString("Blocks: " + std::to_string(GetGraph()->GetBlocks().size()), NL);
  PrintString("VRegs: ");
  PrintInt(GetGraph()->GetNumberOfVRegs());
  PrintNewLine();
  PrintString("Local VRegs: ");
  PrintInt(GetGraph()->GetNumberOfLocalVRegs());
  PrintNewLine();
  PrintNewLine();
}

std::ofstream HGraphFilePrettyPrinter::Open(std::string desc) {
  umask(0);
  std::ofstream out;
  std::string filename = getFilenamePrefix();

  if (desc.size() != 0) {
    filename += "." + desc;
  }

  filename += "." + getFilenameExtension();

  std::string fullpath = mcr::GetFileSrc(hf_, filename);
  D3LOG(INFO) << __func__ << ": open: " << fullpath;
  out.open(fullpath, std::fstream::trunc);

  return out;
}

void HGraphFilePrettyPrinter::GenerateOutput() {
  Clear();
  PrintGraphInfo();
  VisitInsertionOrder();
}

}  // namespace art
