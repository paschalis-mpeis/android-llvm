#!/bin/sh
#
# Copyright 2021 Paschalis Mpeis
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

source ./config.sh
source ./helper.sh

target=. # current directory

# Pull and disassemble DCE (dead code elim) bitcode file:
file=$src_opt_bc

echo "Pulling bitcode: "$file
./adb_pull.sh $file $target

filename_ext=$(basename $file)

# check if disassembler exists
if [ -z $LLVM_DIS ]; then
  echo "ERROR: llvm-dis not set in config.sh"
  exit 1
elif [ ! -f $LLVM_DIS ]; then
  echo "ERROR: llvm-dis does not exist"
  exit 1
fi

cmd="$LLVM_DIS "$target"/"$filename_ext
eval $cmd
rm $target"/"$filename_ext
echo "Pulled (and decompiled) LLVM bitcode: $filename_ext"

# Pulling HGraph in SSA form:
./adb_pull.sh $src_hgraph $target/ssa.hgraph
echo "Pulled hgraph.ssa"

./adb_pull.sh $src_dalvik_bytecode $target/bytecode.dalvik
echo "Pulled bytecode.dalvik"

