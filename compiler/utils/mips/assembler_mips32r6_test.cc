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

#include "assembler_mips.h"

#include <map>

#include "base/stl_util.h"
#include "utils/assembler_test.h"

#define __ GetAssembler()->

namespace art {

struct MIPSCpuRegisterCompare {
  bool operator()(const mips::Register& a, const mips::Register& b) const {
    return a < b;
  }
};

class AssemblerMIPS32r6Test : public AssemblerTest<mips::MipsAssembler,
                                                   mips::Register,
                                                   mips::FRegister,
                                                   uint32_t> {
 public:
  typedef AssemblerTest<mips::MipsAssembler, mips::Register, mips::FRegister, uint32_t> Base;

  AssemblerMIPS32r6Test() :
    instruction_set_features_(MipsInstructionSetFeatures::FromVariant("mips32r6", nullptr)) {
  }

 protected:
  // Get the typically used name for this architecture, e.g., aarch64, x86-64, ...
  std::string GetArchitectureString() OVERRIDE {
    return "mips";
  }

  std::string GetAssemblerCmdName() OVERRIDE {
    // We assemble and link for MIPS32R6. See GetAssemblerParameters() for details.
    return "gcc";
  }

  std::string GetAssemblerParameters() OVERRIDE {
    // We assemble and link for MIPS32R6. The reason is that object files produced for MIPS32R6
    // (and MIPS64R6) with the GNU assembler don't have correct final offsets in PC-relative
    // branches in the .text section and so they require a relocation pass (there's a relocation
    // section, .rela.text, that has the needed info to fix up the branches).
    // We use "-modd-spreg" so we can use odd-numbered single precision FPU registers.
    // We put the code at address 0x1000000 (instead of 0) to avoid overlapping with the
    // .MIPS.abiflags section (there doesn't seem to be a way to suppress its generation easily).
    return " -march=mips32r6 -modd-spreg -Wa,--no-warn"
        " -Wl,-Ttext=0x1000000 -Wl,-e0x1000000 -nostdlib";
  }

  void Pad(std::vector<uint8_t>& data) OVERRIDE {
    // The GNU linker unconditionally pads the code segment with NOPs to a size that is a multiple
    // of 16 and there doesn't appear to be a way to suppress this padding. Our assembler doesn't
    // pad, so, in order for two assembler outputs to match, we need to match the padding as well.
    // NOP is encoded as four zero bytes on MIPS.
    size_t pad_size = RoundUp(data.size(), 16u) - data.size();
    data.insert(data.end(), pad_size, 0);
  }

  std::string GetDisassembleParameters() OVERRIDE {
    return " -D -bbinary -mmips:isa32r6";
  }

  mips::MipsAssembler* CreateAssembler(ArenaAllocator* arena) OVERRIDE {
    return new (arena) mips::MipsAssembler(arena, instruction_set_features_.get());
  }

  void SetUpHelpers() OVERRIDE {
    if (registers_.size() == 0) {
      registers_.push_back(new mips::Register(mips::ZERO));
      registers_.push_back(new mips::Register(mips::AT));
      registers_.push_back(new mips::Register(mips::V0));
      registers_.push_back(new mips::Register(mips::V1));
      registers_.push_back(new mips::Register(mips::A0));
      registers_.push_back(new mips::Register(mips::A1));
      registers_.push_back(new mips::Register(mips::A2));
      registers_.push_back(new mips::Register(mips::A3));
      registers_.push_back(new mips::Register(mips::T0));
      registers_.push_back(new mips::Register(mips::T1));
      registers_.push_back(new mips::Register(mips::T2));
      registers_.push_back(new mips::Register(mips::T3));
      registers_.push_back(new mips::Register(mips::T4));
      registers_.push_back(new mips::Register(mips::T5));
      registers_.push_back(new mips::Register(mips::T6));
      registers_.push_back(new mips::Register(mips::T7));
      registers_.push_back(new mips::Register(mips::S0));
      registers_.push_back(new mips::Register(mips::S1));
      registers_.push_back(new mips::Register(mips::S2));
      registers_.push_back(new mips::Register(mips::S3));
      registers_.push_back(new mips::Register(mips::S4));
      registers_.push_back(new mips::Register(mips::S5));
      registers_.push_back(new mips::Register(mips::S6));
      registers_.push_back(new mips::Register(mips::S7));
      registers_.push_back(new mips::Register(mips::T8));
      registers_.push_back(new mips::Register(mips::T9));
      registers_.push_back(new mips::Register(mips::K0));
      registers_.push_back(new mips::Register(mips::K1));
      registers_.push_back(new mips::Register(mips::GP));
      registers_.push_back(new mips::Register(mips::SP));
      registers_.push_back(new mips::Register(mips::FP));
      registers_.push_back(new mips::Register(mips::RA));

      secondary_register_names_.emplace(mips::Register(mips::ZERO), "zero");
      secondary_register_names_.emplace(mips::Register(mips::AT), "at");
      secondary_register_names_.emplace(mips::Register(mips::V0), "v0");
      secondary_register_names_.emplace(mips::Register(mips::V1), "v1");
      secondary_register_names_.emplace(mips::Register(mips::A0), "a0");
      secondary_register_names_.emplace(mips::Register(mips::A1), "a1");
      secondary_register_names_.emplace(mips::Register(mips::A2), "a2");
      secondary_register_names_.emplace(mips::Register(mips::A3), "a3");
      secondary_register_names_.emplace(mips::Register(mips::T0), "t0");
      secondary_register_names_.emplace(mips::Register(mips::T1), "t1");
      secondary_register_names_.emplace(mips::Register(mips::T2), "t2");
      secondary_register_names_.emplace(mips::Register(mips::T3), "t3");
      secondary_register_names_.emplace(mips::Register(mips::T4), "t4");
      secondary_register_names_.emplace(mips::Register(mips::T5), "t5");
      secondary_register_names_.emplace(mips::Register(mips::T6), "t6");
      secondary_register_names_.emplace(mips::Register(mips::T7), "t7");
      secondary_register_names_.emplace(mips::Register(mips::S0), "s0");
      secondary_register_names_.emplace(mips::Register(mips::S1), "s1");
      secondary_register_names_.emplace(mips::Register(mips::S2), "s2");
      secondary_register_names_.emplace(mips::Register(mips::S3), "s3");
      secondary_register_names_.emplace(mips::Register(mips::S4), "s4");
      secondary_register_names_.emplace(mips::Register(mips::S5), "s5");
      secondary_register_names_.emplace(mips::Register(mips::S6), "s6");
      secondary_register_names_.emplace(mips::Register(mips::S7), "s7");
      secondary_register_names_.emplace(mips::Register(mips::T8), "t8");
      secondary_register_names_.emplace(mips::Register(mips::T9), "t9");
      secondary_register_names_.emplace(mips::Register(mips::K0), "k0");
      secondary_register_names_.emplace(mips::Register(mips::K1), "k1");
      secondary_register_names_.emplace(mips::Register(mips::GP), "gp");
      secondary_register_names_.emplace(mips::Register(mips::SP), "sp");
      secondary_register_names_.emplace(mips::Register(mips::FP), "fp");
      secondary_register_names_.emplace(mips::Register(mips::RA), "ra");

      fp_registers_.push_back(new mips::FRegister(mips::F0));
      fp_registers_.push_back(new mips::FRegister(mips::F1));
      fp_registers_.push_back(new mips::FRegister(mips::F2));
      fp_registers_.push_back(new mips::FRegister(mips::F3));
      fp_registers_.push_back(new mips::FRegister(mips::F4));
      fp_registers_.push_back(new mips::FRegister(mips::F5));
      fp_registers_.push_back(new mips::FRegister(mips::F6));
      fp_registers_.push_back(new mips::FRegister(mips::F7));
      fp_registers_.push_back(new mips::FRegister(mips::F8));
      fp_registers_.push_back(new mips::FRegister(mips::F9));
      fp_registers_.push_back(new mips::FRegister(mips::F10));
      fp_registers_.push_back(new mips::FRegister(mips::F11));
      fp_registers_.push_back(new mips::FRegister(mips::F12));
      fp_registers_.push_back(new mips::FRegister(mips::F13));
      fp_registers_.push_back(new mips::FRegister(mips::F14));
      fp_registers_.push_back(new mips::FRegister(mips::F15));
      fp_registers_.push_back(new mips::FRegister(mips::F16));
      fp_registers_.push_back(new mips::FRegister(mips::F17));
      fp_registers_.push_back(new mips::FRegister(mips::F18));
      fp_registers_.push_back(new mips::FRegister(mips::F19));
      fp_registers_.push_back(new mips::FRegister(mips::F20));
      fp_registers_.push_back(new mips::FRegister(mips::F21));
      fp_registers_.push_back(new mips::FRegister(mips::F22));
      fp_registers_.push_back(new mips::FRegister(mips::F23));
      fp_registers_.push_back(new mips::FRegister(mips::F24));
      fp_registers_.push_back(new mips::FRegister(mips::F25));
      fp_registers_.push_back(new mips::FRegister(mips::F26));
      fp_registers_.push_back(new mips::FRegister(mips::F27));
      fp_registers_.push_back(new mips::FRegister(mips::F28));
      fp_registers_.push_back(new mips::FRegister(mips::F29));
      fp_registers_.push_back(new mips::FRegister(mips::F30));
      fp_registers_.push_back(new mips::FRegister(mips::F31));
    }
  }

  void TearDown() OVERRIDE {
    AssemblerTest::TearDown();
    STLDeleteElements(&registers_);
    STLDeleteElements(&fp_registers_);
  }

  std::vector<mips::Register*> GetRegisters() OVERRIDE {
    return registers_;
  }

  std::vector<mips::FRegister*> GetFPRegisters() OVERRIDE {
    return fp_registers_;
  }

  uint32_t CreateImmediate(int64_t imm_value) OVERRIDE {
    return imm_value;
  }

  std::string GetSecondaryRegisterName(const mips::Register& reg) OVERRIDE {
    CHECK(secondary_register_names_.find(reg) != secondary_register_names_.end());
    return secondary_register_names_[reg];
  }

  std::string RepeatInsn(size_t count, const std::string& insn) {
    std::string result;
    for (; count != 0u; --count) {
      result += insn;
    }
    return result;
  }

  void BranchCondTwoRegsHelper(void (mips::MipsAssembler::*f)(mips::Register,
                                                              mips::Register,
                                                              mips::MipsLabel*),
                               const std::string& instr_name) {
    mips::MipsLabel label;
    (Base::GetAssembler()->*f)(mips::A0, mips::A1, &label);
    constexpr size_t kAdduCount1 = 63;
    for (size_t i = 0; i != kAdduCount1; ++i) {
      __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
    }
    __ Bind(&label);
    constexpr size_t kAdduCount2 = 64;
    for (size_t i = 0; i != kAdduCount2; ++i) {
      __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
    }
    (Base::GetAssembler()->*f)(mips::A2, mips::A3, &label);

    std::string expected =
        ".set noreorder\n" +
        instr_name + " $a0, $a1, 1f\n"
        "nop\n" +
        RepeatInsn(kAdduCount1, "addu $zero, $zero, $zero\n") +
        "1:\n" +
        RepeatInsn(kAdduCount2, "addu $zero, $zero, $zero\n") +
        instr_name + " $a2, $a3, 1b\n"
        "nop\n";
    DriverStr(expected, instr_name);
  }

 private:
  std::vector<mips::Register*> registers_;
  std::map<mips::Register, std::string, MIPSCpuRegisterCompare> secondary_register_names_;

  std::vector<mips::FRegister*> fp_registers_;
  std::unique_ptr<const MipsInstructionSetFeatures> instruction_set_features_;
};


TEST_F(AssemblerMIPS32r6Test, Toolchain) {
  EXPECT_TRUE(CheckTools());
}

TEST_F(AssemblerMIPS32r6Test, MulR6) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::MulR6, "mul ${reg1}, ${reg2}, ${reg3}"), "MulR6");
}

