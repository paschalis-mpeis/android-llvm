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
#include "llvm/ir_builder.h"

#include <llvm/Support/raw_os_ostream.h>
#include "dex/dex_file.h"
#include "asm_arm64.h"
#include "function_helper.h"
#include "hgraph_to_llvm-inl.h"
#include "llvm_utils.h"
#include "llvm_macros_irbthis.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

Value* IRBuilder::StoreToOldShadowFrame(HGraphToLLVM* HL,
                                                Value* old_shadow_frame,
                                                Value* art_method) {
  // Store the method pointer
  HL->StoreToObjectOffset(old_shadow_frame,
                          ShadowFrame::MethodOffset(),
                          art_method);

  return old_shadow_frame;
}

Value* IRBuilder::PushShadowFrame(HGraphToLLVM* HL,
                                          Value* new_shadow_frame,
                                          Value* current_thread,
                                          Value* art_method,
                                          uint32_t num_vregs) {
  DIE << "Shouldn't be creating any ShadowFrames";
  if (McrDebug::DebugInvokeJni()) {
    AndroidLogPrint(INFO, __func__);
  }

  // Load old shadow frame
  Value* old_shadow_frame =
      HL->LoadFromObjectOffset(current_thread,
          HL->GetThreadTopShadowFrameOffset(),
          GetShadowFrameTy()->getPointerTo());
  old_shadow_frame->setName("old_sframe");

  // Store new shadow frame
  HL->StoreToObjectOffset(current_thread,
      HL->GetThreadTopShadowFrameOffset(),
      new_shadow_frame);

  AndroidLogPrint(INFO, "llvm:method");
  HL->ArtCallVerifyArtMethod(art_method);

  // Store the method pointer
  HL->StoreToObjectOffset(new_shadow_frame,
                          ShadowFrame::MethodOffset(),
                          art_method);

  // CHECK skipping those 2
  // Store the number of vregs
  HL->StoreToObjectOffset(new_shadow_frame,
                          ShadowFrame::NumberOfVRegsOffset(),
                          getInt32(num_vregs));

  // // Store the dex pc
  HL->StoreToObjectOffset(new_shadow_frame,
                          ShadowFrame::DexPCOffset(),
                          getInt32(dex::kDexNoIndex));

  // Store the link to previous shadow frame
  HL->StoreToObjectOffset(new_shadow_frame,
                          ShadowFrame::LinkOffset(),
                          old_shadow_frame);

  return old_shadow_frame;
}

void IRBuilder::PopShadowFrame(HGraphToLLVM* HL,
                               Value* current_thread,
                               Value* old_shadow_frame) {
  HL->StoreToObjectOffset(current_thread,
      HL->GetThreadTopShadowFrameOffset(),
      old_shadow_frame);
}

StructType* IRBuilder::getShadowFrameTy(uint32_t vreg_size) {
  std::string name(StringPrintf("ShadowFrame%u", vreg_size));

  // Try to find the existing struct type definition
  if (Type* type = module_->getTypeByName(name)) {
    CHECK(isa<StructType>(type));
    return static_cast<StructType*>(type);
  }

  // Create new struct type definition
  Type* elem_types[] = {
    structShadowFrame_,
    ArrayType::get(getInt32Ty(), vreg_size),
  };

  return StructType::create(elem_types, name);
}

void IRBuilder::CallStoreReturnValue(Value* jvalue,
                                     Value* ret_val,
                                     DataType::Type ret_type) {
  D3LOG(INFO) << "CallStoreReturnValue: " << ret_type;

  Type* ltype = getType(ret_type)->getPointerTo();
  Value* vptr= CreateBitCast(jvalue, ltype);

#ifdef CRDEBUG4
  AndroidLogPrintValue(WARNING, "Store: ", ret_val, ret_type);
#endif
  CreateStore(ret_val, vptr, false);
}

Value* IRBuilder::CallGetReturnValue(HGraphToLLVM* HL,
                                             Value* jvalue,
                                             DataType::Type ret_type) {
  Value* llvm_val = nullptr;
  Type* ltype = getType(ret_type)->getPointerTo();
  Value* vptr= CreateBitCast(jvalue, ltype);
  llvm_val = CreateLoad(vptr);
#ifdef CRDEBUG4
  AndroidLogPrintValue(ERROR, "JValue: fromLLVM", llvm_val, ret_type);
#endif
  return llvm_val;
}

bool IRBuilder::IsSigned(DataType::Type type) {
  return !DataType::IsUnsignedType(type);
}

Value* IRBuilder::mCreateAdd(bool is_fp, Value* lhs, Value* rhs) {
  if (is_fp) {
    return CreateFAdd(lhs, rhs);
  } else {
    return CreateAdd(lhs, rhs, "", false, true);
  }
}

Value* IRBuilder::mCreateSub(bool is_fp,
                                     Value* lhs, Value* rhs) {
  if (is_fp) {
    return CreateFSub(lhs, rhs);
  } else {
    return CreateSub(lhs, rhs, "", false, true);
  }
}

