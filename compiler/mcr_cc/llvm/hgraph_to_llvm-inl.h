/**
 * Several operations used by hgraph_to_llvm.cc and others.
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
#ifndef ART_COMPILER_LLVM_HGRAPH_TO_LLVM_INL_H_
#define ART_COMPILER_LLVM_HGRAPH_TO_LLVM_INL_H_

#include "hgraph_load_ops-inl.h"
#include "hgraph_to_llvm.h"

#include "art_method.h"
#include "asm_arm64.h"
#include "asm_arm_thumb.h"
#include "class_linker-inl.h"
#include "llvm_utils.h"
#include "mcr_cc/analyser.h"
#include "mcr_cc/mcr_cc.h"
#include "mirror/class_loader.h"
#include "mirror/object.h"
#include "mirror/string.h"
#include "scoped_thread_state_change.h"
#include "arch/instruction_set.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

inline InstructionSet HGraphToLLVM::GetISA() {
  return llcu_->GetInstructionSet();
}

inline int64_t HGraphToLLVM::Int64FromLocation(HInstruction* h) {
  return Int64FromConstant(h->AsConstant());
}

inline std::string HGraphToLLVM::GetCallingMethodName(
    std::string pretty_method, HInvoke* invoke, std::string pre_fix) {
  bool is_static = false;
  if (invoke->IsInvokeStaticOrDirect()) {
    is_static = invoke->AsInvokeStaticOrDirect()->IsStatic();
  }
  std::string signature = GetSignature(invoke);
  return GetCallingMethodName(pretty_method, signature, is_static, pre_fix);
}

inline std::string HGraphToLLVM::GetCallingMethodName(
    std::string pretty_method, std::string signature,
    bool is_static, std::string pre_fix) {
  return pre_fix + GetMethodName(pretty_method, is_static, signature);
}

inline std::string getEscapedSignature(std::string unescaped_signature) {
  return mcr::McrCC::EscapeNameForLLVM(unescaped_signature);
}

inline std::string getEscapedSignature(const DexFile* dex_file,
                                       const dex::MethodId& method_id) {
  Signature signature = dex_file->GetMethodSignature(method_id);
  return mcr::McrCC::EscapeNameForLLVM(signature.ToString());
}

inline std::string HGraphToLLVM::GetInnerSignature() {
  const DexFile* dex_file = dcu_.GetDexFile();
  const dex::MethodId& method_id = dex_file->GetMethodId(
      dcu_.GetDexMethodIndex());
  return getEscapedSignature(dex_file, method_id);
}

inline std::string HGraphToLLVM::GetSignature(ArtMethod* method) {
  Thread* self = Thread::Current();
  Locks::mutator_lock_->SharedLock(self);
  std::string res = getEscapedSignature(
      method->GetSignature().ToString());
  Locks::mutator_lock_->SharedUnlock(self);

  return res;
}

inline std::string HGraphToLLVM::GetSignature(HInvoke* hinvoke) {
  uint32_t didx = hinvoke->GetDexMethodIndex();
  const DexFile* dex_file = &GetGraph()->GetDexFile();
  D5LOG(ERROR) << __func__ << prt_->GetInstruction(hinvoke);

  // multi-dex call: use the proxy/original didx
  if (hinvoke->HasSpeculation()) {
    didx = hinvoke->GetOptimizingDexMethodIndex();
  }
    
  MethodReference mr(dex_file, didx);

  // multi-dex code sample: 
  // if (mcr::Analyser::IsMultiDex(mr)) {
  //   mr = mcr::Analyser::GetMultiDexTarget(mr);
  //   dex_file = mr.dex_file;
  //   didx = mr.index;
  // }

  const dex::MethodId& method_id = dex_file->GetMethodId(didx);
  std::string res = getEscapedSignature(dex_file, method_id);

  return res;
}

inline std::string HGraphToLLVM::GetInnerMethodName(
    bool is_static, std::string signature) {
  return GetMethodName(GetPrettyMethod(), is_static, signature);
}

inline std::string HGraphToLLVM::GetMethodName(
    std::string method_name, bool is_static, std::string signature) {
  std::string static_or_instance =
      is_static ? "_static_" : "_instance_";
  return EscapeString(method_name) + static_or_instance + signature;
}

inline std::string HGraphToLLVM::EscapeString(std::string pretty_method) {
  std::size_t s = pretty_method.find(' ');
  std::size_t e = pretty_method.find('(');

  if (s == std::string::npos || e == std::string::npos) {
    DLOG(FATAL) << __func__;
  }

  std::string name = pretty_method.substr(s + 1, e - s - 1);
  return mcr::McrCC::EscapeNameForLLVM(name);
}

inline std::string HGraphToLLVM::GetTwineParam(uint32_t index) {
  return GetTwine(index, "p");
}

inline std::string HGraphToLLVM::GetTwineArg(uint32_t index) {
  return GetTwine(index, "a");
}

inline std::string HGraphToLLVM::GetTwine(uint32_t idx, std::string prefix) {
  return prefix + std::to_string(idx);
}

inline std::string HGraphToLLVM::GetTwine(HInstruction* h) {
  if (h->IsParameterValue()) {
    HParameterValue* pv = h->AsParameterValue();
    if (!pv->CanBeNull()) {
      return "receiver";
    }
  }

  std::string name;
  if (h->IsPhi()) {
    name = "phi";
  } else if (h->IsParameterValue()) {
    name = "p";
  } else {
    name = "reg";
  }

  name += std::to_string(h->GetId());
  return name.c_str();
}

inline void _PrintBasicBlockDebug(HGraphToLLVM* HL, IRBuilder* irb, std::string blockname,
    std::string nameOverride, std::string extra) {
  std::string msg;
  msg+="BB:" + std::to_string(HL->GetGraph()->GetMethodIdx())+":";
  if(nameOverride.length()>0) {
    msg+=nameOverride;
  } else {
    msg+=blockname;
  }
  if(extra.length()>0) msg+=" "+extra;
  msg+="\n";

  irb->AndroidLogPrint(INFO, msg);
}

inline void HGraphToLLVM::PrintBasicBlockDebug(BasicBlock* lblock,
    std::string nameOverride, std::string extra) {
  _PrintBasicBlockDebug(this, irb_, Pretty(lblock), nameOverride, extra);
}

inline void HGraphToLLVM::PrintBasicBlockDebug(HBasicBlock* hblock,
    std::string nameOverride, std::string extra) {
  _PrintBasicBlockDebug(this, irb_, GetBasicBlockName(hblock),
      nameOverride, extra);
}

inline std::string HGraphToLLVM::GetBasicBlockName(HBasicBlock* block) {
  std::string prefix;

  if (block->IsEntryBlock()) {
    return ENTRY_HGRAPH;
  } else if (block->IsExitBlock()) {
    return "exit";
  } else if (block->IsCatchBlock()) {
    prefix = "catch";
  } else {
    prefix = "block";
  }
  return prefix + std::to_string(block->GetBlockId());
}

inline Value* HGraphToLLVM::LoadArgument(
    BasicBlock* block,  const char shortyi,
    Value* args, uint32_t args_index) {

  DataType::Type type = DataType::FromShorty(shortyi);
  size_t alignment = DataType::Size(type);
  Type* ltype = irb_->getTypeFromShorty(shortyi);
  bool is_obj = (type == DataType::Type::kReference);
 
  AllocaInst* alloca = irb_->CreateAlloca(
      ltype, 0, GetTwineArg(args_index));
  alloca->setAlignment(Align(alignment));

  std::vector<Value*> idx_list;
  idx_list.push_back(irb_->getInt32(args_index));
  Value* gep = irb_->CreateInBoundsGEP(args, idx_list);

  Value* cast = irb_->CreateBitCast(gep, ltype->getPointerTo());

  Value* to_store = nullptr;
  if (is_obj) {
    to_store = GetPointerFromHandle(cast);
  } else {
    LoadInst* load_from_args = irb_->CreateLoad(cast);
    to_store = load_from_args;
  }

  irb_->CreateStore(to_store, alloca, false);
  return irb_->CreateLoad(alloca);
}

inline Value* HGraphToLLVM::GetPointerFromHandle(
    Value* ptr) {
  D5LOG(INFO) << __func__ << ": getStackReferenceTy";
  Value* obj_ref = ptr;
  obj_ref = irb_->CreateBitCast(
      obj_ref, irb_->getStackReferenceTy()->getPointerTo());

  ::std::vector<Value*> args{obj_ref};
  Value* obj = irb_->CreateCall(fh_->__object_AsMirrorPtr(), args);

  return irb_->CreateBitCast(obj,
      irb_->getJObjectTy()->getPointerTo());
}

inline Value* HGraphToLLVM::GetHandleFromPointer(
    Value* ptr) {
  Value* obj = irb_->CreateBitCast(
      ptr, irb_->getMirrorObjectTy()->getPointerTo());
  obj->setName("obj");

  ::std::vector<Value*> args{obj};
  Value* obj_ref = irb_->CreateCall(fh_->__object_FromMirrorPtr(), args);
  obj_ref->setName("obj_ref");
  obj_ref= irb_->CreateTrunc(obj_ref, irb_->getJIntTy());

  return obj_ref;  // ref is an i32 value
}

// InputCPURegisterOrZeroRegAt
inline Value* HGraphToLLVM::InputAtOrZero(HInstruction* instr, int index) {
  HInstruction* input = instr->InputAt(index);
  DataType::Type input_type = input->GetType();
  if (input->IsConstant() && input->AsConstant()->IsZeroBitPattern()) {
    if(input_type == DataType::Type::kReference) {
      // using ints to enforce the usage of w registers (wzr)
      return irb_->getJInt(0);
    } else {
      return irb_->getJZero(input_type);
    }
  }
  return getValue(input);
}

inline bool HGraphToLLVM::InputAtIsZero(HInstruction* instr, int index) {
  HInstruction* input = instr->InputAt(index);
  DataType::Type input_type = input->GetType();
  if (input->IsConstant() && input->AsConstant()->IsZeroBitPattern()) {
    if(input_type == DataType::Type::kReference) {
      // using ints to enforce the usage of w registers (wzr)
      return irb_->getJInt(0);
    } else {
      return irb_->getJZero(input_type);
    }
  }
  return getValue(input);
}

/**
 * @brief The index of the array might be the outcome of a HBoundsCheck
 *        instruction.
 *        In that case we would have failed to identify
 *        that the index was an IntConstant instruction.
 *
 *        This method cover this case.
 */