TEST_F(AssemblerMIPS32r6Test, MuhR6) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::MuhR6, "muh ${reg1}, ${reg2}, ${reg3}"), "MuhR6");
}

TEST_F(AssemblerMIPS32r6Test, MuhuR6) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::MuhuR6, "muhu ${reg1}, ${reg2}, ${reg3}"), "MuhuR6");
}

TEST_F(AssemblerMIPS32r6Test, DivR6) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::DivR6, "div ${reg1}, ${reg2}, ${reg3}"), "DivR6");
}

TEST_F(AssemblerMIPS32r6Test, ModR6) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::ModR6, "mod ${reg1}, ${reg2}, ${reg3}"), "ModR6");
}

TEST_F(AssemblerMIPS32r6Test, DivuR6) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::DivuR6, "divu ${reg1}, ${reg2}, ${reg3}"), "DivuR6");
}

TEST_F(AssemblerMIPS32r6Test, ModuR6) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::ModuR6, "modu ${reg1}, ${reg2}, ${reg3}"), "ModuR6");
}

//////////
// MISC //
//////////

TEST_F(AssemblerMIPS32r6Test, Aui) {
  DriverStr(RepeatRRIb(&mips::MipsAssembler::Aui, 16, "aui ${reg1}, ${reg2}, {imm}"), "Aui");
}

