/**
 * Wrapper methods for runtime offets based on the compiling architecture.
 * 
 * NOTE: the code was developed for ARM64 and on Android10.
 * While there is ARM code (32bit) it is not supported
 * (even though we implemented a working version on Android6/ARM/LLVMv3.6)
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

#include "art_method-inl.h"
#include "gc/space/image_space.h"
#include "llvm_macros_irb_.h"


using namespace ::llvm;
namespace art {
namespace LLVM {

uint32_t HGraphToLLVM::GetArtMethodEntryPointFromJniOffset() {
  InstructionSet isa(GetISA());
  switch (isa) {
    case InstructionSet::kArm64:
      return ArtMethod::EntryPointFromJniOffset(
          kArm64PointerSize).Int32Value();
    case InstructionSet::kArm:
    case InstructionSet::kThumb2:
      return ArtMethod::EntryPointFromJniOffset(
          kArmPointerSize).Int32Value();
    default:
      DIE_UNIMPLEMENTED_ARCH(isa);
  }
}

uint32_t HGraphToLLVM::GetThreadTopShadowFrameOffset() {
  InstructionSet isa(GetISA());
  switch (isa) {
    case InstructionSet::kArm64:
      return Thread::TopShadowFrameOffset<kArm64PointerSize>().Int32Value();
    case InstructionSet::kArm:
    case InstructionSet::kThumb2:
      return Thread::TopShadowFrameOffset<kArmPointerSize>().Int32Value();
    default:
      DIE_UNIMPLEMENTED_ARCH(isa);
  }
}

uint32_t HGraphToLLVM::GetThreadTopOfManagedStackOffset() {
  InstructionSet isa(GetISA());
  switch (isa) {
    case InstructionSet::kArm64:
      return Thread::TopOfManagedStackOffset<kArm64PointerSize>().Int32Value();
    case InstructionSet::kArm:
    case InstructionSet::kThumb2:
      return Thread::TopOfManagedStackOffset<kArmPointerSize>().Int32Value();
    default:
      DIE_UNIMPLEMENTED_ARCH(isa);
  }
}

uint32_t HGraphToLLVM::GetThreadExceptionOffset() {
  InstructionSet isa(GetISA());
  switch (isa) {
    case InstructionSet::kArm64:
      return Thread::ExceptionOffset<kArm64PointerSize>().SizeValue();
    case InstructionSet::kArm:
    case InstructionSet::kThumb2:
      return Thread::ExceptionOffset<kArmPointerSize>().SizeValue();
    default:
      DIE_UNIMPLEMENTED_ARCH(isa);
  }
}

uint32_t HGraphToLLVM::GetThreadFlagsOffset() {
  InstructionSet isa(GetISA());
  switch (isa) {
    case InstructionSet::kArm64:
      return Thread::ThreadFlagsOffset<kArm64PointerSize>().SizeValue();
    case InstructionSet::kArm:
    case InstructionSet::kThumb2:
      return Thread::ThreadFlagsOffset<kArmPointerSize>().SizeValue();
    default:
      DIE_UNIMPLEMENTED_ARCH(isa);
  }
}

uint32_t HGraphToLLVM::GetThreadJNIEnvOffset() {
  InstructionSet isa(GetISA());
  switch (isa) {
    case InstructionSet::kArm64:
      return Thread::JniEnvOffset<kArm64PointerSize>().Int32Value();
    case InstructionSet::kArm:
    case InstructionSet::kThumb2:
      return Thread::JniEnvOffset<kArmPointerSize>().Int32Value();
    default:
      DIE_UNIMPLEMENTED_ARCH(isa);
  }
}

static uint32_t GetBootImageOffsetImpl(const void* object, ImageHeader::ImageSections section) {
  Runtime* runtime = Runtime::Current();
  DCHECK(runtime->IsAotCompiler());
  const std::vector<gc::space::ImageSpace*>& boot_image_spaces =
      runtime->GetHeap()->GetBootImageSpaces();
  // Check that the `object` is in the expected section of one of the boot image files.
  DCHECK(std::any_of(boot_image_spaces.begin(),
                     boot_image_spaces.end(),
                     [object, section](gc::space::ImageSpace* space) {
                       uintptr_t begin = reinterpret_cast<uintptr_t>(space->Begin());
                       uintptr_t offset = reinterpret_cast<uintptr_t>(object) - begin;
                       return space->GetImageHeader().GetImageSection(section).Contains(offset);
                     }));
  uintptr_t begin = reinterpret_cast<uintptr_t>(boot_image_spaces.front()->Begin());
  uintptr_t offset = reinterpret_cast<uintptr_t>(object) - begin;
  return dchecked_integral_cast<uint32_t>(offset);
}

// NO_THREAD_SAFETY_ANALYSIS: Avoid taking the mutator lock, boot image classes are non-moveable.
uint32_t HGraphToLLVM::GetBootImageOffset(HLoadClass* load_class)
  NO_THREAD_SAFETY_ANALYSIS {
    ObjPtr<mirror::Class> klass = load_class->GetClass().Get();
    return GetBootImageOffsetImpl(klass.Ptr(), ImageHeader::kSectionObjects);
}

// NO_THREAD_SAFETY_ANALYSIS: Avoid taking the mutator lock, boot image strings are non-moveable.
uint32_t HGraphToLLVM::GetBootImageOffset(HLoadString* load_string) NO_THREAD_SAFETY_ANALYSIS {
  DCHECK_EQ(load_string->GetLoadKind(), HLoadString::LoadKind::kBootImageRelRo);
  ObjPtr<mirror::String> string = load_string->GetString().Get();
  DCHECK(string != nullptr);
  return GetBootImageOffsetImpl(string.Ptr(), ImageHeader::kSectionObjects);
}

}  // namespace LLVM
}  // namespace art