inline HInstruction* HGraphToLLVM::GetArrayIndex(HInstruction* h) {
  CHECK(h->IsArraySet() || h->IsArrayGet());

  HInstruction* index = nullptr;
  if (h->IsArraySet()) {
    index = h->AsArraySet()->GetIndex();
  } else if (h->IsArrayGet()) {
    index = h->AsArrayGet()->GetIndex();
  }

  if (index->IsBoundsCheck()) {
    index = index->AsBoundsCheck()->InputAt(0);
  }
  return index;
}

/**
 * @brief TODO implement volatiles (it's arch specific)
 *        Must emit memory barrier specific to the type of instruction
 *        (get or set).
 *        Must be different for 
 *        example from art::Optimizing:
 *        GenerateMemoryBarrier(MemBarrierKind::kLoadAny)
 */
inline void HGraphToLLVM::UnimplementedVolatile(HInstruction* h) {
  DataType::Type type = h->GetType();
  if (DataType::Is64BitType(type)) {
    DLOG(ERROR) << "Volatile(64bit): " << GetTwine(h);
  } else {
    DLOG(ERROR) << "Volatile: " << GetTwine(h);
  }
}

inline bool HGraphToLLVM::IsShiftOperation(HInstruction* h) {
  return h->IsShl() || h->IsShr() || h->IsUShr() || h->IsRor();
}

