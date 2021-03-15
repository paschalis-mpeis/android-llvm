/**
 * LLVM::IRBuilder is extensively used through out this LLVM backend.
 * Here we extend it's functionality.
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
#ifndef ART_COMPILER_LLVM_IR_BUILDER_H_
#define ART_COMPILER_LLVM_IR_BUILDER_H_

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_os_ostream.h>
#include "base/logging.h"
#include "llvm_info.h"
#include "hgraph_printers.h"
#include "mcr_rt/mcr_rt.h"
#include "optimizing/nodes.h"
#include "hgraph_printers.h"
#include "optimizing/data_type-inl.h"
#include "dex/primitive.h"

#include <android-base/logging.h>

// Any integral type smaller than i32 will be remapped to i32
#define LLVM_REMAP_INT_TYPES

#define _LOGLLVM(irb, severity, msg) (irb)->AndroidLogPrint((severity), (msg))

#ifdef CRDEBUG4
#define _LOGLLVM4(irb, severity, msg) (irb)->AndroidLogPrint((severity), (msg))
#define _LOGLLVM4val(irb, severity, msg, value) (irb)->AndroidLogPrintHex((severity), (msg), (value))
#else
#define _LOGLLVM4(irb, severity, msg) if(LIKELY(false)) (irb)->AndroidLogPrint((severity), (msg))
#define _LOGLLVM4val(irb, severity, msg, value) if(LIKELY(false)) (irb)->AndroidLogPrintHex((severity), (msg), (value))
#endif
#ifdef CRDEBUG3
#define _LOGLLVM3(irb, severity, msg) (irb)->AndroidLogPrint((severity), (msg))
#define _LOGLLVM3val(irb, severity, msg, value) (irb)->AndroidLogPrintHex((severity), (msg), (value))
#else
#define _LOGLLVM3(irb, severity, msg) if(LIKELY(false)) (irb)->AndroidLogPrint((severity), (msg))
#define _LOGLLVM3val(irb, severity, msg, value) if(LIKELY(false)) (irb)->AndroidLogPrintHex((severity), (msg), (value))
#endif
#ifdef CRDEBUG2
#define _LOGLLVM2(irb, severity, msg) (irb)->AndroidLogPrint((severity), (msg))
#define _LOGLLVM2val(irb, severity, msg, value) (irb)->AndroidLogPrintHex((severity), (msg), (value))
#else
#define _LOGLLVM2(irb, severity, msg) if(LIKELY(false)) (irb)->AndroidLogPrint((severity), (msg))
#define _LOGLLVM2val(irb, severity, msg, value) if(LIKELY(false)) (irb)->AndroidLogPrintHex((severity), (msg), (value))
#endif

using namespace ::llvm;
using namespace ::android::base;

namespace art {
namespace LLVM {

class FunctionHelper;
class HGraphToLLVM;

class IRBuilder : public LLVMIRBuilder {
 public:
  explicit IRBuilder(LLVMContext& context,
                     Module& module,
                     FunctionHelper& fh,
                     InstructionSet instruction_set)
      : LLVMIRBuilder(context),
        module_(&module),
        fh_(&fh),
        instruction_set_(instruction_set) {
    GenerateTypeDefinitions();
    UNUSED(instruction_set_);
  }

  void GenerateTypeDefinitions();
  void LoadFromArtModule();
  
  void setHgraphPrinter(::art::HGraphFilePrettyPrinter* prt) {
    prt_ = prt;
  }

  Module* getModule() const {
    return module_;
  }

  //--------------------------------------------------------------------------
  // Type Helper Function
  //--------------------------------------------------------------------------
  StructType* getShadowFrameTy(uint32_t vreg_size);

  StructType* getJValueTy() {
    return module_->getTypeByName("union.art::JValue");
  }

  StructType* getStackReferenceTy() {
#ifdef ART_MCR_ANDROID_10
#define _art_mirror_objref "class.art::mirror::ObjectReference"
#elif defined(ART_MCR_ANDROID_6)
#define _art_mirror_objref "class.art::mirror::ObjectReference.0"
#endif
    return module_->getTypeByName(_art_mirror_objref);
#undef _art_mirror_objref
  }

  StructType* getMirrorObjectTy() {
    return module_->getTypeByName("class.art::mirror::Object");
  }

  PointerType* getMirrorObjectPointerTy() {
    return getMirrorObjectTy()->getPointerTo();
  }


#ifdef CODE_UNUSED
  StructType* getMirrorStringTy() {
    return module_->getTypeByName("class.art::mirror::String");
  }

  PointerType* getMirrorStringPointerTy() {
    return getMirrorStringTy()->getPointerTo();
  }
#endif

  PointerType* getMethodArgsTy() {
    return PointerType::get(IntegerType::get(getContext(), 32), 0);
  }

  Type* getJObjectTy() {
    return IntegerType::get(getContext(), 8); 
  }

  /**
   * @brief naming here a bit confusing?
   *        I mean the size of storing a handle in art?
   */
  Type* getJObjectHandleTy() {
    DLOG(FATAL) << __func__ << ": Dont use this!";
    if(IsCompilingArm64()) {
      return getJLongTy();
    } else {
      return getJIntTy();
    }
  }

  Type* getVoidPointerType() {
    return getJObjectTy()->getPointerTo();
  }

  PointerSize GetPointerSize() {
    if(IsCompilingArm64()) {
      return kArm64PointerSize;
    } else {
      return kArmPointerSize;
    }
  }

  Type* getType(HInstruction* h);
  Type* getType(DataType::Type type);

  Type* getTypeExact(DataType::Type type);
  Type* getTypeExact(HInstruction* h);
  Type* getTypeUpcast(DataType::Type type);
  bool IsChar(DataType::Type type) {
    return (type == DataType::Type::kUint16);
  }

  Type* getTypeFromShorty(char shorty_jty);
  Type* getTypeFromShortyJNI(char shorty_jty);
  Value* UpcastInt(Value* value, HInstruction* h);
  Value* UpcastInt(Value* value, DataType::Type type);

  Type* getJVoidTy() { return getVoidTy(); }
  IntegerType* getJBooleanTy() { return getInt8Ty(); }
  IntegerType* getBooleanTyUpcast() { return getJIntTy(); }
  IntegerType* getCBoolTy() { return getInt1Ty(); }
  IntegerType* getJByteTy() { return getInt8Ty(); }
  IntegerType* getJCharTy() { return getInt16Ty(); }
  IntegerType* getJShortTy() { return getInt16Ty(); }
  IntegerType* getJIntTy() { return getInt32Ty(); }
  IntegerType* getJLongTy() { return getInt64Ty(); }
  Type* getJFloatTy() { return getFloatTy(); }
  Type* getJDoubleTy() { return getDoubleTy(); }

  //--------------------------------------------------------------------------
  // Constant Value Helper Function
  //--------------------------------------------------------------------------

  ConstantInt* getCBool(uint32_t val) {
    return ConstantInt::getSigned(getCBoolTy(), val);
  }

  // CHECK: Shouldn't it be i32?
  ConstantInt* getJBoolean(bool is_true) {
#ifdef LLVM_REMAP_INT_TYPES
    return (is_true) ? getInt8(1) : getInt8(0);
#else
    return (is_true) ? getTrue() : getFalse();
#endif
  }

  ConstantInt* getJByte(int8_t i) {
#ifdef LLVM_REMAP_INT_TYPES
    return getJInt(i);
#else
    return ConstantInt::getSigned(getJByteTy(), i);
#endif
  }

  ConstantInt* getJChar(int16_t i) {
#ifdef LLVM_REMAP_INT_TYPES
    return getJUnsignedInt(i);
#else
    return ConstantInt::getSigned(getJCharTy(), i);
#endif
  }

  ConstantInt* getJShort(int16_t i) {
#ifdef LLVM_REMAP_INT_TYPES
    return getJInt(i);
#else
    return ConstantInt::getSigned(getJShortTy(), i);
#endif
  }

