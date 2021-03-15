/**
 * Some utilities requiring the art/runtime/class_linker.h
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
#include "mcr_cc/linker.h"

#include "class_linker-inl.h"
#include "thread.h"

namespace art {
namespace mcr {

mirror::Class* Linker::ResolveClass(jobject jclass_loader, const DexFile* dex_file,
                                    const dex::ClassDef& class_def) {
  Thread* self = Thread::Current();
  ScopedObjectAccess soa(self);
  StackHandleScope<3> hs(soa.Self());
  Handle<mirror::ClassLoader> class_loader(hs.NewHandle(
        soa.Decode<mirror::ClassLoader>(jclass_loader)));


  ClassLinker* class_linker = Runtime::Current()->GetClassLinker();

  const char* class_desc = dex_file->GetClassDescriptor(class_def);
  ObjPtr<mirror::Class> klass(class_linker->FindClass(self, class_desc, class_loader));

  if (self->IsExceptionPending()) {
    self->ClearException();
  }

  return klass.Ptr();
}



}  // namespace mcr
}  // namespace art
