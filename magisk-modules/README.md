# LLVM Magisk Modules
The below magisk modules are required for overriding/extending components in the `/system/` for utilizing LLVM backend.
The following components should be compiled from the rest of the repositories, or from some prebuilts,
and put in the below locations.

These were tested on a Sailfish and a Pixel device.

## MAGISK Installation:
The runtime components are packed into special signed structures, named APEXes.
Each APEX is mounted on device boot, and might be optionally renamed to a particular mount point (see below).
To override components (libs and bins) in these apexes we use the Magisk project,
since the `/system` filesystem is read-only on Android10.
Magisk essentially overlays specific components on top of the `/system` without
directly modifying it.

APEX module mounting works on some devices (Pixel 1st Gen/Sailfish),
but on some others does not (Pixel 4th gen/Flame).

For unsupported devices (i.e., Flame) the procedure that has to be done is as follows:

### 1. Magisk should be installed using the  [OFFICIAL GUIDE](https://topjohnwu.github.io/Magisk/install.html)
Once this is done, it is **important** to grant root access to any tools/apps that migh need it.  
In our demo case, this will be the shell, e.g.,
```bash
adb shell
$ su
# .. here we have root access..
```
### 2. Install our custom Magisk module version [REPO](https://github.com/Paschalis/Magisk)
With this version you wont be able to grant root access to new apps, however,
you will retain root access to the previously granted ones (i.e., `adb shell`).  
This custom magisk version allows overriding ART APEX components on devices like Flame.
Hopefully this will be properly supported by Magisk in the future.

[prebuilt](./prebuilt): is a pre-compiled version of the modified magisk.

### 3. Install the `llvm-static` and `llvm-rt` modules:
For preparing those modules read the instructions below.

Those components will be generated from:

- compilations of:
    + art
    + bionic
    + external/llvmx
- prebuilts from:
    + NDK
    + Clang (external, outside of SOONG)


## MODULE: llvm-static
The below components normally should be compiled once (infrequent updates):

Required Components:

```bash
├── system
│   ├── bin
│   │   ├── clang                        # prebuilt
│   │   ├── ld                           # prebuilt (ld.lld)
│   │   ├── llc                          # compile from: external/llvmx
│   │   ├── llvm-link                    # compile from: external/llvmx
│   │   └── opt                          # compile from: external/llvmx
│   ├── lib
│   │   ├── crtbegin_so.o                # from NDK
│   │   ├── crtend_so.o                  # from NDK
│   │   ├── libLLVM.so                   # compile from: external/llvmx
│   │   └── libc++_shared.so             # from NDK
│   └── lib64
│       ├── crtbegin_so.o                # from NDK
│       ├── crtend_so.o                  # from NDK
│       ├── libLLVM.so                   # compile from: external/llvmx
│       └── libc++_shared.so             # from NDK
```

Relevant permissions are given using customize.sh.
The NDK and prebuilt components are provided for arm and arm64 (where required).  

The NDK components can be copied by an NDK installation something like:
```bash
cp $NDK/sources/cxx-stl/llvm-libc++/libs/arm64-v8a/libc++_shared.so $lib64/
cp $NDK/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a/libc++_shared.so $lib32/

cp $NDK/platforms/android-$SDK_VER/arch-arm64/usr/lib/crtbegin_so.o $lib64/
cp $NDK/platforms/android-$SDK_VER/arch-arm/usr/lib/crtbegin_so.o $lib32/

cp $NDK/platforms/android-$SDK_VER/arch-arm64/usr/lib/crtend_so.o $lib64/
cp $NDK/platforms/android-$SDK_VER/arch-arm/usr/lib/crtend_so.o $lib32/
```

The LLVM prebuilts can be generated from an external Clang compilation,
outside of the AOSP Soong build system for convenience.
As they are standalone, they embed copies of any libraries that they depend upon (i.e., libLLVM).

The remaining of the components (`libLLVM.so`, `llvm-link`, `opt`, `llc`)can be compiled 
using the [external/llvmx](https://github.com/Paschalis/external-llvmx) project.
Afterwards they should be imported in this module, which will have to be flashed on a mobile device.

## MODULE llvm-rt (llvm-xrt):
These are frequently updated during development and are installed by magisk at:  
- /system/apex/com.android.runtime.release  

The above APEX is mounted (and renamed) on boot by android at:  
- /apex/com.android.runtime  

Those components should be installed from the [art](../README.md#art-components) repository

Required Components:
```bash
└── system
    ├── apex
    │   └── com.android.runtime
    │       ├── bin
    │       │   ├── dex2oat                      # compile from: art
    │       │   ├── linker                       # compile from: bionic
    │       │   └── linker64                     # compile from: bionic
    │       ├── lib
    │       │   ├── libart-compiler.so           # compile from: art
    │       │   └── libart.so                    # compile from: art
    │       └── lib64
    │           ├── libart-compiler.so           # compile from: art
    │           └── libart.so                    # compile from: art
```

The `llvm-xrt` variant works for the Pixel sailfish (1st gen) as
the APEX deployment differs on Android 10 on that device.

