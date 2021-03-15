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
#include "asm_arm64.h"

#include <llvm/IR/DerivedTypes.h>
#include "asm_arm64.h"
#include "llvm_macros_irb.h"
#include "gc/accounting/card_table.h"
#include "ir_builder.h"
#include "mcr_rt/art_impl_arch_arm-inl.h"

using namespace ::llvm;

#define _IMM1 << std::to_string(imm1) <<
#define _IMM2 << std::to_string(imm2) <<
#define _IMM3 << std::to_string(imm3) <<
#define IMM3 << std::to_string(imm3)
#define _IMM4 << std::to_string(imm4) <<
#define _IMM5 << std::to_string(imm5) <<
#define _TR LLVM_THREAD_REG_ARM64
#define _OFST_ << std::to_string(offset) <<
#define _OFST << std::to_string(offset)
#define REG(regid) __reg((regid), (false))
#define REGw(regid) __reg((regid), (true))

namespace art {
namespace LLVM {
namespace Arm64 {

uint32_t offset(QuickEntrypointEnum qpoint) {
  return GetThreadOffset<kArm64PointerSize>(qpoint).Int32Value();
}

ALWAYS_INLINE std::string __reg(uint32_t regid, bool i32) {
  std::string sreg = std::to_string(regid);
  if(i32) {
    return "${" + sreg+ ":w}";
  } else {
    return "$" + sreg+ "";
  }
}

ALWAYS_INLINE std::string __freg(uint32_t regid, bool i32) {
  std::string sreg = std::to_string(regid);
  if(i32) {
    return "${" + sreg+ ":s}";
  } else {
    return "${" + sreg+ ":d}";
  }
}

Value* GetQuickEntrypoint(IRBuilder* irb, QuickEntrypointEnum qpoint) {
  int32_t offset = Arm64::offset(qpoint);
  std::stringstream ss;
  ss << "ldr $0, [" _TR ", #" _OFST_ "];";

  FunctionType* ty = FunctionType::get(
      irb->getJLongTy()->getPointerTo(), false);
  return irb->CallInlineAsm(ty, ss.str(), "=r", false, false);
}

/**
 * @brief Actually since using aliases for clobbers,
 *        this is architectural independent.
 */
void ClobberLR(IRBuilder* irb) {
  FunctionType* ty = FunctionType::get(irb->getJVoidTy(), false);
  irb->CallInlineAsm(ty, "", "~{LR},~{memory}", false, false);
}

/**
 * @brief  INFO duplicated code from __Mov, as I want to eliminate 
 *         all custom ASM, except anything related to TR
 */
Value* GetThreadRegister(IRBuilder* irb) {
  VERIFIED_;
  FunctionType *fTy = FunctionType::get(irb->getVoidPointerType(), false);

  std::string s = "mov " + REG(0) + ", " + _TR;
  std::vector<Attribute> attrs{Attribute::ReadNone};
  InlineAsm* ia = InlineAsm::get(fTy, s, "=r", false);
  return irb->CreateCall(ia);
}

void SetThreadRegister(IRBuilder* irb, Value* thread_self) {
  VERIFIED_;
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType *ty =
    FunctionType::get(irb->getVoidTy(), params, false);
  std::vector<Value*> args{thread_self};

  std::stringstream ss;
  ss << "mov " << _TR << ", $0";
  irb->CallInlineAsm(ty, args, ss.str(), "r");
}

void __Mov(IRBuilder* irb, std::string to, Value* from) {
  AVOID_ASM;
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType *ty =
    FunctionType::get(irb->getVoidTy(), params, false);
  std::vector<Value*> args{from};

  std::string s = "mov " + to +", $0";
  irb->CallInlineAsm(ty, args, s, "r");
}

Value* __Mov(IRBuilder* irb,  std::string from, bool w) {
  AVOID_ASM;
  FunctionType *ty =
    FunctionType::get(irb->getVoidPointerType(), false);
  std::string s = "mov " + __reg(0, w) + ", " + from;
  return irb->CallInlineAsm(ty, s, "=r");
}

/**
 * @brief Stores a float to an int (converts float -> int)
 */
Value* __Fmov(IRBuilder* irb, Value* arg1, bool i32) {
  // AVOID_ASM; // CreateFPToSI
  Type *paramTy= i32?irb->getFloatTy():irb->getDoubleTy();
  Type *retTy = i32?irb->getJIntTy():irb->getJLongTy();

  std::vector<Type*> params{paramTy};
  FunctionType *ty = FunctionType::get(retTy, params, false);
  std::vector<Value*> args{arg1};
  std::string s = "fmov "+ __reg(0, i32) +", " + __freg(1, i32);
  return irb->CallInlineAsm(ty, args, s, "=r,r");
}

/**
 * @brief Stores an int to a float (converts int -> float)
 */
Value* __Fmov_toFP(IRBuilder* irb, Value* arg1, bool i32) {
  // AVOID_ASM; // CreateFPToSI
  Type *retTy = i32?irb->getFloatTy():irb->getDoubleTy();
  Type *paramTy = i32?irb->getJIntTy():irb->getJLongTy();

  std::vector<Type*> params{paramTy};
  FunctionType *ty = FunctionType::get(retTy, params, false);
  std::vector<Value*> args {arg1};
  std::string s = "fmov "+ __freg(0, i32) +", " + __reg(1, i32);
  return irb->CallInlineAsm(ty, args, s, "=r,r");
}

Value* __Neg(IRBuilder* irb, Value* arg1, bool w) {
  // AVOID_ASM;
  // reg = -reg.
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType *ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);
  std::vector<Value*> args{arg1};
  std::string s = "neg " + __reg(0, w) + ", " + __reg(1, w);
  return irb->CallInlineAsm(ty, args, s, "=r,r");
}

Value* __wNeg(IRBuilder* irb, Value* lreg) {
  // CHECK use mCreateNeg
  AVOID_ASM;
  return __Neg(irb,lreg,true);
}

Value* __Ldarb(IRBuilder* irb, Value* arg1) {
  std::vector<Type*> params{irb->getVoidPointerType()};
  // returns int
  FunctionType *ty =FunctionType::get(irb->getJIntTy(), params, false);
  std::vector<Value*> args{arg1};
  // ldarb w16, [x16]
  std::string s = "ldarb "+ REGw(0) +", [" + REG(1) + "]";
  return irb->CallInlineAsm(ty, args, s, "=r,r", false, true);
}

// identical with Ldarb
Value* __Ldarh(IRBuilder* irb, Value* arg1) {
  std::vector<Type*> params{irb->getVoidPointerType()};
  // returns int
  FunctionType *ty =FunctionType::get(irb->getJIntTy(), params, false);
  std::vector<Value*> args{arg1};
  // ldarh w16, [x16]
  std::string s = "ldarh "+ REGw(0) +", [" + REG(1) + "]";
  return irb->CallInlineAsm(ty, args, s, "=r,r", false, true);
}

Value* __Ldar(IRBuilder* irb, Value* arg1, DataType::Type type) {
  std::vector<Type*> params{irb->getVoidPointerType()};

  Type *pTy=nullptr;
  bool i32=true;
  switch(type) {
    case DataType::Type::kInt32:
    case DataType::Type::kUint32:
      pTy=irb->getJIntTy();
      break;
    case DataType::Type::kInt64:
    case DataType::Type::kUint64:
      pTy=irb->getJLongTy();
      i32=false;
      break;
    case DataType::Type::kReference:
      pTy=irb->getVoidPointerType();
      break;
    default:
      DLOG(FATAL) << __func__ << ": wrong type: " << type;
  }

  FunctionType *ty =FunctionType::get(pTy, params, false);
  std::vector<Value*> args{arg1};
  // ldar x/w16, [x16]
  std::string s = "ldar "+ __reg(0,i32) +", [" + REG(1) + "]";
  return irb->CallInlineAsm(ty, args, s, "=r,r", false, true);
}

Value* __Sbfx(IRBuilder* irb, bool i32, Value* arg1, Value* arg2,
    uint32_t imm1, uint32_t imm2) {
  std::vector<Type*> params{2, irb->getVoidPointerType()};
  // returns int
  FunctionType *ty =FunctionType::get(irb->getJIntTy(), params, false);
  std::vector<Value*> args{arg1, arg2};
  // SBFX  R0, R0, #0, #<val>
  std::stringstream ss;
  ss<<"sbfx "<<__reg(0,i32)<<", "<<__reg(1,i32)<<" #" _IMM1 ", #" _IMM2 "";
  return irb->CallInlineAsm(ty, args, ss.str(), "=r,r", false, true);
}

Value* IsClassInited(IRBuilder* irb, Value* klass, uint32_t offset) {
  DIE_ASM_REPLACED;
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType *ty =
    FunctionType::get(irb->getJIntTy(), params, false);
  std::vector<Value*> args{klass};
  std::stringstream ss;

  // add w16, w0, #0x73 (115)
  // ldarb w16, [x16]
  ss
    << "add w16, "<<REGw(1)<<", #" <<offset << ";\n"
    << "ldarb "+ REGw(0) +", [x16]";

  return irb->CallInlineAsm(ty, args, ss.str(),
      "=r,r,~{x16},~{cc},~{memory}", false, true);
}

/**
 * @brief  Quick uses often StoreRelease instrunctions:
 *         stlrb, stlrh, stlr, like this:
 *         <INSTR> WSRC, [xDEST]
 *         They use xBASE, which is DEST + zero offset
 *        
 *         Actual examples:
 *            stlrb   w0, [x0]
 *            ; stlrh: haven't found yet.
 *            stlr w28, [x16]
 *
 *         Offset: assuming offset is always zero here.
 *                 if offset needed then add extra parameter.
 *
 * @param irb
 * @param instr: stlrb, stlrh, and stlr
 * @param src
 * @param base
 */
inline void ___STLR(IRBuilder* irb, std::string instr,
    Value *src, Value* base,
    std::vector<Type*> tp_params, bool i32=true) {
  _LOGLLVM4(irb, INFO, __func__ + instr);

  FunctionType *ty =FunctionType::get(irb->getJVoidTy(), tp_params, false);
  std::vector<Value*> args{src, base};
  std::string s = instr+ " "+ __reg(0,i32) +", [" +REG(1)+ "]";

  irb->CallInlineAsm(ty, args, s, "r,r");
}

void __Stlrb(IRBuilder* irb, Value *src, Value* base) {
  std::vector<Type*> params{
    irb->getJIntTy(),
    irb->getVoidPointerType()};
  ___STLR(irb, "stlrb", src, base, params);
}

void __Stlrh(IRBuilder* irb, Value *src, Value* base) {
std::vector<Type*> params{
    irb->getJIntTy(),
    irb->getVoidPointerType()};
  ___STLR(irb, "stlrh", src, base, params);
}

void __Stlr(IRBuilder* irb, Value *src, Value* base, bool i32, bool isFP) {
  Type* tp1;
  if(i32) {
    if(isFP) {
      tp1= irb->getJFloatTy();
    } else {
      tp1= irb->getJIntTy();
    }
  } else {
    if(isFP) {
      tp1= irb->getJDoubleTy();
    } else {
      tp1= irb->getJLongTy();
    }
  }
  std::vector<Type*> params{
    tp1, irb->getVoidPointerType()};
  ___STLR(irb, "stlr", src, base, params, i32);
}

Value* __Add(IRBuilder* irb, Value* lreg1, uint32_t offset, bool i32) {
  AVOID_ASM; // used in StoreRelease
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType *ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);
  std::vector<Value*> args{lreg1};
  std::string s = "add " + __reg(0,i32) + ", "+ __reg(1, i32)+", #" + std::to_string(offset);

