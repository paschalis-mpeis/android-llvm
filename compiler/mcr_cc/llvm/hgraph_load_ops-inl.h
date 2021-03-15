/**
 * Several load operations for runtime interactions: 
 *    - from offsets
 *    - different sizes (e.g., word, halfword)
 *    - using memory fences
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
#ifndef ART_COMPILER_LLVM_HGRAPH_TO_LLVM_LOAD_OPS_INL_H_
#define ART_COMPILER_LLVM_HGRAPH_TO_LLVM_LOAD_OPS_INL_H_ value

#include "hgraph_to_llvm.h"
#include "hgraph_to_llvm-inl.h"

using namespace ::llvm;
namespace art {
namespace LLVM {

template <bool ptr, bool acquire>
inline Value* HGraphToLLVM::LoadWord(
    Value* reg, Value* loffset, std::string desc) {
  std::vector<Value*> args;
  args.push_back(loffset);
  Value* gep = irb_->CreateInBoundsGEP(reg, args);
  gep->setName(reg->getName().str()+desc);

  // i8*
  Type* wordTy = IntegerType::get(irb_->getContext(), 8)->getPointerTo();
  // i8**
  Value* cast = irb_->CreateBitCast(gep, wordTy->getPointerTo());
  // loaded word: i8*

  Value* word = nullptr;
  if(acquire) {
    DLOG(FATAL) << __func__ << ": use inline asm";
    // INFO __atomic_load wont be resolved
    // word = new LoadInst(wordTy, cast, "loadAcquire",
    //     false/*volatile*/, Align(), AtomicOrdering::Acquire,
    //     SyncScope::System, irb_->GetInsertBlock());
  } else {
    word =irb_->CreateLoad(wordTy, cast);
  }

  if(ptr) {
    word=irb_->CreatePtrToInt(word, irb_->getJIntTy());
    word=irb_->CreateIntToPtr(word, irb_->getJByteTy()->getPointerTo());
  } else {
    word=irb_->CreatePtrToInt(word, irb_->getJIntTy());
  }
  return word;
}

template <bool ptr, bool acquire>
inline Value* HGraphToLLVM::LoadWord(Value* reg, uint32_t offset) {
  std::stringstream ss; ss << std::hex << offset;
  return LoadWord<ptr>(
      reg, irb_->getJUnsignedInt(offset), ss.str());
}

template<bool ptr>
inline Value* HGraphToLLVM::LoadHalfWord(
    Value* reg, Value* loffset, std::string desc) {
  std::vector<Value*> args;
  args.push_back(loffset);
  Value* gep = irb_->CreateInBoundsGEP(reg, args);
  gep->setName(reg->getName().str()+desc);

  // i8*
  Type* ptrTY = IntegerType::get(irb_->getContext(), 8)->getPointerTo();
  // i8**
  Value* cast = irb_->CreateBitCast(gep,  ptrTY->getPointerTo());
  // loaded word: i8*
  Value* loaded =irb_->CreateLoad(ptrTY, cast);

  if(ptr) {
    loaded=irb_->CreatePtrToInt(loaded, irb_->getInt16Ty());
    loaded=irb_->CreateIntToPtr(loaded, ptrTY->getPointerTo());
  } else {
    loaded=irb_->CreatePtrToInt(loaded, irb_->getInt16Ty());
  }
  return loaded;
}

template<bool ptr>
inline Value* HGraphToLLVM::LoadHalfWord(Value* reg, uint32_t offset) {
  std::stringstream ss; ss << std::hex << offset;
  return LoadHalfWord<ptr>(
      reg, irb_->getJUnsignedInt(offset), ss.str());
}

template<bool ptr>
inline Value* HGraphToLLVM::LoadByte(
    Value* reg, Value* loffset, std::string desc) {
  std::vector<Value*> args;
  args.push_back(loffset);
  Value* gep = irb_->CreateInBoundsGEP(reg, args);
  gep->setName(reg->getName().str()+desc);

  // i8*
  Type* ptrTY = IntegerType::get(irb_->getContext(), 8)->getPointerTo();
  // i8**
  Value* cast = irb_->CreateBitCast(gep,  ptrTY->getPointerTo());
  // loaded word: i8*
  Value* loaded =irb_->CreateLoad(ptrTY, cast);

  if(ptr) {
    loaded=irb_->CreatePtrToInt(loaded, irb_->getInt8Ty());
    loaded=irb_->CreateIntToPtr(loaded, ptrTY->getPointerTo());
  } else {
    loaded=irb_->CreatePtrToInt(loaded, irb_->getInt8Ty());
  }
  return loaded;
}

