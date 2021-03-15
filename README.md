# LLVM Backend for the Android compiler (ART)

Experimental backend for increasing the code transformation size of the default Android backend.
Was used the below input-driven compiler optimization framework, based on iterative compilation:

PLDI'21:
[Developer and User Transparent Compiler Optimization for Interactive Applications](https://pldi21.sigplan.org/details/pldi-2021-papers/18/Developer-and-User-Transparent-Compiler-Optimization-for-Interactive-Applications)  
Paschalis Mpeis, Pavlos Petoumenos, Kim Hazelwood, Hugh Leather,

```bib
@inproceedings{mpeis2021developer,
  title={Developer and User-Transparent Compiler Optimization for Interactive Applications},
  author={Mpeis, Paschalis and Petoumenos, Pavlos and Hazelwood, Kim and Leather, Hugh},
  booktitle={42nd ACM SIGPLAN International Conference on Programming Language Design and Implementation},
  year={2021},
  organization={ACM Digital Library}
}
```

## A. Code structure:
All modified/additional files can be found here: [FILES.md](./FILES.md).

## B. AOSP Compilations:

Several components must be compiled and installed on a mobile device.
The compilation is explained below.  
Once compilation is done, those components
have to be installed through [magisk-modules](./magisk-modules/).


### B1. Setting up the AOSP sources + repositories:

First the sources have to be setup.

##### B1.1 Download AOSP sources:

You might also want to check the official guide.
At the time of writing this README the below worked:

```
repo init -u https://android.googlesource.com/platform/manifest -b android10-release --partial-clone --clone-filter=blob:limit=10M
# (authentication method can also be used)
repo sync -j32
```

##### B1.2 Add new repositories:

The following repositories should be put in the AOSP ROOT directory:

- AOSP ROOT:
    + art (this repository)
    + [bionic](https://github.com/Paschalis/aosp-platform-bionic)
    + [external/llvmx](https://github.com/Paschalis/external-llvmx)

### B2. Compile LLVM Compiler: [external/llvmx](https://github.com/Paschalis/external-llvmx)

`libLLVM.so`, `llvm-link`, `opt`, `llc`, `lld` need to be generated.

See [external/llvmx](https://github.com/Paschalis/external-llvmx) for instructions.

This repository should be placed at:  
`<AOSP-ROOT>/external/llvmx`

### B3 ART Components

##### B3.1 [platform/bionic](https://github.com/Paschalis/aosp-platform-bionic)
Modifications to the linker that will allow loading shared libraries that were compiled by the LLVM backend.
`linker` and `linker64` need to be generated.

This repository should be placed at:  
`<AOSP-ROOT>/bionic`

Compilation:  
```
m linker 2>&1
```

##### B3.2 Compile ART Runtime, Compiler, and Compiler Driver
`libart.so`, `libart-compiler.so`,  and `dex2oat` need to be generated using:

This repository should be placed at:  
`<AOSP ROOT>/art`

Compilation:  
```
m -j16 libart libart-compiler dex2oat 2>&1
```

Additional compilations notes: [COMPILATION.md](./COMPILATION.md)

--- 

## C. Use the LLVM backend:
1. Make sure that every needed component is compiled (see AOSP Compilations).
2. Make sure that these components are [installed](./magisk-modules/) on a supported device (tested mainly on Flame).
3. Then [demo app](./demo-llvm/) should be installed.
4. Finally, the scripts at [tools/llvm](tools/llvm/) can be used for compiling a hot region of the demo app.