TEST_F(AssemblerMIPS32r6Test, Auipc) {
  DriverStr(RepeatRIb(&mips::MipsAssembler::Auipc, 16, "auipc ${reg}, {imm}"), "Auipc");
}

TEST_F(AssemblerMIPS32r6Test, Lwpc) {
  // Lwpc() takes an unsigned 19-bit immediate, while the GNU assembler needs a signed offset,
  // hence the sign extension from bit 18 with `imm - ((imm & 0x40000) << 1)`.
  // The GNU assembler also wants the offset to be a multiple of 4, which it will shift right
  // by 2 positions when encoding, hence `<< 2` to compensate for that shift.
  // We capture the value of the immediate with `.set imm, {imm}` because the value is needed
  // twice for the sign extension, but `{imm}` is substituted only once.
  const char* code = ".set imm, {imm}\nlw ${reg}, ((imm - ((imm & 0x40000) << 1)) << 2)($pc)";
  DriverStr(RepeatRIb(&mips::MipsAssembler::Lwpc, 19, code), "Lwpc");
}

TEST_F(AssemblerMIPS32r6Test, Addiupc) {
  // The comment from the Lwpc() test applies to this Addiupc() test as well.
  const char* code = ".set imm, {imm}\naddiupc ${reg}, (imm - ((imm & 0x40000) << 1)) << 2";
  DriverStr(RepeatRIb(&mips::MipsAssembler::Addiupc, 19, code), "Addiupc");
}