Value* IRBuilder::mCreateMul(bool is_fp,
                                     Value* lhs, Value* rhs) {
  if (is_fp) {
    return CreateFMul(lhs, rhs);
  } else {
    return CreateMul(lhs, rhs, "", false, true);
  }
}

Value* IRBuilder::mCreateDiv(bool is_fp,
                                     Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating fdiv: FPMathTag is null";
    return CreateFDiv(lhs, rhs);
  } else {
    return CreateSDiv(lhs, rhs);
  }
}

Value* IRBuilder::mCreateRem(bool is_fp, bool is_signed,
                                     Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating rem: FPMathTag is null";
    return CreateFRem(lhs, rhs);
  } else {
    if (is_signed) {
      return CreateSRem(lhs, rhs);
    } else {
    }
    return CreateURem(lhs, rhs);
  }
}

Value* IRBuilder::mCreateNeg(bool is_fp, bool is_signed,
    Value* v) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating neg: FPMathTag is null";
    return CreateFNeg(v);
  } else {
    // chars (unsigned i16 are handled on the upper level)
    // Unsigned neg is handled 
    return CreateNeg(v);
  }
}

Value* IRBuilder::mCreateCmpEQ(bool is_fp,
                                       Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating fcmp: using ordered: neither operand a QNAN";
    return CreateFCmpOEQ(lhs, rhs);
  } else {
    return CreateICmpEQ(lhs, rhs);
  }
}

Value* IRBuilder::CreateCmpIsNull(Value* v) {
  Value* cmp = mCreateCmpEQ(false, v, getJNull());
  return cmp;
}

Value* IRBuilder::mCreateCmpNE(bool is_fp,
                                       Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating fcmp: using ordered: neither operand a QNAN";
    return CreateFCmpONE(lhs, rhs);
  } else {
    return CreateICmpNE(lhs, rhs);
  }
}

Value* IRBuilder::mCreateCmpGT(bool is_fp, bool is_signed,
                                       Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating fcmp: using ordered: neither operand a QNAN";
    return CreateFCmpOGT(lhs, rhs);
  } else {
    if (is_signed) {
      return CreateICmpSGT(lhs, rhs);
    } else {
      return CreateICmpUGT(lhs, rhs);
    }
  }
}

Value* IRBuilder::mCreateCmpGE(bool is_fp, bool is_signed,
                                       Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating fcmp: using ordered: neither operand a QNAN";
    return CreateFCmpOGE(lhs, rhs);
  } else {
    if (is_signed) {
      return CreateICmpSGE(lhs, rhs);
    } else {
      return CreateICmpUGE(lhs, rhs);
    }
  }
}

Value* IRBuilder::mCreateCmpLT(bool is_fp, bool is_signed,
                                       Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating fcmp: using ordered: neither operand a QNAN";
    return CreateFCmpOLT(lhs, rhs);
  } else {
    if (is_signed) {
      return CreateICmpSLT(lhs, rhs);
    } else {
      return CreateICmpULT(lhs, rhs);
    }
  }
}

Value* IRBuilder::mCreateCmpLE(bool is_fp, bool is_signed,
                                       Value* lhs, Value* rhs) {
  if (is_fp) {
    D4LOG(WARNING) << "Generating fcmp: using ordered: neither operand a QNAN";
    return CreateFCmpOLE(lhs, rhs);
  } else {
    if (is_signed) {
      return CreateICmpSLE(lhs, rhs);
    } else {
      return CreateICmpULE(lhs, rhs);
    }
  }
}

void IRBuilder::mCreateStore(Value* src, Value* ptr,
                             bool is_volatile) {
  Type* srcTy = src->getType();
  Type* ptrTy = ptr->getType();
  Type* dest = ptrTy->getPointerElementType();
  unsigned int src_bits = srcTy->getPrimitiveSizeInBits();
  unsigned int dest_bits = dest->getPrimitiveSizeInBits();
  bool bits_dont_match = (src_bits != dest_bits);
  if (bits_dont_match) {
    std::string ssrc, sptr, tmp;
    raw_string_ostream OS(tmp);
    OS << *src;
    ssrc = OS.str();
    tmp.clear();
    OS << *ptr;
    sptr = OS.str();
    DLOG(FATAL) << "mCreateStore: src_bits: " << std::to_string(src_bits)
               << " dest_bits: " << std::to_string(dest_bits)
               << "\nsrc: " << ssrc
               << "\nptr: " << sptr;
  }
  CreateStore(src, ptr, is_volatile);
}

Value* IRBuilder::AndroidLogSeverity(LogSeverity log_severity) {
  return getJInt(static_cast<int>(log_severity));
}

void IRBuilder::AndroidLogPrint(LogSeverity log_severity, std::string msg) {
  std::vector<Value*> prt_args;
  Value* fmt = mCreateGlobalStringPtr(msg);
  prt_args.push_back(AndroidLogSeverity(log_severity));
  prt_args.push_back(fmt);
  CreateCall(fh_->AndroidLog(), prt_args);
}

