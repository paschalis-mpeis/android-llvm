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

if [ $# -ne 2 ]; then
  echo "Usage: $0 <src> <dest>"
  exit 1
fi
src="$1"
dest="$2"

pullFilename="$(basename -- $src)"

if adb_file_exists $src; then
# userGroup=$(adb_get_user_group $permFile)
    adb shell mkdir -p $SDCARD/.tmp/
    $ADB_SU cp -R $src $SDCARD/.tmp/$pullFilename
    adb pull $SDCARD/.tmp/$pullFilename $dest
    adb shell rm -r $SDCARD/.tmp/
else
    echo "ERROR: File does not exist: $src"
    exit 1
fi
