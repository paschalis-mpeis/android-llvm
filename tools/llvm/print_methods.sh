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

# Print all the methods that are available in an application

appDataDir=$(adb_get_app_data_dir)

$ADB_SU dex2oat --dex-file=$appDataDir/base.apk \
  --oat-file=$appDataDir/oat/arm64/base.capt.odex \
  --compiler-backend=Optimizing \
  --compiler-filter=everything \
  --mcr-user=$USER_ID --mcr-pkg=$PKG \
  --print-methods

echo "Please check the logcat"