  return irb->CallInlineAsm(ty, args, s, "=r,r", false, true);
}

Value* __Add(IRBuilder* irb, Value* lreg1, Value* lreg2, 
    std::vector<Type*> tp_params, bool i32) { 
  FunctionType *ty =
    FunctionType::get(irb->getVoidPointerType(), tp_params, false);
  std::vector<Value*> args{lreg1, lreg2};
  std::string s = "add " + __reg(0,i32) + ", "+ __reg(1, i32)+", " + __reg(2, i32);

  return irb->CallInlineAsm(ty, args, s, "=r,r,r", false, false);
}

Value* Ldrh(IRBuilder* irb, std::string xreg, uint32_t offset) {
  std::stringstream ss;
  ss << "ldrh ${0:w}, [" << xreg << ", #"
    << std::to_string(offset) << "]";

  FunctionType *ty = FunctionType::get(irb->getInt16Ty(), false);
  return irb->CallInlineAsm(ty, ss.str(), "=r", false, false);
}

/**
 * @brief Used by MarkGCCard (OK as it uses TR)
 */
Value* Ldr(IRBuilder* irb, std::string xreg, uint32_t offset) {
  std::stringstream ss;
  ss  << "ldr $0, [" << xreg << ", #" _OFST_ "]";

  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), false);

  return irb->CallInlineAsm(ty, ss.str(), "=r", false, true);
}

