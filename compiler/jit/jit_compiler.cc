/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jit_compiler.h"

#include "android-base/stringprintf.h"

#include "arch/instruction_set.h"
#include "arch/instruction_set_features.h"
#include "art_method-inl.h"
#include "base/stringpiece.h"
#include "base/time_utils.h"
#include "base/timing_logger.h"
#include "base/unix_file/fd_file.h"
#include "debug/elf_debug_writer.h"
#include "driver/compiler_driver.h"
#include "driver/compiler_options.h"
#include "jit/debugger_interface.h"
#include "jit/jit.h"
#include "jit/jit_code_cache.h"
#include "oat_file-inl.h"
#include "oat_quick_method_header.h"
#include "object_lock.h"
#include "optimizing/register_allocator.h"
#include "thread_list.h"

namespace art {
namespace jit {

JitCompiler* JitCompiler::Create() {
  return new JitCompiler();
}

extern "C" void* jit_load(bool* generate_debug_info) {
  VLOG(jit) << "loading jit compiler";
  auto* const jit_compiler = JitCompiler::Create();
  CHECK(jit_compiler != nullptr);
  *generate_debug_info = jit_compiler->GetCompilerOptions()->GetGenerateDebugInfo();
  VLOG(jit) << "Done loading jit compiler";
  return jit_compiler;
}

extern "C" void jit_unload(void* handle) {
  DCHECK(handle != nullptr);
  delete reinterpret_cast<JitCompiler*>(handle);
}

extern "C" bool jit_compile_method(
    void* handle, ArtMethod* method, Thread* self, bool osr)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  auto* jit_compiler = reinterpret_cast<JitCompiler*>(handle);
  DCHECK(jit_compiler != nullptr);
  return jit_compiler->CompileMethod(self, method, osr);
}

extern "C" void jit_types_loaded(void* handle, mirror::Class** types, size_t count)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  auto* jit_compiler = reinterpret_cast<JitCompiler*>(handle);
  DCHECK(jit_compiler != nullptr);
  if (jit_compiler->GetCompilerOptions()->GetGenerateDebugInfo()) {
    const ArrayRef<mirror::Class*> types_array(types, count);
    std::vector<uint8_t> elf_file = debug::WriteDebugElfFileForClasses(
        kRuntimeISA, jit_compiler->GetCompilerDriver()->GetInstructionSetFeatures(), types_array);
    CreateJITCodeEntry(std::move(elf_file));
  }
}

// Callers of this method assume it has NO_RETURN.
NO_RETURN static void Usage(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string error;
  android::base::StringAppendV(&error, fmt, ap);
  LOG(FATAL) << error;
  va_end(ap);
  exit(EXIT_FAILURE);
}

