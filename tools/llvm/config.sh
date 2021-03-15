#!/bin/bash

# git update-index --assume-unchanged config.sh

USER_ID=0
PKG=mp.paschalis.llvm.demo

DEVICE_CODENAME=flame
DEVICE_ARCH=arm64
DEVICE_CPU=kryo

# assuming the device is rooted through magisk
# NOTE: root permissions must be granted first, and then
# a modified version of magisk should be installed.
# That version allows overriding runtime components (libart.so, libart-compiler.so, etc)
# of the runtime APEX, but looses the ability to give new apps root permisions..
HAS_ADB_ROOT=0

DIR_MCR=/data/misc/profiles/llvm

SDCARD=/sdcard/Download

# LLVM DISASSEMBLER
## (must be set before using pull_code.sh)
LLVM_DIS=

# hf_ + stripped method name
HOT_REGION=hf_void#mp.paschalis.llvm.demo.TestLoops.Loop5D@@
DIR_APP=$DIR_MCR/$USER_ID/$PKG
DIR_HF=$DIR_APP/$HOT_REGION
DIR_SRC=$DIR_HF"/src"
F_LLVM_CODE=$DIR_APP/llvm.enabled
F_LLVM_PROFILE=$DIR_APP/profile
F_VERIF_LLVM_CALLED=$DIR_MCR"/verif.llvm_called"
