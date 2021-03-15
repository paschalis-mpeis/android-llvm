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

# pushes to phone through SD card to solve permissions issue
# then restore permissions based on the previous permissions of the file
# or the parent folder if it did not exist 
if [ $# -ne 2 ]; then
  echo "Usage: $0 <src> <dest>"
  exit 1
fi
src="$1"
dest="$2"

pushFilename="$(basename -- $src)"
destFilename="$(basename -- $dest)"

# echo "SRC: $src"
# echo "DEST: $dest"
# echo "pushFile: $pushFilename"
# echo "destFile: $destFilename"

if adb_file_exists $dest; then
    permFile=$dest
else
    # if file did not exist, then use
    # permissions of the parent folder

    # remove trailing / if exists
    t=${dest%/}
    # remove filename/foldername
    t=${t%"$destFilename"}

    permFile=$t
    if ! adb_file_exists $permFile; then
        echo "PermFile does NOT exist: $permFile"
        exit 1
    fi
fi

userGroup=$(adb_get_user_group $permFile)

adb push $src $SDCARD

$ADB_SU mv $SDCARD/$pushFilename $dest

$ADB_SU chown -R $userGroup $dest
$ADB_SU chmod -R  664 $dest