JitCompiler::JitCompiler() {
  compiler_options_.reset(new CompilerOptions(
      CompilerFilter::kDefaultCompilerFilter,
      CompilerOptions::kDefaultHugeMethodThreshold,
      CompilerOptions::kDefaultLargeMethodThreshold,
      CompilerOptions::kDefaultSmallMethodThreshold,
      CompilerOptions::kDefaultTinyMethodThreshold,
      CompilerOptions::kDefaultNumDexMethodsThreshold,
      CompilerOptions::kDefaultInlineMaxCodeUnits,
      /* no_inline_from */ nullptr,
      CompilerOptions::kDefaultTopKProfileThreshold,
      Runtime::Current()->IsJavaDebuggable(),
      CompilerOptions::kDefaultGenerateDebugInfo,
      /* implicit_null_checks */ true,
      /* implicit_so_checks */ true,
      /* implicit_suspend_checks */ false,
      /* pic */ true,  // TODO: Support non-PIC in optimizing.
      /* verbose_methods */ nullptr,
      /* init_failure_output */ nullptr,
      /* abort_on_hard_verifier_failure */ false,
      /* dump_cfg_file_name */ "",
      /* dump_cfg_append */ false,
      /* force_determinism */ false,
      RegisterAllocator::kRegisterAllocatorDefault,
      /* passes_to_run */ nullptr));
  for (const std::string& argument : Runtime::Current()->GetCompilerOptions()) {
    compiler_options_->ParseCompilerOption(argument, Usage);
  }
  const InstructionSet instruction_set = kRuntimeISA;
  for (const StringPiece option : Runtime::Current()->GetCompilerOptions()) {
    VLOG(compiler) << "JIT compiler option " << option;
    std::string error_msg;
    if (option.starts_with("--instruction-set-variant=")) {
      StringPiece str = option.substr(strlen("--instruction-set-variant=")).data();
      VLOG(compiler) << "JIT instruction set variant " << str;
      instruction_set_features_ = InstructionSetFeatures::FromVariant(
          instruction_set, str.as_string(), &error_msg);
      if (instruction_set_features_ == nullptr) {
        LOG(WARNING) << "Error parsing " << option << " message=" << error_msg;
      }
    } else if (option.starts_with("--instruction-set-features=")) {
      StringPiece str = option.substr(strlen("--instruction-set-features=")).data();
      VLOG(compiler) << "JIT instruction set features " << str;
      if (instruction_set_features_ == nullptr) {
        instruction_set_features_ = InstructionSetFeatures::FromVariant(
            instruction_set, "default", &error_msg);
        if (instruction_set_features_ == nullptr) {
          LOG(WARNING) << "Error parsing " << option << " message=" << error_msg;
        }
      }
      instruction_set_features_ =
          instruction_set_features_->AddFeaturesFromString(str.as_string(), &error_msg);
      if (instruction_set_features_ == nullptr) {
        LOG(WARNING) << "Error parsing " << option << " message=" << error_msg;
      }
    }
  }
  if (instruction_set_features_ == nullptr) {
    instruction_set_features_ = InstructionSetFeatures::FromCppDefines();
  }
  cumulative_logger_.reset(new CumulativeLogger("jit times"));
  compiler_driver_.reset(new CompilerDriver(
      compiler_options_.get(),
      /* verification_results */ nullptr,
      Compiler::kOptimizing,
      instruction_set,
      instruction_set_features_.get(),
      /* image_classes */ nullptr,
      /* compiled_classes */ nullptr,
      /* compiled_methods */ nullptr,
      /* thread_count */ 1,
      /* dump_stats */ false,
      /* dump_passes */ false,
      cumulative_logger_.get(),
      /* swap_fd */ -1,
      /* profile_compilation_info */ nullptr));
  // Disable dedupe so we can remove compiled methods.
  compiler_driver_->SetDedupeEnabled(false);
  compiler_driver_->SetSupportBootImageFixup(false);

  size_t thread_count = compiler_driver_->GetThreadCount();
  if (compiler_options_->GetGenerateDebugInfo()) {
    DCHECK_EQ(thread_count, 1u)
        << "Generating debug info only works with one compiler thread";
    jit_logger_.reset(new JitLogger());
    jit_logger_->OpenLog();
  }
}

JitCompiler::~JitCompiler() {
  if (compiler_options_->GetGenerateDebugInfo()) {
    jit_logger_->CloseLog();
  }
}

bool JitCompiler::CompileMethod(Thread* self, ArtMethod* method, bool osr) {
  DCHECK(!method->IsProxyMethod());
  DCHECK(method->GetDeclaringClass()->IsResolved());

  TimingLogger logger("JIT compiler timing logger", true, VLOG_IS_ON(jit));
  self->AssertNoPendingException();
  Runtime* runtime = Runtime::Current();

  // Do the compilation.
  bool success = false;
  {
    TimingLogger::ScopedTiming t2("Compiling", &logger);
    JitCodeCache* const code_cache = runtime->GetJit()->GetCodeCache();
    success = compiler_driver_->GetCompiler()->JitCompile(self, code_cache, method, osr);
    if (success && (jit_logger_ != nullptr)) {
      jit_logger_->WriteLog(code_cache, method, osr);
    }
  }

  // Trim maps to reduce memory usage.
  // TODO: move this to an idle phase.
  {
    TimingLogger::ScopedTiming t2("TrimMaps", &logger);
    runtime->GetJitArenaPool()->TrimMaps();
  }

  runtime->GetJit()->AddTimingLogger(logger);
  return success;
}

}  // namespace jit
}  // namespace art
