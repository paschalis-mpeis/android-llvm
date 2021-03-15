#!/bin/bash
#
# This is based on AOSP
# Copyright (C) 2018 The Android Open Source Project
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LLVM_OUT_DIR="$HOME/workspace/llvm-3.8.1.src"
LLC="$LLVM_OUT_DIR/build/bin/llc"

SCRIPTDIR=`dirname "$0"`
cd "${SCRIPTDIR}/.."

mkdir -p generated

TMP_FILE=generated/tmp.cc
OUTPUT_FILE=generated/art_module.cc

echo "// Generated with ${0}" > ${OUTPUT_FILE}

echo '

#pragma GCC diagnostic ignored "-Wframe-larger-than="
#pragma GCC diagnostic ignored "-Wunused-variable"
// TODO: Remove this pragma after llc can generate makeLLVMModuleContents()
// with smaller frame size.

#include <llvm/IR/Argument.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

#include <vector>

using namespace ::llvm;

namespace art {
namespace LLVM {

' >> ${OUTPUT_FILE}


# Fixes explanation:
# sed.1:
# In llvm3.8 we have to use Attribute for arguments
# sed.2:
# in llvm8 we are using AttributeList in place of AttributeSet
# sed.3:
# AllocaInst has introduced a second parameter: AddressSpace

$LLC -march=cpp -cppgen=contents art_module.ll -o - >> ${TMP_FILE}
# Result: Argument* ptr_this = &*args++;
cat ${TMP_FILE} \
    | sed 's|\(^.*\)\(Value\*\)\(.*\)\(args\+\+;\)|\1Argument\*\3\&\*\4|g' \
    | sed 's/AttributeSet/AttributeList/g' \
    | sed 's|\(^.*AllocaInst(.*\)\(,.*\)\(,.*);$\)|\1, \0\2\3|g' \
    | sed 's|\(^.*setAlignment(\)\([0-9][0-9]*\)\().*$\)|\1Align(\2)\3|g' \
    >> ${OUTPUT_FILE}

# set alignment
## 1: *setAlignment(
## 2: <integer of alignment)
## 3: )*

rm ${TMP_FILE}

echo '
} // namespace LLVM
} // namespace art' >> ${OUTPUT_FILE}


echo "Generated art_module.cc with LLVM api code"
