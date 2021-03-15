#!/bin/bash
toolDir=$(pwd)
AOSP_REL_DIR=../../../../..
BC_FILENAME=art_module_full.bc

echo "Generating $BC_FILENAME"

# hardcoded for darwin:
CXX=prebuilts/clang/host/darwin-x86/clang-r353983c/bin/clang++

cd $AOSP_REL_DIR && $CXX -nostdlibinc -Ibionic/libc/private -Iart/compiler -target aarch64-linux-android -march=armv8-a -mcpu=kryo -Isystem/core/base/include -Iart/runtime -Iart/libdexfile -Iart/libartbase -Iexternal/libcxx/include -Ibionic/libc/include -isystem bionic/libc/include -isystem bionic/libc/kernel/uapi -isystem bionic/libc/kernel/uapi/asm-arm64 -isystem bionic/libc/kernel/android/scsi -isystem bionic/libc/kernel/android/uapi -Ilibnativehelper/include_jni -DART_STACK_OVERFLOW_GAP_arm=8192 -DART_STACK_OVERFLOW_GAP_arm64=8192 -DART_STACK_OVERFLOW_GAP_mips=16384 -DART_STACK_OVERFLOW_GAP_mips64=16384 -DART_STACK_OVERFLOW_GAP_x86=8192 -DART_STACK_OVERFLOW_GAP_x86_64=819 -DART_DEFAULT_GC_TYPE_IS_CMS -DIMT_SIZE=43 -DMCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS -Wno-invalid-offsetof -std=gnu++17 art/compiler/mcr_cc/llvm/tools/plugin_code.cc -c -O0 -S -emit-llvm -o $toolDir/$BC_FILENAME
# -fno-builtin-memcpy -fno-builtin-memset

# TODO clear also this? @llvm.lifetime.start
