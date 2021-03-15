/**
 * Header required for the llvm/tools/plugin_code.cc generated bitcode by the LLVM backend.
 * Basically it includes any structures that we can try to use directly
 * from libart.
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

#ifndef ART_INTERFACE_H_
#define ART_INTERFACE_H_

#include "mcr_rt/art_impl.h"
#include "mcr_rt/mcr_dbg.h"

#include "jni/jni_env_ext-inl.h"
#include "jni/jni_env_ext.h"
#include "jvalue.h"
#include "jvalue-inl.h"
#include "mirror/object.h"
#include "mirror/object_reference.h"

// Copied from stack.h
template <class MirrorType>
class MANAGED StackReference : public art::mirror::CompressedReference<MirrorType> {
};

#endif  // ART_INTERFACE_H_
