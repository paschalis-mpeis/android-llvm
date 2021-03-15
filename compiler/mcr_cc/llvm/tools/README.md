# Plugin code:
We need an automated way to generate ART's necessary components, like the `JValue` struct.

For this, we use the C++ backend of LLVM3.9 to generate LLVM API code automatically,
instead of manually re-writing relevant parts of the runtime code in LLVM bitcode.
This C++ backend is integrated into the `llc` tool, but since 3.9 it was removed from the sources.
There was also a standalone tool named llvm2cpp.
Since no recent version has such backend, and the LLVM bitcode has undergone changes,
we need to update some of the generated LLVM IR to make it compatible with LLVM v10.

I think it's better to completely remove this dependency, and re-write everything using the LLVM API.

The generated code is called plugin code and is stored at `art_module_full.ll`.
To generate it the `plugin_code.cc` goes through the `llc`'s C++ backend.
Then the bitcode is copied to `../art_module.ll`, and the `HELPER` method in it can be manually cleared out.

# Preparation:
### Compile and install `llc` with the `cpp` backend on host
* Tested on : MacOS

1. Download `llvm-3.8.1` from the [releases](https://releases.llvm.org/)  
2. Extract in `<dir>/llvm-src`  
3. `cd llvm-3.8.1`  
4. `mkdir build && cd build`  
5. `cmake -DLLVM_TARGETS_TO_BUILD="CppBackend" ..`  
6. Build it:  
```
cmake --build .
make -jN
```

### Compile plugin code for Android10
#### `make android10`
- compiles plugin code  
- decompile and generate.ll  

#### `fix.ll.android10.sh`
generates ../art_module.ll  

#### `gen_art_module_cc.sh`  
generates the android module  

# Instructions
Part of these instructions will also be given by the make commands (step2).
## Step 1
`plugin_code.cc` will be using whatever is necessary from the runtime

## Step 2
 Generate `art_module_full.ll`
 * Android 6:
`make android6`
 * Android 10:
`make android10`

## Step 3
If `art_module_full.ll` is not copied to `../art_module.ll`, you can do this at this step
(this is the case with android6 I think)

`HELPER_*` methods are not needed in the .ll file. You may delete them at this step

## Step 4
* Call `gen_art_module_cc.sh` to generate `art_module.cc`
    - It uses LLVM C++ API code that generates all the necessary methods and structures

## Step 5
Apply (fixes)[FIXES.md] scripts for LLVM.