TEST_F(AssemblerMIPS32r6Test, Bitswap) {
  DriverStr(RepeatRR(&mips::MipsAssembler::Bitswap, "bitswap ${reg1}, ${reg2}"), "bitswap");
}

TEST_F(AssemblerMIPS32r6Test, Lsa) {
  DriverStr(RepeatRRRIb(&mips::MipsAssembler::Lsa,
                        2,
                        "lsa ${reg1}, ${reg2}, ${reg3}, {imm}",
                        1),
            "lsa");
}

TEST_F(AssemblerMIPS32r6Test, Seleqz) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::Seleqz, "seleqz ${reg1}, ${reg2}, ${reg3}"),
            "seleqz");
}

TEST_F(AssemblerMIPS32r6Test, Selnez) {
  DriverStr(RepeatRRR(&mips::MipsAssembler::Selnez, "selnez ${reg1}, ${reg2}, ${reg3}"),
            "selnez");
}

TEST_F(AssemblerMIPS32r6Test, ClzR6) {
  DriverStr(RepeatRR(&mips::MipsAssembler::ClzR6, "clz ${reg1}, ${reg2}"), "clzR6");
}

TEST_F(AssemblerMIPS32r6Test, CloR6) {
  DriverStr(RepeatRR(&mips::MipsAssembler::CloR6, "clo ${reg1}, ${reg2}"), "cloR6");
}

////////////////////
// FLOATING POINT //
////////////////////

TEST_F(AssemblerMIPS32r6Test, SelS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::SelS, "sel.s ${reg1}, ${reg2}, ${reg3}"), "sel.s");
}

TEST_F(AssemblerMIPS32r6Test, SelD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::SelD, "sel.d ${reg1}, ${reg2}, ${reg3}"), "sel.d");
}

TEST_F(AssemblerMIPS32r6Test, SeleqzS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::SeleqzS, "seleqz.s ${reg1}, ${reg2}, ${reg3}"),
            "seleqz.s");
}

TEST_F(AssemblerMIPS32r6Test, SeleqzD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::SeleqzD, "seleqz.d ${reg1}, ${reg2}, ${reg3}"),
            "seleqz.d");
}

TEST_F(AssemblerMIPS32r6Test, SelnezS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::SelnezS, "selnez.s ${reg1}, ${reg2}, ${reg3}"),
            "selnez.s");
}

TEST_F(AssemblerMIPS32r6Test, SelnezD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::SelnezD, "selnez.d ${reg1}, ${reg2}, ${reg3}"),
            "selnez.d");
}

TEST_F(AssemblerMIPS32r6Test, ClassS) {
  DriverStr(RepeatFF(&mips::MipsAssembler::ClassS, "class.s ${reg1}, ${reg2}"), "class.s");
}

TEST_F(AssemblerMIPS32r6Test, ClassD) {
  DriverStr(RepeatFF(&mips::MipsAssembler::ClassD, "class.d ${reg1}, ${reg2}"), "class.d");
}

TEST_F(AssemblerMIPS32r6Test, MinS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::MinS, "min.s ${reg1}, ${reg2}, ${reg3}"), "min.s");
}

TEST_F(AssemblerMIPS32r6Test, MinD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::MinD, "min.d ${reg1}, ${reg2}, ${reg3}"), "min.d");
}

TEST_F(AssemblerMIPS32r6Test, MaxS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::MaxS, "max.s ${reg1}, ${reg2}, ${reg3}"), "max.s");
}

TEST_F(AssemblerMIPS32r6Test, MaxD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::MaxD, "max.d ${reg1}, ${reg2}, ${reg3}"), "max.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpUnS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUnS, "cmp.un.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.un.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpEqS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpEqS, "cmp.eq.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.eq.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpUeqS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUeqS, "cmp.ueq.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ueq.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpLtS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpLtS, "cmp.lt.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.lt.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpUltS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUltS, "cmp.ult.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ult.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpLeS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpLeS, "cmp.le.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.le.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpUleS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUleS, "cmp.ule.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ule.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpOrS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpOrS, "cmp.or.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.or.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpUneS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUneS, "cmp.une.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.une.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpNeS) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpNeS, "cmp.ne.s ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ne.s");
}