Value* wLdr(IRBuilder* irb, std::string xreg, uint32_t offset) {
  AVOID_ASM;
  std::stringstream ss;
  ss  << "ldr ${0:w}, [" << xreg << ", #" _OFST_ "]";

  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), false);

  return irb->CallInlineAsm(ty, ss.str(), "=r");
}

Value* wLdr(IRBuilder* irb, Value* arg1, uint32_t offset) {
  return __Ldr(irb, arg1, offset, true);
}

Value* wLdr(IRBuilder* irb, Value* arg1, Value* argofst) {
  return __Ldr(irb, arg1, argofst, true);
}

Value* __Ldr(IRBuilder* irb, Value* arg1, Value* argofst, bool i32) {
  std::stringstream ss;
  ss  << "ldr "+__reg(0,i32) +", ["+REG(1)+", " +REG(2)+ "]";

  std::vector<Value*> args{arg1, argofst};
  std::vector<Type*> params{irb->getVoidPointerType(),
    irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);

  return irb->CallInlineAsm(ty, args, ss.str(), "=r,r,r");
}

Value* __Ldr(IRBuilder* irb, Value* arg1, uint32_t offset, bool i32) {
  std::stringstream ss;

  ss  << "ldr "+__reg(0, i32) +", ["+REG(1)+", #" _OFST_ "]";
  std::vector<Value*> args{arg1};
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);

  return irb->CallInlineAsm(ty, args, ss.str(), "=r,r");
}

