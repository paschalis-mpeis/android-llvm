AOSP_REL_DIR=../../../../..
MODULE_FULL=art_module_full
MOD_BC=$(MODULE_FULL).bc
MOD_LL=$(MODULE_FULL).ll

INC=-I$(AOSP_REL_DIR)/art/runtime
INC+=-I$(AOSP_REL_DIR)/art/compiler

# THE SAME LLVM version of opt and lllvm-dis tools have to be used
# tested on 9.0.1. Then some fixes are applied on the disassembled bitcode,
# which is finally passed to an obsolete llvm versin (3.8) that has the
# C++ backend enabled
LLVM_DIR=/usr/local/Cellar/llvm@9/9.0.1_2/bin
LLVM_DIS=$(LLVM_DIR)/llvm-dis
LLVM_OPT=$(LLVM_DIR)/opt 

# android6
# CXX=~/android/toolchains/clang/bin/arm-linux-androideabi-clang++
# android10
# CXX=aarch64-linux-android29-clang++
CXX=$(AOSP_REL_DIR)/prebuilts/clang/host/darwin-x86/clang-r353983c/bin/clang++

DEFINES=-D__arm__ -DMCR_LLVM_GEN_INVOKE_HIST_ON_CACHE_MISS
DEFINES+=-D__INCLUDED_FROM_LLVM_PLUGIN_CODE -DART_MCR -DCRDEBUG2


# @Android 6
# CFLAGS=-std=c++11 $(DEFINES)
CFLAGS=-std=gnu++17 $(DEFINES) -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CFLAGS+=-march=armv8-a -mcpu=kryo -fPIC -fsplit-lto-unit -DANDROID -D__LIBC_API__=10000 -D__LIBM_API__=10000 -D__LIBDL_API__=10000 -D_USING_LIBCXX -fno-exceptions -ffunction-sections -fdata-sections -funwind-tables -fno-short-enums  -no-canonical-prefixes -fvisibility-inlines-hidden -fmessage-length=0 -fno-strict-aliasing -fno-rtti -fvisibility=protected -faddrsig -fcolor-diagnostics -nostdlibinc


TARGET_ARCH=arm64
include flags.android10.mk
CFLAGS=-std=gnu++17 -target aarch64-linux-android $(ANDROID_FLAGS)
.PHONY: compile_android10

android10: compile_android10
	echo $(LLVM_DIS) $(MOD_BC)
	$(LLVM_OPT) -instnamer $(MOD_BC) -o $(MOD_BC)
	$(LLVM_DIS) $(MOD_BC)
	./fix.ll.android10.sh
	echo "\n\nTo generate LLVM backend code:\n1.Clear HELPER method in ../*.ll\n2. Run: ./gen_art_module_cc"

compile_android10:
	./make.android10.sh

android6:
	$(CXX) $(CFLAGS) $(INC) -emit-llvm plugin_code.cc -c -o $(MOD_BC)
	# opt -instname
	$(LLVM_DIS) $(MOD_BC)	
	@echo "Remove manually HELPER_ methods from $(MOD_BC) and execute ./gen_art_module_cc.sh"
	@echo "Also remove lines 1-3 in .ll file"