//   ConstantInt* getHalfWord(int16_t i) {
//     return ConstantInt::getSigned(getJShortTy(), i);
//   }

  ConstantInt* getJInt(int32_t i) {
    return ConstantInt::getSigned(getJIntTy(), i);
  }

  ConstantInt* getJUnsignedInt(uint32_t i) {
    return ConstantInt::get(getJIntTy(), i, false);
  }

  ConstantInt* getJUnsignedInt16(uint16_t i) {
    return ConstantInt::get(getInt16Ty(), i, false);
  }

  ConstantInt* getJLong(int64_t i) {
    return ConstantInt::getSigned(getJLongTy(), i);
  }

  Constant* getJFloat(float f) {
    return ConstantFP::get(getJFloatTy(), f);
  }

  Constant* getJDouble(double d) {
    return ConstantFP::get(getJDoubleTy(), d);
  }

  ConstantPointerNull* getJNull() {
    return ConstantPointerNull::get(getJObjectTy()->getPointerTo());
  }

  ConstantPointerNull* getJNull(Type* type) {
    return ConstantPointerNull::get(type->getPointerTo());
  }

  Constant* getJZero(char shorty_jty) {
    return getJZero(DataType::FromShorty(shorty_jty));
  }

  Constant* getJZero(DataType::Type type) {
    switch (type) {
      case DataType::Type::kVoid:
        DLOG(FATAL) << "Zero is not a value of void type";
        UNREACHABLE();
      case DataType::Type::kBool:
        return getJBoolean(false);
      case DataType::Type::kInt8:
        return getJByte(0);
      case DataType::Type::kUint16:
        return getJChar(0);
      case DataType::Type::kInt16:
        return getJShort(0);
      case DataType::Type::kInt32:
        return getJInt(0);
      case DataType::Type::kInt64:
        return getJLong(0);
      case DataType::Type::kFloat32:
        return getJFloat(0.0f);
      case DataType::Type::kFloat64:
        return getJDouble(0.0);
      case DataType::Type::kReference:
        return getJNull();
      default:
        DLOG(FATAL) << "Unknown dalvik type: " << type;
        UNREACHABLE();
    }
  }

  void CallStoreReturnValue(Value* jvalue,
                            Value* ret_val, DataType::Type ret_type);
  Value* CallGetReturnValue(HGraphToLLVM* hgraph_to_llvm, Value* jvalue,
                                    DataType::Type ret_type);

  Value*	CreateFastCall(Function* F, std::vector<Value*> args);
  Value*	CreateFastCall(Function* F);

  Value* mCreateAdd(bool is_fp, Value* lhs, Value* rhs);
  Value* mCreateSub(bool is_fp, Value* lhs, Value* rhs);
  Value* mCreateMul(bool is_fp, Value* lhs, Value* rhs);
  Value* mCreateDiv(bool is_fp, Value* lhs, Value* rhs);
  Value* mCreateRem(bool is_fp, bool is_signed, Value* lhs, Value* rhs);

  Value* mCreateNeg(bool is_fp, bool is_signed, Value* v);

  Value* mCreateCmpEQ(bool is_fp, Value* lhs, Value* rhs);
  Value* mCreateCmpNE(bool is_fp, Value* lhs, Value* rhs);
  Value* mCreateCmpGT(bool is_fp, bool is_signed, Value* lhs, Value* rhs);
  Value* mCreateCmpGE(bool is_fp, bool is_signed, Value* lhs, Value* rhs);
  Value* mCreateCmpLT(bool is_fp, bool is_signed, Value* lhs, Value* rhs);
  Value* mCreateCmpLE(bool is_fp, bool is_signed, Value* lhs, Value* rhs);
  Value* CreateCmpIsNull(Value* v);

  void mCreateStore(Value* src, Value* dest, bool is_volatile = false);

  static bool IsSigned(DataType::Type type);

  Value* AndroidLogSeverity(LogSeverity log_severity);

  void AndroidLogPrintChar(
      LogSeverity log_severity, std::string msg, Value* value);
  void AndroidLogPrint(LogSeverity log_severity, std::string msg);
  void AndroidLogPrintHex(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintHexAndInt(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintFloat(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintDouble(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintInt(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintUint(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintLong(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintUlong(LogSeverity log_severity, std::string msg,
      Value* value);
  void AndroidLogPrintValue(
      LogSeverity svrt, std::string msg, Value* value, DataType::Type type);

  Type* GetShadowFrameTy() {
    return structShadowFrame_;
  }

  StructType* GetManagedStackTy() {
    return structManagedStack_;
  }

  Value* CreatePtrDisp(Value* base,
                               Value* offset,
                               PointerType* ret_ty, bool i32=false) {
    Value* base_int = CreatePtrToInt(base, getPtrEquivIntTy(i32));
    Value* result_int = CreateAdd(base_int, offset);
    Value* result = CreateIntToPtr(result_int, ret_ty);

    return result;
  }

  IntegerType* getPtrEquivIntTy(bool i32=false) {
    if(i32) {
      return getInt32Ty();
    } else {
      return getInt64Ty();
    }
  }

  ConstantInt* getPtrEquivInt(int64_t i, bool i32=false) {
    return ConstantInt::get(getPtrEquivIntTy(i32), i);
  }

  PointerType* getJEnvTy() {
    return jenv_type_;
  }

  PointerType* getJniObjectTy() {
    return jni_object_type_;
  }

  FunctionHelper* FH() {
    return fh_;
  }

  Value* StoreToOldShadowFrame(HGraphToLLVM* HL,
                                       Value* old_shadow_frame,
                                       Value* art_method);

  Value* PushShadowFrame(HGraphToLLVM* HL,
                                 Value* new_shadow_frame,
                                 Value* current_thread,
                                 Value* art_method,
                                 uint32_t num_vregs);

  void PopShadowFrame(HGraphToLLVM* HL,
                      Value* current_thread,
                      Value* old_shadow_frame);

  bool IsCompilingArm64() {
    return instruction_set_ == InstructionSet::kArm64;
  }

  Value* mCreateGlobalStringPtr(std::string str);

  Value* CallInlineAsm(FunctionType* fty, std::string cmd,
      std::string constraints,
      bool clobberCC = false, bool hasSideEffects=false);

  Value* CallInlineAsm(FunctionType* fty, std::vector<Value*> args,
      std::string cmd, std::string constraints,
      bool clobberCC = false, bool hasSideEffects=false);

  /**
   * @brief Sets the cc contraint (flags register is modified
   */
  Value* DownCallInlineAsm(FunctionType* fty, std::string cmd,
      std::string constraints) {
    return CallInlineAsm(fty, cmd, constraints, true, false);
  }

 /**
  * @brief Sets the cc contraint (flags register is modified
  */
 Value* DownCallInlineAsm(FunctionType* fty, std::vector<Value*> args,
     std::string cmd, std::string constraints) {
   return CallInlineAsm(fty, args, cmd, constraints, true, false);
  }

 private:
  Module* module_;
  FunctionHelper* fh_;
  InstructionSet instruction_set_;
  ::art::HGraphFilePrettyPrinter* prt_ = nullptr;
  StructType* structShadowFrame_;
  StructType* structManagedStack_;
  PointerType* jenv_type_;
  PointerType* jni_object_type_;
  std::map<std::string, Value*> gbl_strings_;
};

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_IR_BUILDER_H_
