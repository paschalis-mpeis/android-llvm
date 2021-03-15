#!/bin/bash
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

if ! adb_file_exists $DIR_MCR; then
  echo "Initializing LLVM directory: $DIR_MCR"
  $ADB_SU mkdir -p $DIR_MCR/$USER_ID
  $ADB_SU chmod -R  777 $DIR_MCR
  $ADB_SU chown -R  system:system $DIR_MCR
fi

if ! adb_file_exists $DIR_APP; then
  echo "Initializing APP directory: $PKG"
  $ADB_SU mkdir -p $DIR_APP
  $ADB_SU chmod -R  777 $DIR_MCR
  $ADB_SU chown -R  system:system $DIR_MCR
fi

echo "Pushing LLVM profile.."
./adb_push.sh profile $F_LLVM_PROFILE

echo "Generating LLVM bitcode.."
$ADB_SU dex2oat --dex-file=$appDataDir/base.apk \
  --oat-file=$appDataDir/oat/arm64/base.llvm.odex \
  --compiler-backend=Optimizing \
  --compiler-filter=everything \
  --comp-type=llvm-gen-bitcode \
  --userid=$USER_ID --pkg=$PKG \
  --instruction-set=$DEVICE_ARCH \
  --instruction-set-variant=$DEVICE_CPU \
  --instruction-set-features=default \
  --llvm-gen-invoke-histogram \
  --app-image-file=$appDataDir/oat/$DEVICE_ARCH/base.llvm.art

link_oat_files  # Link the generated files

echo "Assembling bitcode.."
$ADB_SU dex2oat --dex-file=$appDataDir/base.apk \
  --oat-file=$appDataDir/oat/arm64/base.llvm.odex \
  --compiler-backend=Optimizing \
  --compiler-filter=everything \
  --instruction-set=$DEVICE_ARCH \
  --instruction-set-variant=$DEVICE_CPU \
  --instruction-set-features=default \
  --comp-type=llvm-gen-bitcode \
  --comp-type=llvm-base \
  --comp-baseline=-O3 \
  --userid=$USER_ID --pkg=$PKG \
  --app-image-file=$appDataDir/oat/arm64/base.llvm.art

kill_app

echo "Enabling: LLVM mode.."
./enable_llvm.sh
#echo "Enabling: debug log when LLVM runs.."
#./enable.sh verify-llvm-called

echo ""
echo "Execute the demo app!"