inline Value* __Ldr(IRBuilder* irb, Value* arg1, Type* retTy, bool i32) {
  std::stringstream ss;
  ss  << "ldr "+__reg(0,i32) +", ["+REG(1) + "]";

  Type *paramTy= i32?
    paramTy=irb->getJIntTy()->getPointerTo():
    paramTy=irb->getJLongTy()->getPointerTo();

  std::vector<Value*> args = {arg1};
  std::vector<Type*> params{paramTy};
  FunctionType* ty = FunctionType::get(retTy, params, false);

  return irb->CallInlineAsm(ty, args, ss.str(), "=r,r");
}

Value* Ldr(IRBuilder* irb, Value* arg1, bool i32) {
  Type* retTy = i32?
    retTy=irb->getJIntTy():retTy=irb->getJLongTy();
  return __Ldr(irb, arg1, retTy, i32);

}

Value* sLdr(IRBuilder* irb, Value* arg1, bool i32) {
  AVOID_ASM;
  // ldr s0, pc+24 (addr 0x24e848) (234.343)
  std::stringstream ss;
  ss  << "ldr "+__freg(0,i32) +", ["+REG(1) + "]";

  Type* retTy = nullptr, *paramTy=nullptr;
  if(i32) {
    retTy=irb->getFloatTy();
    paramTy=irb->getJIntTy()->getPointerTo();
  } else {
    retTy=irb->getJDoubleTy();
    paramTy=irb->getJLongTy()->getPointerTo();
  }

  std::vector<Value*> args{arg1};
  std::vector<Type*> params{paramTy};
  FunctionType* ty = FunctionType::get(retTy, params, false);

  return irb->CallInlineAsm(ty, args, ss.str(), "=r,r");
}