TEST_F(AssemblerMIPS32r6Test, CmpUnD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUnD, "cmp.un.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.un.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpEqD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpEqD, "cmp.eq.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.eq.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpUeqD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUeqD, "cmp.ueq.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ueq.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpLtD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpLtD, "cmp.lt.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.lt.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpUltD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUltD, "cmp.ult.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ult.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpLeD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpLeD, "cmp.le.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.le.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpUleD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUleD, "cmp.ule.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ule.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpOrD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpOrD, "cmp.or.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.or.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpUneD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpUneD, "cmp.une.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.une.d");
}

TEST_F(AssemblerMIPS32r6Test, CmpNeD) {
  DriverStr(RepeatFFF(&mips::MipsAssembler::CmpNeD, "cmp.ne.d ${reg1}, ${reg2}, ${reg3}"),
            "cmp.ne.d");
}

TEST_F(AssemblerMIPS32r6Test, LoadDFromOffset) {
  __ LoadDFromOffset(mips::F0, mips::A0, -0x8000);
  __ LoadDFromOffset(mips::F0, mips::A0, +0);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x7FF8);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x7FFB);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x7FFC);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x7FFF);
  __ LoadDFromOffset(mips::F0, mips::A0, -0xFFF0);
  __ LoadDFromOffset(mips::F0, mips::A0, -0x8008);
  __ LoadDFromOffset(mips::F0, mips::A0, -0x8001);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x8000);
  __ LoadDFromOffset(mips::F0, mips::A0, +0xFFF0);
  __ LoadDFromOffset(mips::F0, mips::A0, -0x17FE8);
  __ LoadDFromOffset(mips::F0, mips::A0, -0x0FFF8);
  __ LoadDFromOffset(mips::F0, mips::A0, -0x0FFF1);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x0FFF1);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x0FFF8);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x17FE8);
  __ LoadDFromOffset(mips::F0, mips::A0, -0x17FF0);
  __ LoadDFromOffset(mips::F0, mips::A0, -0x17FE9);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x17FE9);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x17FF0);
  __ LoadDFromOffset(mips::F0, mips::A0, +0x12345678);

  const char* expected =
      "ldc1 $f0, -0x8000($a0)\n"
      "ldc1 $f0, 0($a0)\n"
      "ldc1 $f0, 0x7FF8($a0)\n"
      "lwc1 $f0, 0x7FFB($a0)\n"
      "lw $t8, 0x7FFF($a0)\n"
      "mthc1 $t8, $f0\n"
      "addiu $at, $a0, 0x7FF8\n"
      "lwc1 $f0, 4($at)\n"
      "lw $t8, 8($at)\n"
      "mthc1 $t8, $f0\n"
      "addiu $at, $a0, 0x7FF8\n"
      "lwc1 $f0, 7($at)\n"
      "lw $t8, 11($at)\n"
      "mthc1 $t8, $f0\n"
      "addiu $at, $a0, -0x7FF8\n"
      "ldc1 $f0, -0x7FF8($at)\n"
      "addiu $at, $a0, -0x7FF8\n"
      "ldc1 $f0, -0x10($at)\n"
      "addiu $at, $a0, -0x7FF8\n"
      "lwc1 $f0, -9($at)\n"
      "lw $t8, -5($at)\n"
      "mthc1 $t8, $f0\n"
      "addiu $at, $a0, 0x7FF8\n"
      "ldc1 $f0, 8($at)\n"
      "addiu $at, $a0, 0x7FF8\n"
      "ldc1 $f0, 0x7FF8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "ldc1 $f0, -0x7FE8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "ldc1 $f0, 0x8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "lwc1 $f0, 0xF($at)\n"
      "lw $t8, 0x13($at)\n"
      "mthc1 $t8, $f0\n"
      "aui $at, $a0, 0x1\n"
      "lwc1 $f0, -0xF($at)\n"
      "lw $t8, -0xB($at)\n"
      "mthc1 $t8, $f0\n"
      "aui $at, $a0, 0x1\n"
      "ldc1 $f0, -0x8($at)\n"
      "aui $at, $a0, 0x1\n"
      "ldc1 $f0, 0x7FE8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "ldc1 $f0, -0x7FF0($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "lwc1 $f0, -0x7FE9($at)\n"
      "lw $t8, -0x7FE5($at)\n"
      "mthc1 $t8, $f0\n"
      "aui $at, $a0, 0x1\n"
      "lwc1 $f0, 0x7FE9($at)\n"
      "lw $t8, 0x7FED($at)\n"
      "mthc1 $t8, $f0\n"
      "aui $at, $a0, 0x1\n"
      "ldc1 $f0, 0x7FF0($at)\n"
      "aui $at, $a0, 0x1234\n"
      "ldc1 $f0, 0x5678($at)\n";
  DriverStr(expected, "LoadDFromOffset");
}

