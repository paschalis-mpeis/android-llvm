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

# DBG=1

if [ $HAS_ADB_ROOT -eq 1 ]; then
    # no need for su at all (given adb root has ran at least once)
    ADB_SU="adb shell"
else
    ADB_SU="adb shell su -c"
fi

function adb_get_app_data_dir() {
  $ADB_SU "\"cd /data/app/$PKG-* && pwd\""
}

function adb_get_user_group() {
    # Get user and group of the given file/folder
    if [ $# -ne 1 ]; then
        echo "Usage: $0:adb_get_user_group <file>"
        exit 1
    fi
    file=$1
    user=$($ADB_SU stat -c '%U' $file)
    group=$($ADB_SU stat -c '%G' $file)
    echo $user:$group
}

function adb_get_user() {
    # Get user and group of the given file/folder
    if [ $# -ne 1 ]; then
        echo "Usage: $0:adb_get_user_group <file>"
        exit 1
    fi
    file=$1
    user=$($ADB_SU stat -c '%U' $file)
    echo $user
}

function adb_file_exists() {
    TRUE=0
    FALSE=1
    if [ $# -ne 1 ]; then
        echo "Usage: $0:adb_file_exists <file>"
        exit 1
    fi
    file=$1

    checkFile=$($ADB_SU ls $file &> /dev/null && echo 0)

    if [ "$checkFile" == "0" ] ; then 
        return $TRUE
    else
        return $FALSE
    fi
}

function adb_is_file_symlink() {
    TRUE=0
    FALSE=1

    file=$1
    cmd="if [ -L \"$file\" ]; then echo 1; else  echo 0; fi"
    checkSymlink=$(adb shell $cmd)

    if [ "$checkSymlink" == "1" ] ; then 
        return $TRUE
    else
        return $FALSE
    fi
}

function kill_app() {
    echo "Killing app: $PKG"
    adb shell am force-stop $PKG
}

function enable_option() {
  file=$1
  $ADB_SU" touch "$file
  fix_permissions 774 $file
}
function disable_option() {
  file=$1
  $ADB_SU" rm "$file
}

function fix_permissions() {
  permCode=$1
  file=$2

  # get user and group
  usr=$(adb_get_user $dataDataDir)
  #grp=$(adb_get_group_mcr_app)
  usrGrp="$usr:$usr"

  $ADB_SU "chown $usrGrp $file"
  $ADB_SU "chmod $permCode $file"
}

function fix_oat_permissions() {
  file=$1

  usr=$(adb_get_user $dataDataDir)
  usrGrp="system:$usr"

  $ADB_SU "chown $usrGrp $file"
}

function dbg() {
  msg="$@"
  if [[ $DBG -eq 1 ]]; then
    echo $msg
  fi
}

function link_oat_files() {
  echo "Linking new oat files.."
  # (here assuming that all 3 files are symlinked: .art, .odex, .vdex)
  if adb_is_file_symlink $BASEart; then
    dbg "Already symlinked. Updated oat files at:"
    dbg "$OAT"
    dbg ""
  else
    dbg "Creating symlinks for oat files at:"
    dbg "$OAT"
    dbg ""
    # move the files
    $ADB_SU mv $BASEart $ORIGart
    $ADB_SU mv $BASEodex $ORIGodex
    $ADB_SU mv $BASEvdex $ORIGvdex

    # symlink to LLVM versions:
    $ADB_SU ln -s $LLVMart $BASEart
    $ADB_SU ln -s $LLVModex $BASEodex
    $ADB_SU ln -s $LLVMvdex $BASEvdex
fi

# fix permissions:
fix_oat_permissions $LLVMart
fix_oat_permissions $LLVModex
fix_oat_permissions $LLVMvdex
}

# SETTING SOME VARS:
dataDataDir=/data/data/$PKG
appDataDir=$(adb_get_app_data_dir)

OAT=$appDataDir/oat/$DEVICE_ARCH
BASEart=$OAT"/base.art"
BASEodex=$OAT"/base.odex"
BASEvdex=$OAT"/base.vdex"

LLVMart=$OAT"/base.llvm.art"
LLVModex=$OAT"/base.llvm.odex"
LLVMvdex=$OAT"/base.llvm.vdex"

ORIGart=$OAT"/base.orig.art"
ORIGodex=$OAT"/base.orig.odex"
ORIGvdex=$OAT"/base.orig.vdex"

src_opt_bc="$DIR_SRC/hf.lnk.dce.bc"
src_hgraph="$DIR_SRC/hgraph.ssa.txt"
src_dalvik_bytecode="$DIR_SRC/bytecode.txt"