void wStrb(IRBuilder* irb, Value* arg1, Value* arg2, Value* arg3) {
  TODO_LLVMD4("implement with LLVM");
  // AVOID_ASM;
  std::stringstream ss;
  // strb	w24, [x8,#3456]
  // we are doing this example:
  // strb	w2, [x3, x4]
  ss  << "strb ${0:w}, [$1, $2]";
  std::vector<Value*> args{arg1, arg2, arg3};
  std::vector<Type*> params{
    irb->getVoidPointerType(),
    irb->getVoidPointerType(),
    irb->getVoidPointerType()
  };

  FunctionType* ty = FunctionType::get(irb->getVoidTy(), params, false);
  irb->CallInlineAsm(ty, args, ss.str(), "r,r,r,~{memory}", false, true);
}

Value* wLsr(IRBuilder* irb, Value* arg1, uint32_t offset) {
  TODO_LLVMD4("implement with LLVM");
  std::stringstream ss;
  // lsr w17, w0, #10
  ss  << "lsr ${0:w}, ${1:w}, #" _OFST;

  std::vector<Value*> args{arg1};
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);

  return irb->CallInlineAsm(ty, args, ss.str(), "=r,r,~{memory}", false, true);
}

void Mov(IRBuilder* irb, std::string regname, Value* llvmreg) {
  AVOID_ASM;
  __Mov(irb, regname, llvmreg);
}

void MovConst(IRBuilder* irb,  std::string regname, uint32_t const_val) {
  FunctionType *ty = FunctionType::get(irb->getJVoidTy(), false);
  std::stringstream ss;
  DLOG(WARNING) << "VERIFY_LLVM: StoreHalfWord";

  ss << "mov "<< regname << ", #"<< std::to_string(const_val);
  irb->CallInlineAsm(ty, ss.str(), "");
}

void _voidZeroArgsDowncall(IRBuilder* irb, int32_t offset) {
  std::stringstream ss;
  ss
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n";

  std::string clobbers="~{x29}";
  FunctionType* ty = FunctionType::get(irb->getJVoidTy(), false);
  irb->DownCallInlineAsm(ty, ss.str(), ""+clobbers);
}

void _voidOneArgDowncall(IRBuilder* irb, int32_t offset, Value* arg1) {
  std::stringstream ss;
  ss
    << "mov x0, $0;\n" // arg1
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;";

  std::vector<Value*> args{arg1};
  std::string clobbers="~{x0},~{x29}";
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType* ty = FunctionType::get(irb->getJVoidTy(), params, false);

  irb->CallInlineAsm(ty, args, ss.str(), "r,"+clobbers, true, true);
}


Value* _OneArgDowncall(IRBuilder* irb, int32_t offset, Value* arg1) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args{arg1};
  std::string clobbers="~{x0},~{x29}";
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(), "=r,r,"+clobbers);
}

Value* _OneArgDowncallI(
    IRBuilder* irb, int32_t offset, uint32_t imm1) {
  std::stringstream ss;
  ss
    << "mov x0, #" _IMM1 ";\n"  // imm1
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::string clobbers="~{x0},~{x29}";
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), false);

  return irb->DownCallInlineAsm(ty, ss.str(), "=r,"+clobbers);
}



