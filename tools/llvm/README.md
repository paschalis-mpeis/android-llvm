# tools/llvm
A set of convenient tools for demonstrating an LLVM compilation.

Internally we have used an Orchestrator Android app for controlling more parameters.

An application has configurations in this path:  
`DIR_MCR/<USER_ID>/<APP_PACKAGE/`

The USER_ID is most likely: `0`

## [config.sh](./config.sh)
IMPORTANT: Open this and set some variables depending on the device/host (e.g., `LLVM_DIS`).
It was tested for an `arm64` device.

## [compile.sh](./compile.sh)
It pushes a compilation profile for LLVM, that contains all the methods that will be compiled. The first one will be the region's entrypoint.  
Some reworking is needed to make the published verions to work with several regions.

#### Generating LLVM Bitcode:
It perfoms an IR-to-IR translation (LLVM Backend) for translating the HGraph to LLVM bitcode. It also generates some pretty printed files

At the following directories the below files will be generated:
###### `<DIR_MCR>/<USER_ID>/<APP_PACKAGE>/`:
- `oat.aux`:  
Ideally all this code should be embedded in OAT files, and exploit all caching
mechanisms that the Quick code already benefits from.
This requires lot of additional work, so currently an `oat.aux` file is created,
which is loaded at application bootstrap to load code for any LLVM compiled entrypoints.

##### `<DIR_MCR>/<USER_ID>/<APP_PACKAGE>/hf_<STRIPPED_METHOD_NAME>/src/`:
- `bytecode.txt`: Dalvik Bytecode pretty printed
- `hf.inner.bc`: Inner LLVM method  
    + the entrypoint could be including several inner methods  
    + NOTE: this logic is in place and utilized in other private projects but might need adjustments  
     for this project.  
- `hf.outer.bc`: LLVM Entrypoint  
- `hgraph.ssa.txt`: HGraph pretty printed  
- `hf.lnk.bc`: several bitcodes can be linked together using (`llvm-link` tool)  
- `hf.lnk.dce.bc`: Dead Code Elimination applied (for reviewing the bitcode)  
- `hf.lnk.opt.bc`: Optimized bitcode (`opt` tool)  
- `hf.so`: **this is what will be executed!** It will be loaded as a shared library on application's bootstrap.  

#### New OAT files:

New OAT files will also be generated at:  

`/data/app/<APP_PACKAGE>-<INSTALL_SHA>==/oat/arm64/`:

These allow the runtime to be able to jump to the LLVM `hot region` as
the caller of the region is not inlined.

##### Symlinks:  
```
base.art -> base.llvm.art (symlink)
base.vdex -> base.llvm.vdex (symlink)
base.odex -> base.llvm.odex (symlink)
```

##### Compiled OAT files:  
```
base.llvm.art
base.llvm.odex
base.llvm.vdex
```
##### Original OAT files:
```
base.orig.art
base.orig.odex
base.orig.vdex
```

## [restore_oat.sh](./restore_oat.sh):
Reverts to the original OAT files.

## [enable_llvm.sh](./enable_llvm.sh):
Enables the LLVM code

## [enable.sh](./enable.sh):
Can be extended to accept parameters for enabling different compilation options

NOTE: recompilation is needed after changing options that cause bitcode changes.

Just a couple of options are added in this scripts, but the full list can be found at:  
[compiler/mcr_cc/llvm/debug.h](../../compiler/mcr_cc/llvm/debug.h)

(some are specific to an app, and some are specific to a hot region).

#### Example usages:

Enable LLVM code (same as [enable_llvm.sh](./enable_llvm.sh)):

```
./enable.sh llvm
```

Print verification that LLVM code was executed:
```
./enable.sh verify-llvm-called
```

## [disable.sh](./disable.sh):
Like [enable_llvm.sh](./enable_llvm.sh) but instead it reverses the settings.

## [pull_code.sh](./pull_code.sh):

Pulls to host machine these generated files:

- `bytecode.dalvik`: pretty printed Dalvik Bytecode. This is what it becomes an HGraph.
- `ssa.hgraph`: pretty printed HGraph in SSA form. This is what the IR-to-IR translator works with.
- `hf.lnk.dce.ll`: decompiled LLVM bitcode (with DCE pass previously applied to it).