TEST_F(AssemblerMIPS32r6Test, StoreDToOffset) {
  __ StoreDToOffset(mips::F0, mips::A0, -0x8000);
  __ StoreDToOffset(mips::F0, mips::A0, +0);
  __ StoreDToOffset(mips::F0, mips::A0, +0x7FF8);
  __ StoreDToOffset(mips::F0, mips::A0, +0x7FFB);
  __ StoreDToOffset(mips::F0, mips::A0, +0x7FFC);
  __ StoreDToOffset(mips::F0, mips::A0, +0x7FFF);
  __ StoreDToOffset(mips::F0, mips::A0, -0xFFF0);
  __ StoreDToOffset(mips::F0, mips::A0, -0x8008);
  __ StoreDToOffset(mips::F0, mips::A0, -0x8001);
  __ StoreDToOffset(mips::F0, mips::A0, +0x8000);
  __ StoreDToOffset(mips::F0, mips::A0, +0xFFF0);
  __ StoreDToOffset(mips::F0, mips::A0, -0x17FE8);
  __ StoreDToOffset(mips::F0, mips::A0, -0x0FFF8);
  __ StoreDToOffset(mips::F0, mips::A0, -0x0FFF1);
  __ StoreDToOffset(mips::F0, mips::A0, +0x0FFF1);
  __ StoreDToOffset(mips::F0, mips::A0, +0x0FFF8);
  __ StoreDToOffset(mips::F0, mips::A0, +0x17FE8);
  __ StoreDToOffset(mips::F0, mips::A0, -0x17FF0);
  __ StoreDToOffset(mips::F0, mips::A0, -0x17FE9);
  __ StoreDToOffset(mips::F0, mips::A0, +0x17FE9);
  __ StoreDToOffset(mips::F0, mips::A0, +0x17FF0);
  __ StoreDToOffset(mips::F0, mips::A0, +0x12345678);

  const char* expected =
      "sdc1 $f0, -0x8000($a0)\n"
      "sdc1 $f0, 0($a0)\n"
      "sdc1 $f0, 0x7FF8($a0)\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, 0x7FFB($a0)\n"
      "sw $t8, 0x7FFF($a0)\n"
      "addiu $at, $a0, 0x7FF8\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, 4($at)\n"
      "sw $t8, 8($at)\n"
      "addiu $at, $a0, 0x7FF8\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, 7($at)\n"
      "sw $t8, 11($at)\n"
      "addiu $at, $a0, -0x7FF8\n"
      "sdc1 $f0, -0x7FF8($at)\n"
      "addiu $at, $a0, -0x7FF8\n"
      "sdc1 $f0, -0x10($at)\n"
      "addiu $at, $a0, -0x7FF8\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, -9($at)\n"
      "sw $t8, -5($at)\n"
      "addiu $at, $a0, 0x7FF8\n"
      "sdc1 $f0, 8($at)\n"
      "addiu $at, $a0, 0x7FF8\n"
      "sdc1 $f0, 0x7FF8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "sdc1 $f0, -0x7FE8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "sdc1 $f0, 0x8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, 0xF($at)\n"
      "sw $t8, 0x13($at)\n"
      "aui $at, $a0, 0x1\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, -0xF($at)\n"
      "sw $t8, -0xB($at)\n"
      "aui $at, $a0, 0x1\n"
      "sdc1 $f0, -0x8($at)\n"
      "aui $at, $a0, 0x1\n"
      "sdc1 $f0, 0x7FE8($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "sdc1 $f0, -0x7FF0($at)\n"
      "aui $at, $a0, 0xFFFF\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, -0x7FE9($at)\n"
      "sw $t8, -0x7FE5($at)\n"
      "aui $at, $a0, 0x1\n"
      "mfhc1 $t8, $f0\n"
      "swc1 $f0, 0x7FE9($at)\n"
      "sw $t8, 0x7FED($at)\n"
      "aui $at, $a0, 0x1\n"
      "sdc1 $f0, 0x7FF0($at)\n"
      "aui $at, $a0, 0x1234\n"
      "sdc1 $f0, 0x5678($at)\n";
  DriverStr(expected, "StoreDToOffset");
}

