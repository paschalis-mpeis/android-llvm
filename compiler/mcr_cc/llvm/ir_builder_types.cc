/**
 * Dalvik type mapping to LLVM types.
 * It also provides some upcasting/downcasting ops, and loads some
 * types from art_module.ll (generated/)
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

Type* IRBuilder::getType(DataType::Type type) {
  return getTypeUpcast(type);
}

Type* IRBuilder::getType(HInstruction* h) {
  D5LOG(INFO) << __func___ << prt_->GetInstruction(h);
  return getType(h->GetType());
}

Type* IRBuilder::getTypeExact(HInstruction* h) {
  return getTypeUpcast(h->GetType());
}

Type* IRBuilder::getTypeFromShorty(char shorty_jty) {
  return getType(DataType::FromShorty(shorty_jty));
}

Type* IRBuilder::getTypeFromShortyJNI(char shorty_jty) {
  Primitive::Type t = Primitive::GetType(shorty_jty);
  if (t == Primitive::kPrimNot) {
    return getJniObjectTy();
  }
  return getTypeFromShorty(shorty_jty);
}

Value* IRBuilder::UpcastInt(Value* value, HInstruction* h) {
  return UpcastInt(value, h->GetType());
}

/**
 * @brief upcast to i32 anything smaller
 *        only for integers
 */
Value* IRBuilder::UpcastInt(Value* value, DataType::Type type) {
  
  if(DataType::IsIntegralType(type) &&
      !DataType::Is64BitType(type)) {
    if(type == DataType::Type::kUint16) {
      DLOG(INFO) << __func___ << "char upcast";
    }
    Type* ltype = getTypeUpcast(type);
    if(IsSigned(type)) {
      value = CreateSExt(value, ltype);
    } else {
      value = CreateZExt(value, ltype);
    }
  }
  return value;
}

/**
 * @brief 
 *
 * INFO we also upcast char, despite that it has
 * specific HTypeConversion operations, to convert it from ui16 to i32
 *
 */
Type* IRBuilder::getTypeUpcast(DataType::Type type) {
  // If integer, get upcast  type
  if(DataType::IsIntegralType(type)) {
    switch (type) {
      case DataType::Type::kBool:
      case DataType::Type::kInt8:
      case DataType::Type::kUint8:
      // Char operations come with extra HTypeConversion
      case DataType::Type::kUint16:
      case DataType::Type::kInt16:
      case DataType::Type::kUint32:
      case DataType::Type::kInt32:
        return getJIntTy();
      case DataType::Type::kUint64:
      case DataType::Type::kInt64:
        return getJLongTy();
      default:
        DLOG(FATAL) << "Unknown davlik type: " << type;
        UNREACHABLE();
    }
  }

  return getTypeExact(type);
}

Type* IRBuilder::getTypeExact(DataType::Type type) {
  switch (type) {
    case DataType::Type::kVoid:
      return getJVoidTy();
    case DataType::Type::kBool:
      return getJBooleanTy();
    case DataType::Type::kInt8:
    case DataType::Type::kUint8:
      return getJByteTy();
    case DataType::Type::kUint16:
      return getJCharTy();
    case DataType::Type::kInt16:
      return getJShortTy();
    case DataType::Type::kUint32:
    case DataType::Type::kInt32:
      return getJIntTy();
    case DataType::Type::kUint64:
    case DataType::Type::kInt64:
      return getJLongTy();
    case DataType::Type::kFloat32:
      return getJFloatTy();
    case DataType::Type::kFloat64:
      return getJDoubleTy();
    case DataType::Type::kReference:
      return getJObjectTy()->getPointerTo();
  }
  DLOG(FATAL) << "Unknown dalvik type: " << type;
  UNREACHABLE();
}

void IRBuilder::GenerateTypeDefinitions() {
  D4LOG(INFO) << "CHECK_LLVM: GenerateTypeDefinitions";
  // JavaObject
  StructType* StructTy_JavaObject = module_->getTypeByName("JavaObject");
  if (!StructTy_JavaObject) {
    StructTy_JavaObject = StructType::create(module_->getContext(), "JavaObject");
  }
  std::vector<Type*> StructTy_JavaObject_fields;
  if (StructTy_JavaObject->isOpaque()) {
    StructTy_JavaObject->setBody(StructTy_JavaObject_fields, /*isPacked=*/false);
  }

  // JObject
  StructType* StructTy_class__jobject = module_->getTypeByName("class._jobject");
  if (!StructTy_class__jobject) {
    StructTy_class__jobject = StructType::create(module_->getContext(), "class._jobject");
  }
  std::vector<Type*> StructTy_class__jobject_fields;
  StructTy_class__jobject_fields.push_back(IntegerType::get(module_->getContext(), 8));
  if (StructTy_class__jobject->isOpaque()) {
    StructTy_class__jobject->setBody(StructTy_class__jobject_fields, /*isPacked=*/false);
  }
  PointerType* PointerTy_49 = PointerType::get(StructTy_class__jobject, 0);

  jni_object_type_ = PointerTy_49;
  CHECK(jni_object_type_ != NULL);
}

void IRBuilder::LoadFromArtModule() {
  INFO4_;

  // Struct: ShadowFrame
  structShadowFrame_ =
    module_->getTypeByName("class.art::ShadowFrame");
  CHECK(structShadowFrame_ != NULL);

  // Struct: ManagedStack
  structManagedStack_ =
    module_->getTypeByName("class.art::ManagedStack");
  CHECK(structManagedStack_ != NULL);

  StructType* StructTy_struct__JNIEnv = nullptr;
#ifdef ART_MCR_ANDROID_10
  StructTy_struct__JNIEnv = module_->getTypeByName("class.art::JNIEnvExt");
#elif defined(ART_MCR_ANDROID_6)
  StructTy_struct__JNIEnv = module_->getTypeByName("struct.art::JNIEnvExt");
#endif
  CHECK(StructTy_struct__JNIEnv != nullptr);

  jenv_type_ = PointerType::get(StructTy_struct__JNIEnv, 0);
  CHECK(jenv_type_ != nullptr);
}


#include "llvm_macros_undef.h"
}  // namespace LLVM
}  // namespace art
