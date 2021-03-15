# mcr-cc: LLVM backend code that is kept as a separate submodule
`libart-compiler` interacts with this code.
It requires familiarity with the remaining of the ART code:
- art/runtime/
- art/compiler
- dex2oat (compiler driver)

# Description: 
Some source code files contain short descriptions on top of the license (either in .cc or in .h files).  
A few important source code files are explained below.

### [analyser.cc](./analyser.cc)
This is a placeholder class for performing static bytecode analysis.
In this version it reads the method to be compiled from files.

### [llvm/](./llvm)
IR-to-IR translation, from HGraph (nodes.h) to LLVM IR

#### [llvm/debug.h](./llvm/debug.h):
Several debugging options for LLVM.


#### [llvm/llvm_compilation_unit.cc](./llvm/llvm_compilation_unit.cc):
Each generated bitcode is in a separate file. For N methods we will have
N bitcode files. Each of those a single compilation unit.
That's an LLVM Compilation Unit (LLVMCU).

#### [llvm/hgraph_to_llvm.cc](./llvm/hgraph_to_llvm.cc):
The whole conversion process starts here with `ExpandIR`.
It setups the entrypoints to LLVM (`llvm_live_` is the one that will be used),
and some other initialization methods (`InitInner`s) for resolving
structures like ArtMethods.

There are two variants of InitInners.
The first (initIchf) it is called by the outermost method (the actual entrypoint).
The others (initInit) are called by the InitInner method of the outermost method, for initializing any other called methods that are also compiled.

Both variants share most of the code, and both are included in the generated version, however a `DCE` pass of LLVM will clear what's not needed anyway.

It also generates placeholders for BasicBlocks and Phis.
The actual population of the instructions within the BasicBlocks is
done by `llvm/hgraph_converter.cc`

##### [llvm/hgraph_converter.cc](./llvm/hgraph_converter.cc):
It utilizes `hgraph_helper.cc` to move some more complex logic like `HandleInvoke`, `CallMethod`, and others.

# NOTES:
#### multi-dex:
multi-dex applications were not reliably supported so there are not part of this release.
