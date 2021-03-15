# A. New source files:

The bulk of the code is at:

## [compiler/mcr_cc](compiler/mcr_cc/): compiler additions
#### - [compiler/mcr_cc/llvm](compiler/mcr_cc/llvm/): **LLVM backend code** 

## [runtime/mcr_rt](runtime/mcr_rt/): runtime additions


# B. Modified files:

The below source files were modified for binding the LLVM backend with the
compiler, and providing runtime support for it.

## B1. SOONG Build System modifications:
```
build/Android.bp
build/Android.common.mk
build/apex/Android.bp
build/art.go
dex2oat/Android.bp
compiler/Android.bp
runtime/Android.bp
```

## B2. Compiler modifications:
```
compiler/driver/compiler_options.cc
compiler/driver/compiler_options.h
compiler/optimizing/code_generator_arm64.cc
compiler/optimizing/data_type.h
compiler/optimizing/inliner.cc
compiler/optimizing/instruction_builder.cc
compiler/optimizing/instruction_simplifier_arm64.cc
compiler/optimizing/instruction_simplifier_shared.cc
compiler/optimizing/nodes.cc
compiler/optimizing/nodes.h
compiler/optimizing/optimizing_compiler.cc
```

## B3. dex2oat: Compiler Driver
```
dex2oat/dex2oat.cc
dex2oat/dex2oat_options.cc
dex2oat/dex2oat_options.def
dex2oat/driver/compiler_driver.cc
dex2oat/driver/compiler_driver.h
```

## B4. runtime:
```
runtime/arch/arm/quick_entrypoints_arm.S
runtime/arch/arm64/quick_entrypoints_arm64.S
runtime/art_method-inl.h
runtime/art_method.cc
runtime/art_method.h
runtime/class_linker-inl.h
runtime/class_linker.cc
runtime/class_loader_context.cc
runtime/common_dex_operations.h
runtime/entrypoints/entrypoint_utils-inl.h
runtime/entrypoints/entrypoint_utils.cc
runtime/entrypoints/entrypoint_utils.h
runtime/entrypoints/jni/jni_entrypoints.cc 
runtime/entrypoints/llvm/debug_entrypoints.cc
runtime/entrypoints/llvm/entrypoints.cc
runtime/entrypoints/quick/quick_alloc_entrypoints.cc
runtime/entrypoints/quick/quick_cast_entrypoints.cc
runtime/entrypoints/quick/quick_default_externs.h
runtime/entrypoints/quick/quick_default_init_entrypoints.h
runtime/entrypoints/quick/quick_dexcache_entrypoints.cc
runtime/entrypoints/quick/quick_entrypoints.h
runtime/entrypoints/quick/quick_entrypoints_list.h
runtime/entrypoints/quick/quick_field_entrypoints.cc
runtime/entrypoints/quick/quick_lock_entrypoints.cc
runtime/entrypoints/quick/quick_thread_entrypoints.cc
runtime/entrypoints/quick/quick_throw_entrypoints.cc
runtime/entrypoints/quick/quick_trampoline_entrypoints.cc
runtime/gc/heap.cc
runtime/hidden_api.cc
runtime/instrumentation.cc 
runtime/interpreter/interpreter_common.cc
runtime/jni/java_vm_ext.cc
runtime/jni/jni_internal.cc
runtime/managed_stack-inl.h
runtime/managed_stack.h
runtime/mirror/class.h
runtime/native/dalvik_system_VMRuntime.cc
runtime/oat_file_manager.cc
runtime/stack.cc
runtime/thread-inl.h
runtime/thread.cc
runtime/thread.h
runtime/thread_list.cc
runtime/thread_list.h
runtime/trace.cc
```