inline void HGraphToLLVM::StoreToObjectOffset(Value* addr,
                                              uint32_t offset,
                                              Value* new_value) {
  if (offset == 0) {
    Value* cast = irb_->CreateBitCast(addr, new_value->getType()->getPointerTo());
    irb_->CreateStore(new_value, cast, false);
  } else {
    // Convert offset to value
    Value* llvm_offset = irb_->getPtrEquivInt(offset);
    // Calculate the value's address
    Value* value_addr =
        irb_->CreatePtrDisp(addr, llvm_offset, new_value->getType()->getPointerTo());

    // Store
    irb_->CreateStore(new_value, value_addr, false);
  }
}

inline Value* HGraphToLLVM::LoadThread() {
  // switch to the first basic block
  BasicBlock* pinsert_point = irb_->GetInsertBlock();
  Function* F=pinsert_point->getParent();
  BasicBlock* entry_block=&F->getEntryBlock();
  ::llvm::Instruction* insertBefore = nullptr;

  D4LOG(INFO) << __func__ << ": For: " << Pretty(F);
  const bool dontAddOnFisrtBB = fh_->IsSuspendCheck(F);

  // as LoadThread on creation is modifying the insert point,
  // it will modify our custom insertion point (first instruction), so we:
  // 1. get the method
  // 2. set the insertion point (1st instruction)
  // 3. call the method
  // 4. restore the outermost insertion point
  Function* func = fh_->LoadThread(this, irb_);

  if(!dontAddOnFisrtBB) {
    insertBefore=entry_block->getFirstNonPHI();
    if(insertBefore == nullptr) {
      // could be the first instruction
      irb_->SetInsertPoint(entry_block);
    } else {
      irb_->SetInsertPoint(insertBefore);
    }
  }

  Value* thread_register=irb_->CreateCall(func);
  thread_register->setName("thread");
  
  if(!dontAddOnFisrtBB) {
    irb_->SetInsertPoint(pinsert_point);
  }
  return thread_register;
}

