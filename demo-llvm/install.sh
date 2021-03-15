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

APK_FILE_DEBUG="app/build/outputs/apk/app-debug.apk"
APK_FILE="app/build/outputs/apk/release/app-release.apk"

rm -f $APK_FILE_DEBUG
rm -f $APK_FILE
# ./gradlew assembleDebug 
./gradlew clean assembleRelease

if [ ! -f $APK_FILE ]; then
  printf "Installation failed. APK does not exist: $APK\n" 
  exit 1
fi

adb install -r $APK_FILE