TEST_F(AssemblerMIPS32r6Test, LoadFarthestNearLabelAddress) {
  mips::MipsLabel label;
  __ LoadLabelAddress(mips::V0, mips::ZERO, &label);
  constexpr size_t kAdduCount = 0x3FFDE;
  for (size_t i = 0; i != kAdduCount; ++i) {
    __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
  }
  __ Bind(&label);

  std::string expected =
      "lapc $v0, 1f\n" +
      RepeatInsn(kAdduCount, "addu $zero, $zero, $zero\n") +
      "1:\n";
  DriverStr(expected, "LoadFarthestNearLabelAddress");
}

TEST_F(AssemblerMIPS32r6Test, LoadNearestFarLabelAddress) {
  mips::MipsLabel label;
  __ LoadLabelAddress(mips::V0, mips::ZERO, &label);
  constexpr size_t kAdduCount = 0x3FFDF;
  for (size_t i = 0; i != kAdduCount; ++i) {
    __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
  }
  __ Bind(&label);

  std::string expected =
      "1:\n"
      "auipc $at, %hi(2f - 1b)\n"
      "addiu $v0, $at, %lo(2f - 1b)\n" +
      RepeatInsn(kAdduCount, "addu $zero, $zero, $zero\n") +
      "2:\n";
  DriverStr(expected, "LoadNearestFarLabelAddress");
}

TEST_F(AssemblerMIPS32r6Test, LoadFarthestNearLiteral) {
  mips::Literal* literal = __ NewLiteral<uint32_t>(0x12345678);
  __ LoadLiteral(mips::V0, mips::ZERO, literal);
  constexpr size_t kAdduCount = 0x3FFDE;
  for (size_t i = 0; i != kAdduCount; ++i) {
    __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
  }

  std::string expected =
      "lwpc $v0, 1f\n" +
      RepeatInsn(kAdduCount, "addu $zero, $zero, $zero\n") +
      "1:\n"
      ".word 0x12345678\n";
  DriverStr(expected, "LoadFarthestNearLiteral");
}

TEST_F(AssemblerMIPS32r6Test, LoadNearestFarLiteral) {
  mips::Literal* literal = __ NewLiteral<uint32_t>(0x12345678);
  __ LoadLiteral(mips::V0, mips::ZERO, literal);
  constexpr size_t kAdduCount = 0x3FFDF;
  for (size_t i = 0; i != kAdduCount; ++i) {
    __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
  }

  std::string expected =
      "1:\n"
      "auipc $at, %hi(2f - 1b)\n"
      "lw $v0, %lo(2f - 1b)($at)\n" +
      RepeatInsn(kAdduCount, "addu $zero, $zero, $zero\n") +
      "2:\n"
      ".word 0x12345678\n";
  DriverStr(expected, "LoadNearestFarLiteral");
}

//////////////
// BRANCHES //
//////////////

TEST_F(AssemblerMIPS32r6Test, ImpossibleReordering) {
  mips::MipsLabel label;
  __ SetReorder(true);
  __ Bind(&label);

  __ CmpLtD(mips::F0, mips::F2, mips::F4);
  __ Bc1nez(mips::F0, &label);  // F0 dependency.

  __ MulD(mips::F10, mips::F2, mips::F4);
  __ Bc1eqz(mips::F10, &label);  // F10 dependency.

  std::string expected =
      ".set noreorder\n"
      "1:\n"

      "cmp.lt.d $f0, $f2, $f4\n"
      "bc1nez $f0, 1b\n"
      "nop\n"

      "mul.d $f10, $f2, $f4\n"
      "bc1eqz $f10, 1b\n"
      "nop\n";
  DriverStr(expected, "ImpossibleReordering");
}

TEST_F(AssemblerMIPS32r6Test, Reordering) {
  mips::MipsLabel label;
  __ SetReorder(true);
  __ Bind(&label);

  __ CmpLtD(mips::F0, mips::F2, mips::F4);
  __ Bc1nez(mips::F2, &label);

  __ MulD(mips::F0, mips::F2, mips::F4);
  __ Bc1eqz(mips::F4, &label);

  std::string expected =
      ".set noreorder\n"
      "1:\n"

      "bc1nez $f2, 1b\n"
      "cmp.lt.d $f0, $f2, $f4\n"

      "bc1eqz $f4, 1b\n"
      "mul.d $f0, $f2, $f4\n";
  DriverStr(expected, "Reordering");
}

