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

# Enable debug options for testing:

if [[ $# -eq 0 ]]; then
  echo "No option given"
  echo "Usage: $0 <option>"
fi

option=$1

## Verify that LLVM was called:
if [[ $option == "verify-llvm-called" ]]; then
  disable_option $F_VERIF_LLVM_CALLED 
  ## enable LLVM code (same as calling enable_llvm.sh)
elif [[ $option == "llvm" ]]; then
  disable_option $F_LLVM_CODE
fi