void _voidTwoArgDowncall(IRBuilder* irb,
    int32_t offset,
    Value* arg1, Value* arg2) {
  std::stringstream ss;
  ss
    << "mov x0, $0;\n" // arg1
    << "mov x1, $1;\n" // arg2
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n";

  std::vector<Value*> args{arg1,arg2};
  std::string clobbers="~{x0},~{x1},~{x29}";
  std::vector<Type*> params{
    irb->getVoidPointerType(),
    irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidTy(), params, false);

  irb->DownCallInlineAsm(ty, args, ss.str(), "r,r,"+clobbers);
}

/**
 * @brief  Two registers in (first value, second immediate)
 *
 * @param irb
 * @param offset
 * @param arg1 llvm register
 * @param arg2 immediate value
 * @param swap_args use imm2 in x0, and lreg in x1
 *
 * @return 
 */
Value* _TwoArgDowncallI(IRBuilder* irb, int32_t offset,
    Value* arg1, uint32_t imm2, bool swap_args) {
  std::stringstream ss;
  if(swap_args) {
    ss
      << "mov x1, $1;\n" // arg1
      << "mov x0, #" _IMM2 ";\n";  // arg2 immediate
  } else {
    ss
      << "mov x0, $1;\n" // arg1
      << "mov x1, #" _IMM2 ";\n";  // arg2 immediate
  }
  ss
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args{arg1};
  std::string clobbers="~{x0},~{x1},~{x29}";
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(),"=r,r,"+clobbers);
}

Value* _TwoArgDowncall(IRBuilder* irb, int32_t offset,
    Value* arg1, Value* arg2, Type* retTy) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "mov x1, $2;\n" // arg2
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  std::string clobbers="~{x0},~{x1},~{x29}";
  std::vector<Type*> params{irb->getVoidPointerType(),
    irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(retTy, params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(),"=r,r,r,"+clobbers);
}

/**
 * @brief Thread arguments downcal
 *
 * @param irb
 * @param offset
 * @param arg1 void* (llvm register)
 * @param imm2 immediate value
 * @param imm3 immedaite value
 *
 * @return 
 */
Value* _ThreeArgDowncallII(IRBuilder* irb,
    int32_t offset,
    Value* arg1, uint32_t imm2, uint32_t imm3) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "mov x1, #" _IMM2 ";\n"
    << "mov x2, #" _IMM3 ";\n"
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args{arg1};
  std::string clobbers="~{x0},~{x1},~{x2},~{x29}";
  std::vector<Type*> params{irb->getVoidPointerType()};
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(), "=r,r,"+clobbers);
}

/**
 * @brief 
 *
 * @param irb
 * @param offset
 * @param arg1 void*
 * @param arg2 uint32_t
 * @param imm3
 *
 * @return 
 */
Value* _ThreeArgDowncallI(IRBuilder* irb, int32_t offset,
    std::vector<Type*> tp_params,
    Value* arg1, Value *arg2, uint32_t imm3) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "mov x1, $2;\n" // arg2
    << "mov x2, #" _IMM3 ";\n"
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  std::string clobbers="~{x0},~{x1},~{x2},~{x29}";
  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), tp_params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(), "=r,r,r,"+clobbers);
}

/**
 * @brief
 *
 * @param irb
 * @param offset
 * @param tp_params Types of the parameters
 * @param arg1
 * @param arg2
 * @param arg3
 *
 * @return 
 */
Value* _ThreeArgDowncall(IRBuilder* irb,
    int32_t offset,
    std::vector<Type*> tp_params,
    Value* arg1, Value *arg2, Value *arg3) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "mov x1, $2;\n" // arg2
    << "mov x2, $3;\n" // arg3
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  std::string clobbers="~{x0},~{x1},~{x2},~{x29}";

  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), tp_params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(), "=r,r,r,r,"+clobbers);
}