TEST_F(AssemblerMIPS32r6Test, SetReorder) {
  mips::MipsLabel label1, label2, label3, label4;

  __ SetReorder(true);
  __ Bind(&label1);
  __ Addu(mips::T0, mips::T1, mips::T2);
  __ Bc1nez(mips::F0, &label1);

  __ SetReorder(false);
  __ Bind(&label2);
  __ Addu(mips::T0, mips::T1, mips::T2);
  __ Bc1nez(mips::F0, &label2);

  __ SetReorder(true);
  __ Bind(&label3);
  __ Addu(mips::T0, mips::T1, mips::T2);
  __ Bc1eqz(mips::F0, &label3);

  __ SetReorder(false);
  __ Bind(&label4);
  __ Addu(mips::T0, mips::T1, mips::T2);
  __ Bc1eqz(mips::F0, &label4);

  std::string expected =
      ".set noreorder\n"
      "1:\n"
      "bc1nez $f0, 1b\n"
      "addu $t0, $t1, $t2\n"

      "2:\n"
      "addu $t0, $t1, $t2\n"
      "bc1nez $f0, 2b\n"
      "nop\n"

      "3:\n"
      "bc1eqz $f0, 3b\n"
      "addu $t0, $t1, $t2\n"

      "4:\n"
      "addu $t0, $t1, $t2\n"
      "bc1eqz $f0, 4b\n"
      "nop\n";
  DriverStr(expected, "SetReorder");
}

TEST_F(AssemblerMIPS32r6Test, LongBranchReorder) {
  mips::MipsLabel label;
  __ SetReorder(true);
  __ Subu(mips::T0, mips::T1, mips::T2);
  __ Bc1nez(mips::F0, &label);
  constexpr uint32_t kAdduCount1 = (1u << 15) + 1;
  for (uint32_t i = 0; i != kAdduCount1; ++i) {
    __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
  }
  __ Bind(&label);
  constexpr uint32_t kAdduCount2 = (1u << 15) + 1;
  for (uint32_t i = 0; i != kAdduCount2; ++i) {
    __ Addu(mips::ZERO, mips::ZERO, mips::ZERO);
  }
  __ Subu(mips::T0, mips::T1, mips::T2);
  __ Bc1eqz(mips::F0, &label);

  uint32_t offset_forward = 2 + kAdduCount1;  // 2: account for auipc and jic.
  offset_forward <<= 2;
  offset_forward += (offset_forward & 0x8000) << 1;  // Account for sign extension in jic.

  uint32_t offset_back = -(kAdduCount2 + 2);  // 2: account for subu and bc1nez.
  offset_back <<= 2;
  offset_back += (offset_back & 0x8000) << 1;  // Account for sign extension in jic.

  std::ostringstream oss;
  oss <<
      ".set noreorder\n"
      "subu $t0, $t1, $t2\n"
      "bc1eqz $f0, 1f\n"
      "auipc $at, 0x" << std::hex << High16Bits(offset_forward) << "\n"
      "jic $at, 0x" << std::hex << Low16Bits(offset_forward) << "\n"
      "1:\n" <<
      RepeatInsn(kAdduCount1, "addu $zero, $zero, $zero\n") <<
      "2:\n" <<
      RepeatInsn(kAdduCount2, "addu $zero, $zero, $zero\n") <<
      "subu $t0, $t1, $t2\n"
      "bc1nez $f0, 3f\n"
      "auipc $at, 0x" << std::hex << High16Bits(offset_back) << "\n"
      "jic $at, 0x" << std::hex << Low16Bits(offset_back) << "\n"
      "3:\n";
  std::string expected = oss.str();
  DriverStr(expected, "LongBeqc");
}

// TODO: MipsAssembler::Bc
//       MipsAssembler::Jic
//       MipsAssembler::Jialc
//       MipsAssembler::Bltc
//       MipsAssembler::Bltzc
//       MipsAssembler::Bgtzc
//       MipsAssembler::Bgec
//       MipsAssembler::Bgezc
//       MipsAssembler::Blezc
//       MipsAssembler::Bltuc
//       MipsAssembler::Bgeuc
//       MipsAssembler::Beqc
//       MipsAssembler::Bnec
//       MipsAssembler::Beqzc
//       MipsAssembler::Bnezc
//       MipsAssembler::Bc1eqz
//       MipsAssembler::Bc1nez
//       MipsAssembler::Buncond
//       MipsAssembler::Bcond
//       MipsAssembler::Call

// TODO:  AssemblerMIPS32r6Test.B
//        AssemblerMIPS32r6Test.Beq
//        AssemblerMIPS32r6Test.Bne
//        AssemblerMIPS32r6Test.Beqz
//        AssemblerMIPS32r6Test.Bnez
//        AssemblerMIPS32r6Test.Bltz
//        AssemblerMIPS32r6Test.Bgez
//        AssemblerMIPS32r6Test.Blez
//        AssemblerMIPS32r6Test.Bgtz
//        AssemblerMIPS32r6Test.Blt
//        AssemblerMIPS32r6Test.Bge
//        AssemblerMIPS32r6Test.Bltu
//        AssemblerMIPS32r6Test.Bgeu

#undef __

}  // namespace art
