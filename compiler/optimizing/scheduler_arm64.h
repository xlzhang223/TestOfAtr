/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef ART_COMPILER_OPTIMIZING_SCHEDULER_ARM64_H_
#define ART_COMPILER_OPTIMIZING_SCHEDULER_ARM64_H_

#include "scheduler.h"

namespace art {
namespace arm64 {

static constexpr uint32_t kArm64MemoryLoadLatency = 5;
static constexpr uint32_t kArm64MemoryStoreLatency = 3;

static constexpr uint32_t kArm64CallInternalLatency = 10;
static constexpr uint32_t kArm64CallLatency = 5;

// AArch64 instruction latency.
// We currently assume that all arm64 CPUs share the same instruction latency list.
static constexpr uint32_t kArm64IntegerOpLatency = 2;
static constexpr uint32_t kArm64FloatingPointOpLatency = 5;


static constexpr uint32_t kArm64DataProcWithShifterOpLatency = 3;
static constexpr uint32_t kArm64DivDoubleLatency = 30;
static constexpr uint32_t kArm64DivFloatLatency = 15;
static constexpr uint32_t kArm64DivIntegerLatency = 5;
static constexpr uint32_t kArm64LoadStringInternalLatency = 7;
static constexpr uint32_t kArm64MulFloatingPointLatency = 6;
static constexpr uint32_t kArm64MulIntegerLatency = 6;
static constexpr uint32_t kArm64TypeConversionFloatingPointIntegerLatency = 5;

class SchedulingLatencyVisitorARM64 : public SchedulingLatencyVisitor {
 public:
  // Default visitor for instructions not handled specifically below.
  void VisitInstruction(HInstruction* ATTRIBUTE_UNUSED) {
    last_visited_latency_ = kArm64IntegerOpLatency;
  }

// We add a second unused parameter to be able to use this macro like the others
// defined in `nodes.h`.
#define FOR_EACH_SCHEDULED_COMMON_INSTRUCTION(M) \
  M(ArrayGet         , unused)                   \
  M(ArrayLength      , unused)                   \
  M(ArraySet         , unused)                   \
  M(BinaryOperation  , unused)                   \
  M(BoundsCheck      , unused)                   \
  M(Div              , unused)                   \
  M(InstanceFieldGet , unused)                   \
  M(InstanceOf       , unused)                   \
  M(Invoke           , unused)                   \
  M(LoadString       , unused)                   \
  M(Mul              , unused)                   \
  M(NewArray         , unused)                   \
  M(NewInstance      , unused)                   \
  M(Rem              , unused)                   \
  M(StaticFieldGet   , unused)                   \
  M(SuspendCheck     , unused)                   \
  M(TypeConversion   , unused)

#define FOR_EACH_SCHEDULED_SHARED_INSTRUCTION(M) \
  M(BitwiseNegatedRight, unused)                 \
  M(MultiplyAccumulate, unused)                  \
  M(IntermediateAddress, unused)                 \
  M(DataProcWithShifterOp, unused)

#define DECLARE_VISIT_INSTRUCTION(type, unused)  \
  void Visit##type(H##type* instruction) OVERRIDE;

  FOR_EACH_SCHEDULED_COMMON_INSTRUCTION(DECLARE_VISIT_INSTRUCTION)
  FOR_EACH_SCHEDULED_SHARED_INSTRUCTION(DECLARE_VISIT_INSTRUCTION)
  FOR_EACH_CONCRETE_INSTRUCTION_ARM64(DECLARE_VISIT_INSTRUCTION)

#undef DECLARE_VISIT_INSTRUCTION
};

class HSchedulerARM64 : public HScheduler {
 public:
  HSchedulerARM64(ArenaAllocator* arena, SchedulingNodeSelector* selector)
      : HScheduler(arena, &arm64_latency_visitor_, selector) {}
  ~HSchedulerARM64() OVERRIDE {}

  bool IsSchedulable(const HInstruction* instruction) const OVERRIDE {
#define CASE_INSTRUCTION_KIND(type, unused) case \
  HInstruction::InstructionKind::k##type:
    switch (instruction->GetKind()) {
      FOR_EACH_SCHEDULED_SHARED_INSTRUCTION(CASE_INSTRUCTION_KIND)
        return true;
      FOR_EACH_CONCRETE_INSTRUCTION_ARM64(CASE_INSTRUCTION_KIND)
        return true;
      default:
        return HScheduler::IsSchedulable(instruction);
    }
#undef CASE_INSTRUCTION_KIND
  }

 private:
  SchedulingLatencyVisitorARM64 arm64_latency_visitor_;
  DISALLOW_COPY_AND_ASSIGN(HSchedulerARM64);
};

}  // namespace arm64
}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_SCHEDULER_ARM64_H_
