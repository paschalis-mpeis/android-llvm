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
#ifndef ART_COMPILER_OPTIMIZING_HGRAPH_PRINTERS_H_
#define ART_COMPILER_OPTIMIZING_HGRAPH_PRINTERS_H_

#include <inttypes.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <android-base/stringprintf.h>
#include "base/logging.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_rt/mcr_rt.h"
#include "mcr_rt/utils.h"
#include "optimizing/nodes.h"
#include "optimizing/pretty_printer.h"

namespace art {

namespace LLVM {
class LLVMCompilationUnit;
}

//  Print HGraph to a file
class HGraphFilePrettyPrinter : public HPrettyPrinter {
 public:
  explicit HGraphFilePrettyPrinter(HGraph* graph)
      : HPrettyPrinter(graph),
        use_spaces_(true),
        tab_stop_(4) {
    ss_.str(std::string());
    hf_ = mcr::McrCC::PrettyMethod(graph);
  }

  void PrintInt(int value) override {
    ss_ << android::base::StringPrintf("%d", value);
  }

  void PrintInt64(int64_t value) {
    ss_ << android::base::StringPrintf("%" PRId64, value);
  }

  void PrintDouble(double value) {
    ss_ << android::base::StringPrintf("%lf", value);
  }

  void PrintStream(std::stringstream ss) {
    ss_ << ss.str();
  }

  void PrintString(std::string s, bool new_line = false) {
    ss_ << s;
    if (new_line) {
      ss_ << "\n";
    }
  }

  void PrintBoolean(bool value) {
    ss_ << (value ? "true" : "false");
  }

  void PrintString(const char* value) override {
    ss_ << value;
  }

  void PrintCharacter(char character, int num = 1) {
    ss_ << std::string(num, character);
  }

  void PrintSpace(int num = 1) {
    PrintCharacter(' ', num);
  }

  void OpAssignment(bool with_spaces = true) {
    if (with_spaces) PrintSpace();
    PrintString("=");
    if (with_spaces) PrintSpace();
  }

  void ParenthesisOpen() {
    PrintString("(");
  }

  void ParenthesisClose() {
    PrintString(")");
  }

  void PrintTab(int num = 1) {
    if (use_spaces_) {
      PrintCharacter(' ', tab_stop_ * num);
    } else {
      PrintCharacter('\t', num);
    }
  }

  void PrintNewLine(int num) {
    PrintCharacter('\n', num);
  }

  // Can't do it differently because we have to override
  void PrintNewLine() override {
    PrintCharacter('\n');
  }

  // Ported from HPrettyPrinter or StringPrettyPrinter
  std::string GetBasicBlock(HBasicBlock* block);
  std::string GetPreInstruction(HInstruction* instruction);
  std::string GetPostInstruction(HInstruction* instruction);
  std::string GetInstruction(HInstruction* instruction);

  virtual void PrintGraphInfo();

  void Clear() {
    ss_.str(std::string());
    ss_.clear();
  }

  std::string str() {
    GenerateOutput();
    std::string result(ss_.str());
    return result;
  }

  void Write(std::string filename_desc = "") {
    std::ofstream out = Open(filename_desc);
    GenerateOutput();
    out << ss_.rdbuf();
    out.close();
    chmod(filename_desc.c_str(), 0666);
  }

  void VisitBasicBlock(HBasicBlock* block) override;
  void VisitGoto(HGoto* gota) override;

 protected:
  std::string hf_;
  std::stringstream ss_;
  const char* filename_;
  bool use_spaces_;
  uint8_t tab_stop_;

  virtual std::string getFilenamePrefix() {
    return FILE_HGRAPH_PREFIX;
  }

  virtual std::string getFilenameExtension() {
    return FILE_HGRAPH_EXT;
  }

  virtual void GenerateOutput();

 private:
  std::ofstream Open(std::string desc);

  DISALLOW_COPY_AND_ASSIGN(HGraphFilePrettyPrinter);
};

// Get an HGraph instruction in std::string
class InstructionPrettyPrinter : public StringPrettyPrinter {
 public:
  explicit InstructionPrettyPrinter(HGraph* graph)
      : StringPrettyPrinter(graph) {
  }

  std::string toString(HInstruction* i) {
    Clear();
    StringPrettyPrinter::VisitInstruction(i);
    return StringPrettyPrinter::str();
  }
};

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_HGRAPH_FILE_PRINTERS_H_
