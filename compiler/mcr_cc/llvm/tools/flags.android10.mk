CFLAGS:=
CPPFLAGS:=-std=gnu++17

ifeq ($(TARGET_ARCH),arm64)
ARCH_FLAGS_COMMON:=-march=armv8-a -mcpu=kryo
ARCH_FLAGS:=$(ARCH_FLAGS_COMMON) -fsplit-lto-unit 
ARCH_INC:=-isystem $(phd_aosp)/bionic/libc/kernel/uapi/asm-arm64
else ifeq ($(TARGET_ARCH),arm)
CPPFLAGS:=-std=gnu++11
ARCH_FLAGS:=-msoft-float -march=armv8-a -mfloat-abi=softfp -mfpu=neon-fp-armv8 -mcpu=cortex-a53 -D__ARM_FEATURE_LPAE=1 -mthumb
#  -mthumb-interwork

ARCH_INC:=-isystem $(phd_aosp)/bionic/libc/kernel/uapi/asm-arm -isystem bionic/libc/kernel/android/scsi
# CLR_ARM64: -marm 
endif

ANDROID_FLAGS:=-fno-exceptions -ffunction-sections -fdata-sections -funwind-tables -fno-short-enums  -no-canonical-prefixes -fvisibility-inlines-hidden -fmessage-length=0 -fno-strict-aliasing -fno-rtti -fvisibility=protected -fPIC 

EXTRA_FLAGS:=-faddrsig -fcolor-diagnostics -nostdlibinc

# Necessary headers to fix Catalina NDK issues
INC_AOSP=-I$(phd_aosp)/system/core/liblog/include -isystem $(phd_aosp)/bionic/libc/include -isystem $(phd_aosp)/bionic/libc/kernel/uapi -isystem $(phd_aosp)/bionic/libc/kernel/android/uapi $(ARCH_INC)

# EXTRA_EXCLUDED:=
# -fstrict-aliasing
# LINKER
# -Wa,--noexecstack
# -fstack-protector-strong: will cause issues w/ replaying
#  (we have set no stack protector)

ANDROID_FLAGS+=$(ARCH_FLAGS) $(EXTRA_FLAGS) $(INC_AOSP) -DART_MCR_ANDROID_10

# REMOVED FLAGS:
# -msoft-float arm only
# -fno-canonical-system-headers # removed, unknown argument
# -mcpu=cortex-a15 ARCH SPECIFIC
# -D__ARM_FEATURE_LPAE=1 arm
# -mfloat-abi=softfp arm
# -mfpu=neon arm (used anyway/mandatory for arm64 so no needed)
# -fno-builtin-sin removed
# -fno-strict-volatile-bitfields removed, unknown argument
# -fgcse-after-reload removed, not supported
# -frerun-cse-after-loop removed, unknown argument

# clang++: error: -llog: 'linker' input unused [-Werror,-Wunused-command-line-argument]
