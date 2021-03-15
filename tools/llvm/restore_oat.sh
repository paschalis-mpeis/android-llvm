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

if ! adb_is_file_symlink $BASEart; then
  echo "ERROR: oat files not symlinked at:"
  echo "$OAT"
else
  # delete the symlinks
  $ADB_SU rm $BASEart
  $ADB_SU rm $BASEodex
  $ADB_SU rm $BASEvdex

  # restore the original files
  $ADB_SU mv $ORIGart $BASEart
  $ADB_SU mv $ORIGodex $BASEodex
  $ADB_SU mv $ORIGvdex $BASEvdex

  echo "Restored the default compilation."
fi