inline Value* HGraphToLLVM::LoadJNIEnv() {
  D5LOG(INFO) << __func__;

  // switch to the first basic block
  BasicBlock* pinsert_point = irb_->GetInsertBlock();
  Function* F = pinsert_point->getParent();
  BasicBlock* entry_block=&F->getEntryBlock();

  // thread is isnerted at the beginning
  Value* thread = GetLoadedThread();

  // we will insert jnienv at the end (but before any branch insnt)
  ::llvm::Instruction* pinst = &*(entry_block->rbegin());
  if(isa<BranchInst>(pinst) || isa<SwitchInst>(pinst)) {
    irb_->SetInsertPoint(pinst);
  } else {
    irb_->SetInsertPoint(entry_block);
  }

  // LoadFromAddress also works
  Value* jni_env = LoadFromObjectOffset(
      thread, GetThreadJNIEnvOffset(), irb_->getJEnvTy());
  jni_env->setName("JNIEnv");

  irb_->SetInsertPoint(pinsert_point);
  return jni_env;
}

inline void HGraphToLLVM::CacheThreadForCurrentMethod(Value* param_thread) {
  // Some methods might have thread as a parameter, e.g., llvm_*
  // So it's cached, so it can be used directlyt
  BasicBlock* insert_block = irb_->GetInsertBlock();
  Function* F= insert_block->getParent();
  loaded_thread_[F]=param_thread;
}

inline Value* HGraphToLLVM::GetLoadedThread() {
  BasicBlock* insert_block = irb_->GetInsertBlock();
  Function* F= insert_block->getParent();
  CHECK(F != nullptr) << "failed to get current function";

  Value* value=nullptr;
  if(loaded_thread_.find(F) != loaded_thread_.end()) {
    value=loaded_thread_[F];
  } else { // load using inline asm and return
    value=LoadThread();
    loaded_thread_[F]=value;
  }
  return value;
}

inline Value* HGraphToLLVM::GetLoadedJNIEnv() {
  BasicBlock* insert_block = irb_->GetInsertBlock();
  Function* F=insert_block->getParent();
  CHECK(F != nullptr) << "failed to get current function";

  Value* loaded =nullptr;
  if(jnienv_.find(F) != jnienv_.end()) {
    loaded=jnienv_[F];
  } else { // load using inline asm and return
    loaded=LoadJNIEnv();
    jnienv_[F]=loaded;
  }
  return loaded;
}

inline Value* HGraphToLLVM::GetQuickEntrypointASM(QuickEntrypointEnum qpoint) {
  Function* F = fh_->GetQuickEntrypoint(this, irb_, qpoint);
  Value* e=irb_->CreateCall(F);
  e->setName("entrypoint" + std::to_string(static_cast<int>(qpoint)) + "_");
  return e;
}

inline Value* HGraphToLLVM::GetQuickEntrypoint(QuickEntrypointEnum qpoint) {
  InstructionSet isa = GetLlvmCompilationUnit()->GetInstructionSet();
  D5LOG(INFO) << __func__;
  Value* thread = GetLoadedThread();

  Value *e= nullptr;
  uint32_t offset;
  switch (isa) {
    case InstructionSet::kArm64:
        offset = Arm64::offset(qpoint);
        break;
    default:
      DLOG(FATAL) << "Unimplemented for architecture: " << isa;
      UNREACHABLE();
  }

  e = Load<true>(thread, offset);
  e->setName("quick" + std::to_string(static_cast<int>(qpoint)));
  return e;
}