inline void _AndroidLogPrintLlvmValue(
    IRBuilder* irb,
    FunctionHelper *FH,
    LogSeverity log_severity,
    std::string msg,
    std::string valfmt,
   Value* value) {
  msg += ": "+ valfmt + "\n";
  std::vector<Value*> prt_args;
  Value* fmt = irb->mCreateGlobalStringPtr(msg);
  prt_args.push_back(irb->AndroidLogSeverity(log_severity));
  prt_args.push_back(fmt);
  prt_args.push_back(value);
  irb->CreateCall(FH->AndroidLog(), prt_args);
}

void IRBuilder::AndroidLogPrintChar(
    LogSeverity log_severity, std::string msg, Value* value) {
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "%c", value);
}

void IRBuilder::AndroidLogPrintHexAndInt(
    LogSeverity log_severity, std::string msg, Value* value) {
  msg +=": 0x%lx (%d)\n";
  std::vector<Value*> prt_args;
  Value* fmt = mCreateGlobalStringPtr(msg);
  prt_args.push_back(AndroidLogSeverity(log_severity));
  prt_args.push_back(fmt);
  prt_args.push_back(value);
  prt_args.push_back(value);
  CreateCall(fh_->AndroidLog(), prt_args);
}

void IRBuilder::AndroidLogPrintHex(
    LogSeverity log_severity, std::string msg, Value* value) {
  // INFO %p is pointer
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "0x%lx", value);
}

void IRBuilder::AndroidLogPrintFloat(
    LogSeverity log_severity, std::string msg, Value* value) {
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "%f", value);
}

void IRBuilder::AndroidLogPrintDouble(
    LogSeverity log_severity, std::string msg, Value* value) {
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "%lf", value);
}

void IRBuilder::AndroidLogPrintInt(
    LogSeverity log_severity, std::string msg, Value* value) {
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "%d", value);
}

void IRBuilder::AndroidLogPrintUint(
    LogSeverity log_severity, std::string msg, Value* value) {
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "%u", value);
}
void IRBuilder::AndroidLogPrintLong(
    LogSeverity log_severity, std::string msg, Value* value) {
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "%ld", value);
}

void IRBuilder::AndroidLogPrintUlong(
    LogSeverity log_severity, std::string msg, Value* value) {
  _AndroidLogPrintLlvmValue(this, fh_, log_severity, msg, "%lu", value);
}

void IRBuilder::AndroidLogPrintValue(
    LogSeverity svrt, std::string msg, Value* value, DataType::Type type) {

  switch (type) {
    case DataType::Type::kBool:
    case DataType::Type::kUint8:
    case DataType::Type::kUint32:
      AndroidLogPrintUint(svrt, msg , value);
      break;
    case DataType::Type::kUint16:
      AndroidLogPrintChar(svrt, msg , value);
      break;
    case DataType::Type::kInt8:
    case DataType::Type::kInt16:
    case DataType::Type::kInt32:
      AndroidLogPrintInt(svrt, msg , value);
      break;
    case DataType::Type::kInt64:
      AndroidLogPrintLong(svrt, msg , value);
      break;
    case DataType::Type::kUint64:
      AndroidLogPrintUlong(svrt, msg , value);
      break;
    case DataType::Type::kFloat32:
      AndroidLogPrintFloat(svrt, msg , value);
      break;
    case DataType::Type::kFloat64:
      AndroidLogPrintDouble(svrt, msg , value);
      break;
    case DataType::Type::kReference:
      AndroidLogPrintHex(svrt, msg, value);
      break;
    case DataType::Type::kVoid:
      msg+=" void";
      AndroidLogPrint(svrt, msg);
  }
}


Value* IRBuilder::mCreateGlobalStringPtr(std::string str) {
  if (gbl_strings_.find(str) != gbl_strings_.end()) {
    return gbl_strings_[str];
  }

  Value* newstr = CreateGlobalStringPtr(str, "str");
  gbl_strings_.insert(
      std::pair<std::string, Value*>(
          str, newstr));

  return newstr;
}

inline std::string ClobberFlagsReg(std::string constraints, bool clobberCC) {
  if(clobberCC) {
    std::string clobber_cc="~{cc},~{memory}";
    if(constraints.size() > 0) {
      clobber_cc=","+clobber_cc;
    }
    constraints+=clobber_cc;
  }
  return constraints;
}

Value* IRBuilder::CallInlineAsm(FunctionType* fty, std::string cmd,
    std::string constraints, bool clobberCC, bool hasSideEffects) {

  constraints=ClobberFlagsReg(constraints, clobberCC);
  InlineAsm* ia = InlineAsm::get(fty, cmd, constraints, hasSideEffects);
  return CreateCall(ia);
}

Value* IRBuilder::CallInlineAsm(FunctionType* fty, std::vector<Value*> args,
    std::string cmd, std::string constraints,
    bool clobberCC, bool hasSideEffects) {

  constraints=ClobberFlagsReg(constraints, clobberCC);
  InlineAsm* ia = InlineAsm::get(fty, cmd, constraints, hasSideEffects);
  return CreateCall(ia, args);
}

Value*	IRBuilder::CreateFastCall(Function* F, std::vector<Value*> args) {
  return CreateCall(F, args);
}

Value*	IRBuilder::CreateFastCall(Function* F) {
  return CreateCall(F);
}


#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
