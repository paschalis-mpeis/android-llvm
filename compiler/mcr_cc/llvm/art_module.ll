; ModuleID = 'art_module_full.bc'
;source_filename = "art/compiler/mcr_cc/llvm/tools/plugin_code.cc"
;target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
;target triple = "aarch64-unknown-linux-android"

%"union.art::JValue" = type { i64 }
%"class.art::StackReference" = type { %"class.art::mirror::CompressedReference" }
%"class.art::mirror::CompressedReference" = type { %"class.art::mirror::ObjectReference" }
%"class.art::mirror::ObjectReference" = type { i32 }
%"class.art::mirror::Object" = type { %"class.art::mirror::HeapReference", i32 }
%"class.art::mirror::HeapReference" = type { %"class.art::Atomic" }
%"class.art::Atomic" = type { %"struct.std::__1::atomic" }
%"struct.std::__1::atomic" = type { %"struct.std::__1::__atomic_base" }
%"struct.std::__1::__atomic_base" = type { %"struct.std::__1::__atomic_base.0" }
%"struct.std::__1::__atomic_base.0" = type { i32 }
%"class.art::JNIEnvExt" = type <{ %struct._JNIEnv, %"class.art::Thread"*, %"class.art::JavaVMExt"*, %"struct.art::IRTSegmentState", [4 x i8], %"class.art::IndirectReferenceTable", %"class.std::__1::vector.25", %"class.art::ReferenceTable", %struct.JNINativeInterface*, %"class.std::__1::vector.38", i64, i32, i8, i8, [2 x i8] }>
%struct._JNIEnv = type { %struct.JNINativeInterface* }
%"class.art::Thread" = type <{ %"struct.art::Thread::tls_32bit_sized_values", [4 x i8], %"struct.art::Thread::tls_64bit_sized_values", %"struct.art::Thread::tls_ptr_sized_values", %"class.art::InterpreterCache", %"class.art::Mutex"*, %"class.art::ConditionVariable"*, %"class.art::Monitor"*, i8, [7 x i8], i64, %"class.std::__1::list", %"class.art::SafeMap", i8, [7 x i8] }>
%"struct.art::Thread::tls_32bit_sized_values" = type { %"union.art::Thread::StateAndFlags", i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, %"class.art::Atomic", %"class.art::Atomic.1", i32, i32, i32, i32, %"struct.std::__1::atomic" }
%"union.art::Thread::StateAndFlags" = type { %"class.art::Atomic.1" }
%"class.art::Atomic.1" = type { %"struct.std::__1::atomic.2" }
%"struct.std::__1::atomic.2" = type { %"struct.std::__1::__atomic_base.3" }
%"struct.std::__1::__atomic_base.3" = type { %"struct.std::__1::__atomic_base.4" }
%"struct.std::__1::__atomic_base.4" = type { i32 }
%"struct.art::Thread::tls_64bit_sized_values" = type { i64, %"struct.art::RuntimeStats" }
%"struct.art::RuntimeStats" = type { i64, i64, i64, i64, i64, i64, i64 }
%"struct.art::Thread::tls_ptr_sized_values" = type { i8*, %"class.art::mirror::Throwable"*, i8*, %"class.art::ManagedStack", i64*, %"class.art::JNIEnvExt"*, %"class.art::JNIEnvExt"*, %"class.art::Thread"*, %"class.art::mirror::Object"*, %class._jobject*, i8*, i64, %"union.art::Thread::tls_ptr_sized_values::DepsOrStackTraceSample", %"class.art::Thread"*, %"class.art::mirror::Object"*, %"class.art::BaseHandleScope"*, %class._jobject*, %"class.art::Context"*, %"class.std::__1::deque"*, %"struct.art::DebugInvokeReq"*, %"class.art::SingleStepControl"*, %"class.art::StackedShadowFrameRecord"*, %"class.art::DeoptimizationContextRecord"*, %"class.art::FrameIdToShadowFrame"*, %"class.std::__1::basic_string"*, i64, i8*, %"class.art::Closure"*, [3 x %"class.art::Atomic.1"*], i8*, i8*, i8*, i8*, i64, %"struct.art::JniEntryPoints", %"struct.art::QuickEntryPoints", i8*, [16 x i8*], %"class.art::StackReference"*, %"class.art::StackReference"*, [75 x %"class.art::BaseMutex"*], %"class.art::Closure"*, %"class.art::verifier::MethodVerifier"*, %"class.art::gc::accounting::AtomicStack"*, %"class.art::mirror::Throwable"* }
%"class.art::ManagedStack" = type { %"class.art::ManagedStack::TaggedTopQuickFrame", %"class.art::ManagedStack"*, %"class.art::ShadowFrame"* }
%"class.art::ManagedStack::TaggedTopQuickFrame" = type { i64 }
%"class.art::ShadowFrame" = type opaque
%"union.art::Thread::tls_ptr_sized_values::DepsOrStackTraceSample" = type { %"class.std::__1::vector"* }
%"class.std::__1::vector" = type opaque
%"class.art::BaseHandleScope" = type <{ %"class.art::BaseHandleScope"*, i32 }>
%class._jobject = type { i8 }
%"class.art::Context" = type opaque
%"class.std::__1::deque" = type opaque
%"struct.art::DebugInvokeReq" = type opaque
%"class.art::SingleStepControl" = type opaque
%"class.art::StackedShadowFrameRecord" = type opaque
%"class.art::DeoptimizationContextRecord" = type opaque
%"class.art::FrameIdToShadowFrame" = type opaque
%"class.std::__1::basic_string" = type { %"class.std::__1::__compressed_pair" }
%"class.std::__1::__compressed_pair" = type { %"struct.std::__1::__compressed_pair_elem" }
%"struct.std::__1::__compressed_pair_elem" = type { %"struct.std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >::__rep" }
%"struct.std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >::__rep" = type { %union.anon }
%union.anon = type { %"struct.std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >::__long" }
%"struct.std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >::__long" = type { i64, i64, i8* }
%"struct.art::JniEntryPoints" = type { i8* (%struct._JNIEnv*, %class._jobject*)* }
%"struct.art::QuickEntryPoints" = type { i8* (%"class.art::mirror::Class"*, i32)*, i8* (%"class.art::mirror::Class"*, i32)*, i8* (%"class.art::mirror::Class"*, i32)*, i8* (%"class.art::mirror::Class"*, i32)*, i8* (%"class.art::mirror::Class"*, i32)*, i8* (%"class.art::mirror::Class"*)*, i8* (%"class.art::mirror::Class"*)*, i8* (%"class.art::mirror::Class"*)*, i8* (%"class.art::mirror::Class"*)*, i8* (i8*, i32, i32, i32)*, i8* (i32, i32, i8*)*, i8* (i8*)*, i64 (%"class.art::mirror::Object"*, %"class.art::mirror::Class"*)*, void (%"class.art::mirror::Object"*, %"class.art::mirror::Class"*)*, i8* (%"class.art::mirror::Class"*)*, i8* (i32)*, i8* (i32)*, i8* (i32)*, i8* (i32)*, i8* (i32)*, i32 (i32, i8*, i8)*, i32 (i32, i8)*, i32 (i32, i8*, i16)*, i32 (i32, i16)*, i32 (i32, i8*, i32)*, i32 (i32, i32)*, i32 (i32, i8*, i64)*, i32 (i32, i64)*, i32 (i32, i8*, i8*)*, i32 (i32, i8*)*, i64 (i32, i8*)*, i64 (i32, i8*)*, i64 (i32)*, i64 (i32)*, i64 (i32, i8*)*, i64 (i32, i8*)*, i64 (i32)*, i64 (i32)*, i64 (i32, i8*)*, i64 (i32)*, i64 (i32, i8*)*, i64 (i32)*, i8* (i32, i8*)*, i8* (i32)*, void (%"class.art::mirror::Array"*, i32, %"class.art::mirror::Object"*)*, i32 (%"class.art::Thread"*)*, i32 (%"class.art::Thread"*)*, i32 (%class._jobject*, %"class.art::Thread"*)*, void (i32, %"class.art::Thread"*)*, void (i32, %"class.art::Thread"*)*, void (i32, %class._jobject*, %"class.art::Thread"*)*, %"class.art::mirror::Object"* (%class._jobject*, i32, %"class.art::Thread"*)*, %"class.art::mirror::Object"* (%class._jobject*, i32, %"class.art::Thread"*)*, %"class.art::mirror::Object"* (%class._jobject*, i32, %class._jobject*, %"class.art::Thread"*)*, void (%"class.art::ArtMethod"*)*, void (%"class.art::mirror::Object"*)*, void (%"class.art::mirror::Object"*)*, i32 (double, double)*, i32 (float, float)*, i32 (double, double)*, i32 (float, float)*, double (double)*, double (double)*, double (double)*, double (double)*, double (double)*, double (double, double)*, double (double, double)*, double (double)*, double (double)*, double (double)*, double (double)*, double (double, double)*, double (double)*, double (double)*, double (double, double)*, double (double)*, double (double)*, double (double)*, double (double, double)*, double (i64)*, float (float, float)*, float (i64)*, i32 (double)*, i32 (float)*, i32 (i32, i32)*, i64 (double)*, i64 (float)*, i64 (i64, i64)*, i64 (i64, i64)*, i64 (i64, i64)*, i64 (i64, i32)*, i64 (i64, i32)*, i64 (i64, i32)*, i32 (i8*, i32, i32)*, i32 (i8*, i8*)*, i8* (i8*, i8*, i64)*, void (%"class.art::ArtMethod"*)*, void (%"class.art::ArtMethod"*)*, void (%"class.art::ArtMethod"*)*, void (i32, i8*)*, void (i32, i8*)*, void (i32, i8*)*, void (i32, i8*)*, void (i32, i8*)*, void (i32, i8*)*, void (i32, i8*)*, void ()*, void (%"class.art::mirror::Object"*)*, void (i32, i32)*, void ()*, void ()*, void (i8*)*, void (i32, i32)*, void (i32)*, i64 (i64*)*, void (i64*, i64)*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void ()*, void (%"class.art::mirror::CompressedReference"*, %"class.art::Thread"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*, %"class.art::mirror::Object"*, i32)*, %"class.art::mirror::Object"* (%"class.art::GcRoot"*)*, %"class.art::mirror::Object"* (%"class.art::mirror::Object"*, %"class.art::mirror::Object"*, i32)*, i8* (%"class.art::ArtMethod"*, i32, i8**)*, i8* (%"class.art::ArtMethod"*, i32, i8**)*, i8* (%"class.art::ArtMethod"*, i32, i8**)*, i8* (%"class.art::ArtMethod"*, i32, i32)*, i8* (%"class.art::ArtMethod"*, i8*, i8*, i32, i32)*, i8* (%"class.art::mirror::Object"*, %"class.art::ArtMethod"*)*, i8* (%"class.art::mirror::Object"*, %"class.art::ArtMethod"*)*, i8* (%"class.art::ArtMethod"*, i32, i8**)*, void ()*, void (%"union.art::JValue"*, %"class.art::mirror::Object"*)*, i8* (i32, %"class.art::ArtMethod"*)*, i8* (i32, i8*, %"class.art::ArtMethod"*)*, void (%"class.art::ArtMethod"*, i32*, i32, %"class.art::Thread"*, %"union.art::JValue"*, i8*)*, void (%"class.art::ArtMethod"*, i32*, i32, %"class.art::Thread"*, %"union.art::JValue"*, i8*)*, void (%"class.art::ArtMethod"*, i32*, i32, %"class.art::Thread"*, %"union.art::JValue"*, i8*)*, void (%"class.art::ArtMethod"*, i32*, i32, %"class.art::Thread"*, %"union.art::JValue"*, i8*)*, void (%"class.art::ManagedStack"*)*, void (%"class.art::ManagedStack"*)*, void ()*, void (%"class.art::ArtMethod"*)*, void (%"class.art::mirror::Class"*)*, void (%"class.art::mirror::Object"*)*, void ()* }
%"class.art::mirror::Class" = type { %"class.art::mirror::Object", %"class.art::mirror::HeapReference.7", %"class.art::mirror::HeapReference", %"class.art::mirror::HeapReference.8", %"class.art::mirror::HeapReference.9", %"class.art::mirror::HeapReference.10", %"class.art::mirror::HeapReference.11", %"class.art::mirror::HeapReference", %"class.art::mirror::HeapReference.12", i64, i64, i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i16, i16 }
%"class.art::mirror::HeapReference.7" = type { %"class.art::Atomic" }
%"class.art::mirror::HeapReference.8" = type { %"class.art::Atomic" }
%"class.art::mirror::HeapReference.9" = type { %"class.art::Atomic" }
%"class.art::mirror::HeapReference.10" = type { %"class.art::Atomic" }
%"class.art::mirror::HeapReference.11" = type { %"class.art::Atomic" }
%"class.art::mirror::HeapReference.12" = type { %"class.art::Atomic" }
%"class.art::mirror::Array" = type { %"class.art::mirror::Object", i32, [0 x i32] }
%"class.art::GcRoot" = type { %"class.art::mirror::CompressedReference" }
%"class.art::ArtMethod" = type opaque
%"class.art::BaseMutex" = type opaque
%"class.art::Closure" = type opaque
%"class.art::verifier::MethodVerifier" = type opaque
%"class.art::gc::accounting::AtomicStack" = type opaque
%"class.art::mirror::Throwable" = type opaque
%"class.art::InterpreterCache" = type { %"struct.std::__1::array" }
%"struct.std::__1::array" = type { [256 x %"struct.std::__1::pair"] }
%"struct.std::__1::pair" = type { i8*, i64 }
%"class.art::Mutex" = type opaque
%"class.art::ConditionVariable" = type opaque
%"class.art::Monitor" = type opaque
%"class.std::__1::list" = type { %"class.std::__1::__list_imp" }
%"class.std::__1::__list_imp" = type { %"struct.std::__1::__list_node_base", %"class.std::__1::__compressed_pair.13" }
%"struct.std::__1::__list_node_base" = type { %"struct.std::__1::__list_node_base"*, %"struct.std::__1::__list_node_base"* }
%"class.std::__1::__compressed_pair.13" = type { %"struct.std::__1::__compressed_pair_elem.14" }
%"struct.std::__1::__compressed_pair_elem.14" = type { i64 }
%"class.art::SafeMap" = type { %"class.std::__1::map" }
%"class.std::__1::map" = type { %"class.std::__1::__tree" }
%"class.std::__1::__tree" = type { %"class.std::__1::__tree_end_node"*, %"class.std::__1::__compressed_pair.18", %"class.std::__1::__compressed_pair.23" }
%"class.std::__1::__tree_end_node" = type { %"class.std::__1::__tree_node_base"* }
%"class.std::__1::__tree_node_base" = type opaque
%"class.std::__1::__compressed_pair.18" = type { %"struct.std::__1::__compressed_pair_elem.19" }
%"struct.std::__1::__compressed_pair_elem.19" = type { %"class.std::__1::__tree_end_node" }
%"class.std::__1::__compressed_pair.23" = type { %"struct.std::__1::__compressed_pair_elem.14" }
%"class.art::JavaVMExt" = type opaque
%"struct.art::IRTSegmentState" = type { i32 }
%"class.art::IndirectReferenceTable" = type { %"struct.art::IRTSegmentState", %"class.art::MemMap", %"class.art::IrtEntry"*, i32, i64, i64, %"struct.art::IRTSegmentState", i32 }
%"class.art::MemMap" = type { %"class.std::__1::basic_string", i8*, i64, i8*, i64, i32, i8, i8, i64 }
%"class.art::IrtEntry" = type { i32, [7 x %"class.art::GcRoot"] }
%"class.std::__1::vector.25" = type { %"class.std::__1::__vector_base" }
%"class.std::__1::__vector_base" = type { %"struct.art::IRTSegmentState"*, %"struct.art::IRTSegmentState"*, %"class.std::__1::__compressed_pair.26" }
%"class.std::__1::__compressed_pair.26" = type { %"struct.std::__1::__compressed_pair_elem.27" }
%"struct.std::__1::__compressed_pair_elem.27" = type { %"struct.art::IRTSegmentState"* }
%"class.art::ReferenceTable" = type { %"class.std::__1::basic_string", %"class.std::__1::vector.31", i64 }
%"class.std::__1::vector.31" = type { %"class.std::__1::__vector_base.32" }
%"class.std::__1::__vector_base.32" = type { %"class.art::GcRoot"*, %"class.art::GcRoot"*, %"class.std::__1::__compressed_pair.33" }
%"class.std::__1::__compressed_pair.33" = type { %"struct.std::__1::__compressed_pair_elem.34" }
%"struct.std::__1::__compressed_pair_elem.34" = type { %"class.art::GcRoot"* }
%struct.JNINativeInterface = type { i8*, i8*, i8*, i8*, i32 (%struct._JNIEnv*)*, %class._jclass* (%struct._JNIEnv*, i8*, %class._jobject*, i8*, i32)*, %class._jclass* (%struct._JNIEnv*, i8*)*, %struct._jmethodID* (%struct._JNIEnv*, %class._jobject*)*, %struct._jfieldID* (%struct._JNIEnv*, %class._jobject*)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, i8)*, %class._jclass* (%struct._JNIEnv*, %class._jclass*)*, i8 (%struct._JNIEnv*, %class._jclass*, %class._jclass*)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, i8)*, i32 (%struct._JNIEnv*, %class._jthrowable*)*, i32 (%struct._JNIEnv*, %class._jclass*, i8*)*, %class._jthrowable* (%struct._JNIEnv*)*, void (%struct._JNIEnv*)*, void (%struct._JNIEnv*)*, void (%struct._JNIEnv*, i8*)*, i32 (%struct._JNIEnv*, i32)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*)*, void (%struct._JNIEnv*, %class._jobject*)*, void (%struct._JNIEnv*, %class._jobject*)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jobject*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*)*, i32 (%struct._JNIEnv*, i32)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, %class._jclass* (%struct._JNIEnv*, %class._jobject*)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jclass*)*, %struct._jmethodID* (%struct._JNIEnv*, %class._jclass*, i8*, i8*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, i32 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, i32 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i32 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, i64 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, i64 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i64 (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, float (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, float (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, float (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, double (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, double (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, double (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, ...)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %"struct.std::__va_list"*)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jmethodID*, %union.jvalue*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i8 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i16 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, i16 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i16 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i16 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, i16 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i16 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i32 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, i32 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i32 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i64 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, i64 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i64 (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, float (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, float (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, float (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, double (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, double (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, double (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, void (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, ...)*, void (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, void (%struct._JNIEnv*, %class._jobject*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, %struct._jfieldID* (%struct._JNIEnv*, %class._jclass*, i8*, i8*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, i8 (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, i16 (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, i32 (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, i64 (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, float (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, double (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, %class._jobject*)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, i8)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, i8)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, i16)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, i16)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, i32)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, i64)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, float)*, void (%struct._JNIEnv*, %class._jobject*, %struct._jfieldID*, double)*, %struct._jmethodID* (%struct._JNIEnv*, %class._jclass*, i8*, i8*)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i32 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, i32 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i32 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, i64 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, i64 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, i64 (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, float (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, float (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, float (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, double (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, double (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, double (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, ...)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %"struct.std::__va_list"*)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jmethodID*, %union.jvalue*)*, %struct._jfieldID* (%struct._JNIEnv*, %class._jclass*, i8*, i8*)*, %class._jobject* (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, i8 (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, i16 (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, i32 (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, i64 (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, float (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, double (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, %class._jobject*)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, i8)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, i8)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, i16)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, i16)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, i32)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, i64)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, float)*, void (%struct._JNIEnv*, %class._jclass*, %struct._jfieldID*, double)*, %class._jstring* (%struct._JNIEnv*, i16*, i32)*, i32 (%struct._JNIEnv*, %class._jstring*)*, i16* (%struct._JNIEnv*, %class._jstring*, i8*)*, void (%struct._JNIEnv*, %class._jstring*, i16*)*, %class._jstring* (%struct._JNIEnv*, i8*)*, i32 (%struct._JNIEnv*, %class._jstring*)*, i8* (%struct._JNIEnv*, %class._jstring*, i8*)*, void (%struct._JNIEnv*, %class._jstring*, i8*)*, i32 (%struct._JNIEnv*, %class._jarray*)*, %class._jobjectArray* (%struct._JNIEnv*, i32, %class._jclass*, %class._jobject*)*, %class._jobject* (%struct._JNIEnv*, %class._jobjectArray*, i32)*, void (%struct._JNIEnv*, %class._jobjectArray*, i32, %class._jobject*)*, %class._jbooleanArray* (%struct._JNIEnv*, i32)*, %class._jbyteArray* (%struct._JNIEnv*, i32)*, %class._jcharArray* (%struct._JNIEnv*, i32)*, %class._jshortArray* (%struct._JNIEnv*, i32)*, %class._jintArray* (%struct._JNIEnv*, i32)*, %class._jlongArray* (%struct._JNIEnv*, i32)*, %class._jfloatArray* (%struct._JNIEnv*, i32)*, %class._jdoubleArray* (%struct._JNIEnv*, i32)*, i8* (%struct._JNIEnv*, %class._jbooleanArray*, i8*)*, i8* (%struct._JNIEnv*, %class._jbyteArray*, i8*)*, i16* (%struct._JNIEnv*, %class._jcharArray*, i8*)*, i16* (%struct._JNIEnv*, %class._jshortArray*, i8*)*, i32* (%struct._JNIEnv*, %class._jintArray*, i8*)*, i64* (%struct._JNIEnv*, %class._jlongArray*, i8*)*, float* (%struct._JNIEnv*, %class._jfloatArray*, i8*)*, double* (%struct._JNIEnv*, %class._jdoubleArray*, i8*)*, void (%struct._JNIEnv*, %class._jbooleanArray*, i8*, i32)*, void (%struct._JNIEnv*, %class._jbyteArray*, i8*, i32)*, void (%struct._JNIEnv*, %class._jcharArray*, i16*, i32)*, void (%struct._JNIEnv*, %class._jshortArray*, i16*, i32)*, void (%struct._JNIEnv*, %class._jintArray*, i32*, i32)*, void (%struct._JNIEnv*, %class._jlongArray*, i64*, i32)*, void (%struct._JNIEnv*, %class._jfloatArray*, float*, i32)*, void (%struct._JNIEnv*, %class._jdoubleArray*, double*, i32)*, void (%struct._JNIEnv*, %class._jbooleanArray*, i32, i32, i8*)*, void (%struct._JNIEnv*, %class._jbyteArray*, i32, i32, i8*)*, void (%struct._JNIEnv*, %class._jcharArray*, i32, i32, i16*)*, void (%struct._JNIEnv*, %class._jshortArray*, i32, i32, i16*)*, void (%struct._JNIEnv*, %class._jintArray*, i32, i32, i32*)*, void (%struct._JNIEnv*, %class._jlongArray*, i32, i32, i64*)*, void (%struct._JNIEnv*, %class._jfloatArray*, i32, i32, float*)*, void (%struct._JNIEnv*, %class._jdoubleArray*, i32, i32, double*)*, void (%struct._JNIEnv*, %class._jbooleanArray*, i32, i32, i8*)*, void (%struct._JNIEnv*, %class._jbyteArray*, i32, i32, i8*)*, void (%struct._JNIEnv*, %class._jcharArray*, i32, i32, i16*)*, void (%struct._JNIEnv*, %class._jshortArray*, i32, i32, i16*)*, void (%struct._JNIEnv*, %class._jintArray*, i32, i32, i32*)*, void (%struct._JNIEnv*, %class._jlongArray*, i32, i32, i64*)*, void (%struct._JNIEnv*, %class._jfloatArray*, i32, i32, float*)*, void (%struct._JNIEnv*, %class._jdoubleArray*, i32, i32, double*)*, i32 (%struct._JNIEnv*, %class._jclass*, %struct.JNINativeMethod*, i32)*, i32 (%struct._JNIEnv*, %class._jclass*)*, i32 (%struct._JNIEnv*, %class._jobject*)*, i32 (%struct._JNIEnv*, %class._jobject*)*, i32 (%struct._JNIEnv*, %struct._JavaVM**)*, void (%struct._JNIEnv*, %class._jstring*, i32, i32, i16*)*, void (%struct._JNIEnv*, %class._jstring*, i32, i32, i8*)*, i8* (%struct._JNIEnv*, %class._jarray*, i8*)*, void (%struct._JNIEnv*, %class._jarray*, i8*, i32)*, i16* (%struct._JNIEnv*, %class._jstring*, i8*)*, void (%struct._JNIEnv*, %class._jstring*, i16*)*, %class._jobject* (%struct._JNIEnv*, %class._jobject*)*, void (%struct._JNIEnv*, %class._jobject*)*, i8 (%struct._JNIEnv*)*, %class._jobject* (%struct._JNIEnv*, i8*, i64)*, i8* (%struct._JNIEnv*, %class._jobject*)*, i64 (%struct._JNIEnv*, %class._jobject*)*, i32 (%struct._JNIEnv*, %class._jobject*)* }
%class._jclass = type { i8 }
%struct._jmethodID = type opaque
%struct._jfieldID = type opaque
%class._jthrowable = type { i8 }
%"struct.std::__va_list" = type { i8*, i8*, i8*, i32, i32 }
%union.jvalue = type { i64 }
%class._jstring = type { i8 }
%class._jarray = type { i8 }
%class._jobjectArray = type { i8 }
%class._jbooleanArray = type { i8 }
%class._jbyteArray = type { i8 }
%class._jcharArray = type { i8 }
%class._jshortArray = type { i8 }
%class._jintArray = type { i8 }
%class._jlongArray = type { i8 }
%class._jfloatArray = type { i8 }
%class._jdoubleArray = type { i8 }
%struct.JNINativeMethod = type { i8*, i8*, i8* }
%struct._JavaVM = type { %struct.JNIInvokeInterface* }
%struct.JNIInvokeInterface = type { i8*, i8*, i8*, i32 (%struct._JavaVM*)*, i32 (%struct._JavaVM*, %struct._JNIEnv**, i8*)*, i32 (%struct._JavaVM*)*, i32 (%struct._JavaVM*, i8**, i32)*, i32 (%struct._JavaVM*, %struct._JNIEnv**, i8*)* }
%"class.std::__1::vector.38" = type { %"class.std::__1::__vector_base.39" }
%"class.std::__1::__vector_base.39" = type { %"struct.std::__1::pair.40"*, %"struct.std::__1::pair.40"*, %"class.std::__1::__compressed_pair.41" }
%"struct.std::__1::pair.40" = type opaque
%"class.std::__1::__compressed_pair.41" = type { %"struct.std::__1::__compressed_pair_elem.42" }
%"struct.std::__1::__compressed_pair_elem.42" = type { %"struct.std::__1::pair.40"* }

$_ZN3art6JValueC2Ev = comdat any

$_ZNK3art6JValue4GetBEv = comdat any

$_ZNK3art6JValue4GetCEv = comdat any

$_ZNK3art6JValue4GetDEv = comdat any

$_ZNK3art6JValue4GetFEv = comdat any

$_ZNK3art6JValue4GetIEv = comdat any

$_ZNK3art6JValue4GetJEv = comdat any

$_ZNK3art6JValue4GetLEv = comdat any

$_ZNK3art6JValue4GetSEv = comdat any

$_ZNK3art6JValue4GetZEv = comdat any

$_ZN3art6JValue9GetGCRootEv = comdat any

$_ZN3art6JValue4SetBEa = comdat any

$_ZN3art6JValue4SetCEt = comdat any

$_ZN3art6JValue4SetDEd = comdat any

$_ZN3art6JValue4SetFEf = comdat any

$_ZN3art6JValue4SetIEi = comdat any

$_ZN3art6JValue4SetJEl = comdat any

$_ZN3art6JValue4SetSEs = comdat any

$_ZN3art6JValue4SetZEh = comdat any

$_ZNK3art6mirror15ObjectReferenceILb0ENS0_6ObjectEE11AsMirrorPtrEv = comdat any

$_ZN3art6mirror19CompressedReferenceINS0_6ObjectEE13FromMirrorPtrEPS2_ = comdat any

$_ZNK3art9JNIEnvExt7GetSelfEv = comdat any

$_ZN3art6mirror14PtrCompressionILb0ENS0_6ObjectEE10DecompressEj = comdat any

$_ZN3art6mirror19CompressedReferenceINS0_6ObjectEEC2EPS2_ = comdat any

$_ZN3art6mirror15ObjectReferenceILb0ENS0_6ObjectEEC2EPS2_ = comdat any

$_ZN3art6mirror14PtrCompressionILb0ENS0_6ObjectEE8CompressEPS2_ = comdat any

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValueC2Ev(%"union.art::JValue"* %arg) unnamed_addr #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to i64*
  store i64 0, i64* %tmp2, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr i8 @_ZNK3art6JValue4GetBEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to i8*
  %tmp3 = load i8, i8* %tmp2, align 8
  ret i8 %tmp3
}

; Function Attrs: nounwind
define linkonce_odr i16 @_ZNK3art6JValue4GetCEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to i16*
  %tmp3 = load i16, i16* %tmp2, align 8
  ret i16 %tmp3
}

; Function Attrs: nounwind
define linkonce_odr double @_ZNK3art6JValue4GetDEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to double*
  %tmp3 = load double, double* %tmp2, align 8
  ret double %tmp3
}

; Function Attrs: nounwind
define linkonce_odr float @_ZNK3art6JValue4GetFEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to float*
  %tmp3 = load float, float* %tmp2, align 8
  ret float %tmp3
}

; Function Attrs: nounwind
define linkonce_odr i32 @_ZNK3art6JValue4GetIEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to i32*
  %tmp3 = load i32, i32* %tmp2, align 8
  ret i32 %tmp3
}

; Function Attrs: nounwind
define linkonce_odr i64 @_ZNK3art6JValue4GetJEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to i64*
  %tmp3 = load i64, i64* %tmp2, align 8
  ret i64 %tmp3
}

; Function Attrs: nounwind
define linkonce_odr %"class.art::mirror::Object"* @_ZNK3art6JValue4GetLEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to %"class.art::mirror::Object"**
  %tmp3 = load %"class.art::mirror::Object"*, %"class.art::mirror::Object"** %tmp2, align 8
  ret %"class.art::mirror::Object"* %tmp3
}

; Function Attrs: nounwind
define linkonce_odr i16 @_ZNK3art6JValue4GetSEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to i16*
  %tmp3 = load i16, i16* %tmp2, align 8
  ret i16 %tmp3
}

; Function Attrs: nounwind
define linkonce_odr i8 @_ZNK3art6JValue4GetZEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to i8*
  %tmp3 = load i8, i8* %tmp2, align 8
  ret i8 %tmp3
}

; Function Attrs: nounwind
define linkonce_odr %"class.art::mirror::Object"** @_ZN3art6JValue9GetGCRootEv(%"union.art::JValue"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  %tmp1 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp2 = bitcast %"union.art::JValue"* %tmp1 to %"class.art::mirror::Object"**
  ret %"class.art::mirror::Object"** %tmp2
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetBEa(%"union.art::JValue"* %arg, i8 %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca i8, align 1
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store i8 %arg1, i8* %tmp2, align 1
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load i8, i8* %tmp2, align 1
  %tmp5 = sext i8 %tmp4 to i64
  %tmp6 = shl i64 %tmp5, 56
  %tmp7 = ashr i64 %tmp6, 56
  %tmp8 = bitcast %"union.art::JValue"* %tmp3 to i64*
  store i64 %tmp7, i64* %tmp8, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetCEt(%"union.art::JValue"* %arg, i16 %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca i16, align 2
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store i16 %arg1, i16* %tmp2, align 2
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load i16, i16* %tmp2, align 2
  %tmp5 = zext i16 %tmp4 to i64
  %tmp6 = bitcast %"union.art::JValue"* %tmp3 to i64*
  store i64 %tmp5, i64* %tmp6, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetDEd(%"union.art::JValue"* %arg, double %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca double, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store double %arg1, double* %tmp2, align 8
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load double, double* %tmp2, align 8
  %tmp5 = bitcast %"union.art::JValue"* %tmp3 to double*
  store double %tmp4, double* %tmp5, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetFEf(%"union.art::JValue"* %arg, float %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca float, align 4
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store float %arg1, float* %tmp2, align 4
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load float, float* %tmp2, align 4
  %tmp5 = bitcast %"union.art::JValue"* %tmp3 to float*
  store float %tmp4, float* %tmp5, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetIEi(%"union.art::JValue"* %arg, i32 %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca i32, align 4
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store i32 %arg1, i32* %tmp2, align 4
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load i32, i32* %tmp2, align 4
  %tmp5 = sext i32 %tmp4 to i64
  %tmp6 = shl i64 %tmp5, 32
  %tmp7 = ashr i64 %tmp6, 32
  %tmp8 = bitcast %"union.art::JValue"* %tmp3 to i64*
  store i64 %tmp7, i64* %tmp8, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetJEl(%"union.art::JValue"* %arg, i64 %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca i64, align 8
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store i64 %arg1, i64* %tmp2, align 8
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load i64, i64* %tmp2, align 8
  %tmp5 = bitcast %"union.art::JValue"* %tmp3 to i64*
  store i64 %tmp4, i64* %tmp5, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetSEs(%"union.art::JValue"* %arg, i16 %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca i16, align 2
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store i16 %arg1, i16* %tmp2, align 2
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load i16, i16* %tmp2, align 2
  %tmp5 = sext i16 %tmp4 to i64
  %tmp6 = shl i64 %tmp5, 48
  %tmp7 = ashr i64 %tmp6, 48
  %tmp8 = bitcast %"union.art::JValue"* %tmp3 to i64*
  store i64 %tmp7, i64* %tmp8, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr void @_ZN3art6JValue4SetZEh(%"union.art::JValue"* %arg, i8 %arg1) #1 comdat align 2 {
bb:
  %tmp = alloca %"union.art::JValue"*, align 8
  %tmp2 = alloca i8, align 1
  store %"union.art::JValue"* %arg, %"union.art::JValue"** %tmp, align 8
  store i8 %arg1, i8* %tmp2, align 1
  %tmp3 = load %"union.art::JValue"*, %"union.art::JValue"** %tmp, align 8
  %tmp4 = load i8, i8* %tmp2, align 1
  %tmp5 = zext i8 %tmp4 to i64
  %tmp6 = bitcast %"union.art::JValue"* %tmp3 to i64*
  store i64 %tmp5, i64* %tmp6, align 8
  ret void
}

; Function Attrs:
define linkonce_odr %"class.art::mirror::Object"* @_ZNK3art6mirror15ObjectReferenceILb0ENS0_6ObjectEE11AsMirrorPtrEv(%"class.art::mirror::ObjectReference"* %arg) #0 comdat align 2 {
bb:
  %tmp = alloca %"class.art::mirror::ObjectReference"*, align 8
  store %"class.art::mirror::ObjectReference"* %arg, %"class.art::mirror::ObjectReference"** %tmp, align 8
  %tmp1 = load %"class.art::mirror::ObjectReference"*, %"class.art::mirror::ObjectReference"** %tmp, align 8
  %tmp2 = getelementptr inbounds %"class.art::mirror::ObjectReference", %"class.art::mirror::ObjectReference"* %tmp1, i32 0, i32 0
  %tmp3 = load i32, i32* %tmp2, align 4
  %tmp4 = call %"class.art::mirror::Object"* @_ZN3art6mirror14PtrCompressionILb0ENS0_6ObjectEE10DecompressEj(i32 %tmp3)
  ret %"class.art::mirror::Object"* %tmp4
}

; Function Attrs:
define linkonce_odr i64 @_ZN3art6mirror19CompressedReferenceINS0_6ObjectEE13FromMirrorPtrEPS2_(%"class.art::mirror::Object"* %arg) #0 comdat align 2 {
bb:
  %tmp = alloca %"class.art::mirror::CompressedReference", align 4
  %tmp1 = alloca %"class.art::mirror::Object"*, align 8
  store %"class.art::mirror::Object"* %arg, %"class.art::mirror::Object"** %tmp1, align 8
  %tmp2 = load %"class.art::mirror::Object"*, %"class.art::mirror::Object"** %tmp1, align 8
  call void @_ZN3art6mirror19CompressedReferenceINS0_6ObjectEEC2EPS2_(%"class.art::mirror::CompressedReference"* %tmp, %"class.art::mirror::Object"* %tmp2)
  %tmp3 = getelementptr inbounds %"class.art::mirror::CompressedReference", %"class.art::mirror::CompressedReference"* %tmp, i32 0, i32 0
  %tmp4 = getelementptr inbounds %"class.art::mirror::ObjectReference", %"class.art::mirror::ObjectReference"* %tmp3, i32 0, i32 0
  %tmp5 = load i32, i32* %tmp4, align 4
  %tmp6 = zext i32 %tmp5 to i64
  ret i64 %tmp6
}

declare void @_ZN3art4LLVM14ArrayPutObjectEPviS1_(i8*, i32, i8*) #2

declare i8* @_ZN3art4LLVM13ResolveStringEPvj(i8*, i32) #2

declare void @_ZN3art4LLVM16InvokeMethodSLOWEPvS1_PNS_6JValueEz(i8*, i8*, %"union.art::JValue"*, ...) #2

declare void @_ZN3art4LLVM11DebugInvokeEPvS1_PNS_6JValueEz(i8*, i8*, %"union.art::JValue"*, ...) #2

declare void @_ZN3art4LLVM20DebugInvokeJniMethodEPvS1_(i8*, i8*) #2

declare void @_ZN3art4LLVM15InvokeJniMethodEPvS1_PNS_6JValueEz(i8*, i8*, %"union.art::JValue"*, ...) #2

declare void @_ZN3art4LLVM10AndroidLogEiPKcz(i32, i8*, ...) #2

declare i8* @_ZN3art4LLVM11AllocObjectEPvj(i8*, i32) #2

declare i8* @_ZN3art4LLVM27AllocObjectWithAccessChecksEPvj(i8*, i32) #2

declare i8* @_ZN3art4LLVM10AllocArrayEPvjj(i8*, i32, i32) #2

declare i8* @_ZN3art4LLVM26AllocArrayWithAccessChecksEPvjj(i8*, i32, i32) #2

declare void @_ZN3art4LLVM9CheckCastEPvS1_(i8*, i8*) #2

declare i1 @_ZN3art4LLVM10InstanceOfEPvS1_(i8*, i8*) #2

declare void @_ZN3art4LLVM29AddToInvokeHistogram_fromLLVMEPvS1_jS1_(i8*, i8*, i32, i8*) #2

declare %class._jobject* @_ZN3art4LLVM15AddJniReferenceEPNS_9JNIEnvExtEPv(%"class.art::JNIEnvExt"*, i8*) #2

declare void @_ZN3art4LLVM18RemoveJniReferenceEPNS_9JNIEnvExtEP8_jobject(%"class.art::JNIEnvExt"*, %class._jobject*) #2

declare void @_ZN3art4LLVM25VerifyCurrentThreadMethodEv() #2

declare void @_ZN3art4LLVM12VerifyThreadEPv(i8*) #2

declare void @_ZN3art4LLVM12VerifyStringEPv(i8*) #2

declare void @_ZN3art4LLVM15VerifyBssObjectEPv(i8*) #2

; Function Attrs: nounwind
define linkonce_odr %"class.art::Thread"* @_ZNK3art9JNIEnvExt7GetSelfEv(%"class.art::JNIEnvExt"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"class.art::JNIEnvExt"*, align 8
  store %"class.art::JNIEnvExt"* %arg, %"class.art::JNIEnvExt"** %tmp, align 8
  %tmp1 = load %"class.art::JNIEnvExt"*, %"class.art::JNIEnvExt"** %tmp, align 8
  %tmp2 = getelementptr inbounds %"class.art::JNIEnvExt", %"class.art::JNIEnvExt"* %tmp1, i32 0, i32 1
  %tmp3 = load %"class.art::Thread"*, %"class.art::Thread"** %tmp2, align 8
  ret %"class.art::Thread"* %tmp3
}

declare i32 @_ZN3art4LLVM15StringCompareToEPvS1_(i8*, i8*) #2

declare i8* @_ZN3art4LLVM17GetDeclaringClassEPv(i8*) #2

declare void @_ZN3art4LLVM10workaroundEv() #2

declare void @_ZN3art4LLVM12workaroundIIEii(i32, i32) #2

declare void @_ZN3art4LLVM12SetJniMethodEPv(i8*) #2

declare void @_ZN3art4LLVM15EnableDebugLLVMEv() #2

; Function Attrs: nounwind
define linkonce_odr %"class.art::mirror::Object"* @_ZN3art6mirror14PtrCompressionILb0ENS0_6ObjectEE10DecompressEj(i32 %arg) #1 comdat align 2 {
bb:
  %tmp = alloca i32, align 4
  %tmp1 = alloca i64, align 8
  store i32 %arg, i32* %tmp, align 4
  %tmp2 = load i32, i32* %tmp, align 4
  %tmp3 = zext i32 %tmp2 to i64
  store i64 %tmp3, i64* %tmp1, align 8
  %tmp4 = load i64, i64* %tmp1, align 8
  %tmp5 = inttoptr i64 %tmp4 to %"class.art::mirror::Object"*
  ret %"class.art::mirror::Object"* %tmp5
}

; Function Attrs:
define linkonce_odr void @_ZN3art6mirror19CompressedReferenceINS0_6ObjectEEC2EPS2_(%"class.art::mirror::CompressedReference"* %arg, %"class.art::mirror::Object"* %arg1) unnamed_addr #0 comdat align 2 {
bb:
  %tmp = alloca %"class.art::mirror::CompressedReference"*, align 8
  %tmp2 = alloca %"class.art::mirror::Object"*, align 8
  store %"class.art::mirror::CompressedReference"* %arg, %"class.art::mirror::CompressedReference"** %tmp, align 8
  store %"class.art::mirror::Object"* %arg1, %"class.art::mirror::Object"** %tmp2, align 8
  %tmp3 = load %"class.art::mirror::CompressedReference"*, %"class.art::mirror::CompressedReference"** %tmp, align 8
  %tmp4 = bitcast %"class.art::mirror::CompressedReference"* %tmp3 to %"class.art::mirror::ObjectReference"*
  %tmp5 = load %"class.art::mirror::Object"*, %"class.art::mirror::Object"** %tmp2, align 8
  call void @_ZN3art6mirror15ObjectReferenceILb0ENS0_6ObjectEEC2EPS2_(%"class.art::mirror::ObjectReference"* %tmp4, %"class.art::mirror::Object"* %tmp5)
  ret void
}

; Function Attrs:
define linkonce_odr void @_ZN3art6mirror15ObjectReferenceILb0ENS0_6ObjectEEC2EPS2_(%"class.art::mirror::ObjectReference"* %arg, %"class.art::mirror::Object"* %arg1) unnamed_addr #0 comdat align 2 {
bb:
  %tmp = alloca %"class.art::mirror::ObjectReference"*, align 8
  %tmp2 = alloca %"class.art::mirror::Object"*, align 8
  store %"class.art::mirror::ObjectReference"* %arg, %"class.art::mirror::ObjectReference"** %tmp, align 8
  store %"class.art::mirror::Object"* %arg1, %"class.art::mirror::Object"** %tmp2, align 8
  %tmp3 = load %"class.art::mirror::ObjectReference"*, %"class.art::mirror::ObjectReference"** %tmp, align 8
  %tmp4 = getelementptr inbounds %"class.art::mirror::ObjectReference", %"class.art::mirror::ObjectReference"* %tmp3, i32 0, i32 0
  %tmp5 = load %"class.art::mirror::Object"*, %"class.art::mirror::Object"** %tmp2, align 8
  %tmp6 = call i32 @_ZN3art6mirror14PtrCompressionILb0ENS0_6ObjectEE8CompressEPS2_(%"class.art::mirror::Object"* %tmp5)
  store i32 %tmp6, i32* %tmp4, align 4
  ret void
}

; Function Attrs: nounwind
define linkonce_odr i32 @_ZN3art6mirror14PtrCompressionILb0ENS0_6ObjectEE8CompressEPS2_(%"class.art::mirror::Object"* %arg) #1 comdat align 2 {
bb:
  %tmp = alloca %"class.art::mirror::Object"*, align 8
  %tmp1 = alloca i64, align 8
  store %"class.art::mirror::Object"* %arg, %"class.art::mirror::Object"** %tmp, align 8
  %tmp2 = load %"class.art::mirror::Object"*, %"class.art::mirror::Object"** %tmp, align 8
  %tmp3 = ptrtoint %"class.art::mirror::Object"* %tmp2 to i64
  store i64 %tmp3, i64* %tmp1, align 8
  %tmp4 = load i64, i64* %tmp1, align 8
  %tmp5 = trunc i64 %tmp4 to i32
  ret i32 %tmp5
}

attributes #0 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="kryo" "target-features"="+neon" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="kryo" "target-features"="+neon" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="kryo" "target-features"="+neon" "unsafe-fp-math"="false" "use-soft-float"="false" }

;!llvm.module.flags = !{!0, !1, !2}
;!llvm.ident = !{!3}

;!0 = !{i32 1, !"wchar_size", i32 4}
;!1 = !{i32 7, !"PIC Level", i32 2}
;!2 = !{i32 7, !"PIE Level", i32 2}
;!3 = !{!"Android (5484270 based on r353983c) clang version 9.0.3 (https://android.googlesource.com/toolchain/clang 745b335211bb9eadfa6aa6301f84715cee4b37c5) (https://android.googlesource.com/toolchain/llvm 60cf23e54e46c807513f7a36d0a7b777920b5881) (based on LLVM 9.0.3svn)"}