inline Value* HGraphToLLVM::artCall(QuickEntrypointEnum qpoint, Type* retTy) {
  Value* art_entrypoint = GetQuickEntrypointASM(qpoint);
  PointerType* fpTy = PointerType::getUnqual(FunctionType::get(retTy, false));
  Function* rt_method=(Function*) irb_->CreateBitCast(art_entrypoint, fpTy);
  rt_method->setName(Pretty(qpoint)+ "_");
  return irb_->CreateCall(rt_method);
}

inline Value* HGraphToLLVM::artCall(QuickEntrypointEnum qpoint, Type* retTy,
    std::vector<Type*> params, std::vector<Value*> args) {
  Value* art_entrypoint = GetQuickEntrypointASM(qpoint);

  // getUnqual is for zero address space
  PointerType* fpTy = PointerType::getUnqual(
      FunctionType::get(retTy, params, false));

  Function* rt_method= (Function*)irb_->CreateBitCast(art_entrypoint, fpTy);
  rt_method->setName(Pretty(qpoint));
  return irb_->CreateCall(rt_method, args);
}

inline Value* HGraphToLLVM::GetClassTypeIdx(Value* klass) {
  uint32_t offset = mirror::Class::TypeIdxOffset().Uint32Value();
  LOG(WARNING) << "VERIFY_LLVM: mirror::Class::TypeIdxOffset: " << offset;
  Type* type = irb_->getJIntTy()->getPointerTo();
  return LoadFromAddress(klass, offset, type);
}

inline Value* HGraphToLLVM::GetObjectClassDirect(Value* obj) {
  uint32_t offset = mirror::Object::ClassOffset().Uint32Value();
  return LoadWord<true>(obj, offset);
}

inline Value* HGraphToLLVM::GenerateReferenceObjectClass(
    HInstruction* h, Value* lobj, ReadBarrierOption read_barrier_option) {
  uint32_t offset = mirror::Object::ClassOffset().Uint32Value();
  return GenerateReferenceLoad(irb_, h, lobj, offset, read_barrier_option);
}

inline void HGraphToLLVM::CallMarkGCCard(Value* obj, Value* val,
    DataType::Type type, bool value_can_be_null) {
  std::vector<Value*> args;
  args.push_back(obj);
  args.push_back(val);
  Function* f = fh_->MarkGCCard(irb_, type, value_can_be_null);
  irb_->CreateCall(f, args);
}

inline Value* HGraphToLLVM::GetArtMethodClassSLOW(Value* art_method) {
  irb_->AndroidLogPrint(WARNING, "GetArtMethodClassSLOW: SlowPath");

  std::vector<Value*> args;
  args.push_back(art_method);
  Value* v = irb_->CreateCall(fh_->__GetDeclaringClass(), args);
  v->setName("declaring_class_");
  return v;
}

inline Value* HGraphToLLVM::GetArtMethodClass(Value* art_method) {
  uint32_t offset = ArtMethod::DeclaringClassOffset().Int32Value();
  Value* klass = LoadWord<true>(art_method, offset);
  klass->setName("art_method_class");
  return klass;
}

// VERIFIED
inline Value* HGraphToLLVM::GetStringLength(Value* str_obj) {
  uint32_t offset = mirror::String::CountOffset().Uint32Value();
  irb_->AndroidLogPrint(WARNING, "VERIFY_LLVM: GetStringLength");
  D2LOG(INFO) << "GetStringLength: offset: " << std::to_string(offset);

  std::vector<Value*> offst_string_len;
  offst_string_len.push_back(irb_->getJUnsignedInt(offset));
  Value* gep = irb_->CreateInBoundsGEP(str_obj, offst_string_len);
  Value* cast = irb_->CreateBitCast(
      gep, irb_->getJIntTy()->getPointerTo());
  Value* str_len = irb_->CreateLoad(cast);
  str_len->setName("strlen");
  return str_len;
}