template<bool ptr>
inline Value* HGraphToLLVM::LoadByte(Value* reg, uint32_t offset) {
  std::stringstream ss; ss << std::hex << offset;
  return LoadByte<ptr>(
      reg, irb_->getJUnsignedInt(offset), ss.str());
}

template <bool ptr, bool acquire>
inline Value* HGraphToLLVM::Load(
    Value* reg, Value* loffset, std::string desc) {
  std::vector<Value*> args;
  args.push_back(loffset);
  Value* gep = irb_->CreateInBoundsGEP(reg, args);
  gep->setName(reg->getName().str()+desc);

  Type* loadTy=nullptr;
  if(ptr) {
    loadTy=IntegerType::get(irb_->getContext(), 8)->getPointerTo();
  } else {
    loadTy=IntegerType::get(irb_->getContext(), 8);
  }
  Value* cast = irb_->CreateBitCast(gep,  loadTy->getPointerTo());

  Value* value=nullptr;
  if(acquire) {
    DLOG(FATAL) << __func__ << ": use inline asm";
    // because __atomic_load wont be resolved
    // word = new LoadInst(wordTy, cast, "loadAcquire",
    //     false/*volatile*/, Align(), AtomicOrdering::Acquire,
    //     SyncScope::System, irb_->GetInsertBlock());
    // value=new LoadInst(loadTy, cast, "loadAcquire",
    //     false/*volatile*/, Align(), AtomicOrdering::Acquire,
    //     SyncScope::System, irb_->GetInsertBlock());
  } else {
    value=irb_->CreateLoad(loadTy, cast);
  }
  return value;
}

template<bool ptr, bool acquire>
inline Value* HGraphToLLVM::Load(Value* reg, uint32_t offset) {
  std::stringstream ss; ss << std::hex << offset;
  return Load<ptr, acquire>(reg, irb_->getJUnsignedInt(offset), ss.str());
}

inline LoadInst* HGraphToLLVM::LoadFromObjectOffset(
    Value* object_addr, int64_t offset, Type* type) {
  // Convert offset to value
  Value* llvm_offset = irb_->getPtrEquivInt(offset);
  // Calculate the value's address
  Value* value_addr = irb_->CreatePtrDisp(object_addr, llvm_offset, type->getPointerTo());

  if (McrDebug::DebugInvokeJni()) {
    std::vector<Value*> prt_args;
    Value* fmt =
        irb_->mCreateGlobalStringPtr("LoadFromObjectOffset: %p\n");
    prt_args.push_back(irb_->AndroidLogSeverity(mcr::INFO));
    prt_args.push_back(fmt);
    prt_args.push_back(value_addr);
    irb_->CreateCall(fh_->AndroidLog(), prt_args);
  }

  return irb_->CreateLoad(value_addr);
}

inline Value* HGraphToLLVM::LoadFromAddress16(Value* addr,
                                              uint16_t offset,
                                              Type* type) {
  std::vector<Value*> voffset;
  voffset.push_back(irb_->getJUnsignedInt16(offset));

  Value* gep = irb_->CreateInBoundsGEP(addr, voffset);
  Value* cast = irb_->CreateBitCast(gep, type);
  Value* value = irb_->CreateLoad(cast);
  return value;
}

inline Value* HGraphToLLVM::LoadFromAddress(
    Value* addr, uint32_t offset, Type* type) {
  std::vector<Value*> voffset;
  voffset.push_back(irb_->getJUnsignedInt(offset));

  Value* gep = addr;
  if (offset != 0) {
    gep = irb_->CreateInBoundsGEP(addr, voffset);
  }

  Value* cast = irb_->CreateBitCast(gep, type);
  Value* value = irb_->CreateLoad(cast);
  if(McrDebug::DebugLlvmCode2()) {
    irb_->AndroidLogPrintHexAndInt(INFO, "LoadFromAddress (CHECK)", value);
  }
  return value;
}

}  // namespace LLVM
}  // namespace art

#endif  // ifndef ART_COMPILER_LLVM_HGRAPH_TO_LLVM_LOAD_OPS_INL_H_