void _FourArgDowncall(IRBuilder* irb,
    int32_t offset, std::vector<Type*> tp_params,
    Value* arg1, Value *arg2, Value *arg3,
    Value* arg4) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "mov x1, $2;\n" // arg2
    << "mov x2, $3;\n" // arg3
    << "mov x3, $4;\n" // arg4
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  args.push_back(arg4);
  std::string clobbers="~{x0},~{x1},~{x2},~{x3},~{x29}";

  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), tp_params, false);

  irb->DownCallInlineAsm(ty, args, ss.str(), "=r,r,r,r,r,"+clobbers);
}

Value* _FiveArgDowncallII(IRBuilder* irb,
    int32_t offset,
    Value* arg1, Value *arg2, Value *arg3,
    uint32_t imm4, uint32_t imm5) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "mov x1, $2;\n" // arg2
    << "mov x2, $3;\n" // arg3
    << "mov x3, #" _IMM4 ";\n" // imm4
    << "mov x4, #" _IMM5 ";\n" // imm5
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Type*> params{
    irb->getVoidPointerType(),
    irb->getVoidPointerType(),
    irb->getVoidPointerType()
  };

  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  std::string clobbers="~{x0},~{x1},~{x2},~{x3},~{x4},~{x29}";

  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(), "=r,r,r,r,"+clobbers);
}

Value* _FiveArgDowncall(IRBuilder* irb,
    int32_t offset, std::vector<Type*> tp_params,
    Value* arg1, Value *arg2, Value *arg3,
    Value* arg4, Value* arg5) {
  std::stringstream ss;
  ss
    << "mov x0, $1;\n" // arg1
    << "mov x1, $2;\n" // arg2
    << "mov x2, $3;\n" // arg3
    << "mov x3, $4;\n" // arg4
    << "mov x4, $5;\n" // arg5
    << "ldr x29, [" _TR ", #" _OFST_ "];\n"
    << "blr x29;\n"
    << "mov $0, x0;\n"; // result

  std::vector<Value*> args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  args.push_back(arg4);
  args.push_back(arg5);
  std::string clobbers="~{x0},~{x1},~{x2},~{x3},~{x4},~{x29}";

  FunctionType* ty =
    FunctionType::get(irb->getVoidPointerType(), tp_params, false);

  return irb->DownCallInlineAsm(ty, args, ss.str(), "=r,r,r,r,r,r,"+clobbers);
}

Value* ___BinaryOp(IRBuilder* irb, std::string op,
    Value* lhs, Value* rhs, bool i32) {
  Type *pTy= i32? pTy=irb->getJIntTy():pTy=irb->getJLongTy();
  std::vector<Type*> params{2, pTy};
  FunctionType *fTy=FunctionType::get(pTy, params, false);
  std::vector<Value*> args{lhs, rhs};
  std::string s=
    op + " " + __reg(0,i32) + ", "+__reg(1, i32)+", "+__reg(2, i32);

  return irb->CallInlineAsm(fTy, args, s, "=r,r,r", false, true);
}

Value* ___BinaryOpImm2(IRBuilder* irb, std::string op,
    Value* lhs, u_int32_t imm2, bool i32) {
  Type *pTy= i32? pTy=irb->getJIntTy():pTy=irb->getJLongTy();
  std::vector<Type*> params{pTy};
  FunctionType *fTy=FunctionType::get(pTy, params, false);
  std::vector<Value*> args{lhs};
  std::stringstream ss;
  ss <<
    op<<" "<<__reg(0,i32)<<", "<<__reg(1, i32)<< ", #" _IMM2 "";

  return irb->CallInlineAsm(fTy, args, ss.str(), "=r,r", false, true);
}

#undef _IMM1
#undef _IMM2
#undef IMM3
#undef _IMM3
#undef _IMM4
#undef _IMM5
#undef _TR
#undef _OFST_
#undef REG
#undef REGw

#include "llvm_macros_undef.h"

}  // namespace Arm64
}  // namespace LLVM
}  // namespace art