inline ArtMethod* HGraphToLLVM::ResolveMethod(
    const DexFile* dex_file, uint32_t dex_method_idx, InvokeType invoke_type) {
  Thread* self = Thread::Current();
  ScopedObjectAccess soa(self);
  StackHandleScope<4> hs(soa.Self());

  ClassLinker* linker = dcu_.GetClassLinker();
  ObjPtr<mirror::DexCache> dex_cache = linker->FindDexCache(self, *dex_file);
  Handle<mirror::DexCache> h_dex_cache(hs.NewHandle(dex_cache.Ptr()));

  D3LOG(INFO) << __func__ << ": input: " << dex_method_idx
    << ":" << mcr::McrCC::PrettyDexFile(dex_file->GetLocation());
  ArtMethod* resolved_method = linker->ResolveMethod<__resolve_mode>(
      dex_method_idx, h_dex_cache,
      dcu_.GetClassLoader(), nullptr, invoke_type);

  std::string extra;
  if (resolved_method->IsNative()) extra += " [Native]";
  if (resolved_method->IsAbstract()) extra += " [Abstract]";
  D3LOG(INFO) << __func__ << " Found: " 
              << resolved_method->GetDexMethodIndex()
              << ": " << resolved_method->PrettyMethod()
              // << ":" << invoke_type
              << "\nType:" << resolved_method->GetInvokeType()
              << ": " << extra << " DexFile: "
              << mcr::McrCC::PrettyDexFile(
                  resolved_method->GetDexFile()->GetLocation())
              << "\nORIG: MethodID:" << dex_method_idx << " DexLoc:"
              << mcr::McrCC::PrettyDexFile(dex_file->GetLocation());

  MethodReference method_ref(resolved_method->GetDexFile(),
                             resolved_method->GetDexMethodIndex());

  return resolved_method;
}

inline mirror::Class* HGraphToLLVM::ResolveClass(
    const DexFile* dex_file, uint16_t dex_type_idx) {
  Thread* self = Thread::Current();
  ScopedObjectAccess soa(self);
  StackHandleScope<4> hs(soa.Self());

  Handle<mirror::DexCache> dex_cache(hs.NewHandle(
      dcu_.GetClassLinker()->FindDexCache(self, *dex_file)));

  dex::TypeIndex index(static_cast<uint32_t>(dex_type_idx));
  ClassLinker* linker = dcu_.GetClassLinker();
  ObjPtr<mirror::Class> klass = linker->ResolveType(index, dex_cache, dcu_.GetClassLoader());

  return klass.Ptr();
}

inline void HGraphToLLVM::CallClassInitializationCheck(Value* klass) {
  std::vector<Value*> args;
  args.push_back(klass);
  irb_->CreateCall(fh_->GenerateClassInitializationCheck(this, irb_), args);
}

inline DataType::Type _GetFieldType(HInstruction* h) {
  if (h->IsInstanceFieldSet()) {
    return h->AsInstanceFieldSet()->GetFieldInfo().GetFieldType();
  } else if (h->IsStaticFieldSet()) {
    return h->AsStaticFieldSet()->GetFieldInfo().GetFieldType();
  } else if (h->IsInstanceFieldGet()) {
    return h->AsInstanceFieldGet()->GetFieldInfo().GetFieldType();
  } else if (h->IsStaticFieldGet()) {
    return h->AsStaticFieldGet()->GetFieldInfo().GetFieldType();
  } else if (h->AsArrayGet()) {
    return h->AsArrayGet()->GetType();
  } else if (h->AsArraySet()) {
    return h->AsArraySet()->GetType();
  } else {
    DLOG(FATAL) << __func__;
    UNREACHABLE();
  }
}

inline const FieldInfo _GetFieldInfo(HInstruction* h) {
  if (h->IsInstanceFieldSet()) {
    return h->AsInstanceFieldSet()->GetFieldInfo();
  } else if (h->IsStaticFieldSet()) {
    return h->AsStaticFieldSet()->GetFieldInfo();
  } else if (h->IsInstanceFieldGet()) {
    return h->AsInstanceFieldGet()->GetFieldInfo();
  } else if (h->IsStaticFieldGet()) {
    return h->AsStaticFieldGet()->GetFieldInfo();
  } else {
    DLOG(FATAL) << __func__;
    UNREACHABLE();
  }
}

inline bool HGraphToLLVM::FieldValueCanBeNull(HInstruction* h) {
  if (h->IsStaticFieldSet()) {
    return h->AsStaticFieldSet()->GetValueCanBeNull();
  } else if (h->IsInstanceFieldSet()) {
    return h->AsInstanceFieldSet()->GetValueCanBeNull();
  }
  DLOG(FATAL) << __func__;
  UNREACHABLE();
}

inline bool HGraphToLLVM::IsFieldStatic(HInstruction* h) {
  return (h->IsStaticFieldGet() || h->IsStaticFieldSet());
}

}  // namespace LLVM
}  // namespace art

#endif  // ART_COMPILER_LLVM_HGRAPH_TO_LLVM_INL_H_
