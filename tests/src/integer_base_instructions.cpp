#include <gtest/gtest.h>
#include <HartFixture.hpp>

// TODO JALR

// TODO BEQ
// TODO BNE
// TODO BLT
// TODO BGE
// TODO BLTU
// TODO BGEU

// TODO LB
// TODO LH
// TODO LW
// TODO LD
// TODO LBU
// TODO LHU
// TODO LWU

// TODO SB
// TODO SH
// TODO SW

// TODO ADDI
// TODO ADDIW
// TODO SLTI
// TODO SLTIU
// TODO XORI
// TODO ORI
// TODO ANDI
// TODO SLLI
// TODO SLLIW
// TODO SRLI
// TODO SRLIW
// TODO SRAI
// TODO SRAIW

// TODO ADD
// TODO ADDW
// TODO SUB
// TODO SUBW
// TODO SLL
// TODO SLLW
// TODO SLT
// TODO SLTU
// TODO XOR
// TODO SRL
// TODO SRLW
// TODO SRA
// TODO SRAW
// TODO OR
// TODO AND

// TODO FENCE
// TODO FENCE.I

// TODO ECALL
// TODO EBREAK

// TODO CSRRW
// TODO CSRRS
// TODO CSRRC
// TODO CSRRWI
// TODO CSRRSI
// TODO CSRRCI

// TODO MUL
// TODO MULH
// TODO MULHSU
// TODO MULHU
// TODO DIV
// TODO DIVU
// TODO REM
// TODO REMU

// TODO MULW
// TODO DIVW
// TODO DIVUW
// TODO REMW
// TODO REMUW


/* @EncodeAsm: InstructionADDIW.rv64gc
    li a0, 2
    addiw a0, a0, -3
    j 0
*/
#include <InstructionADDIW.rv64gc.h>
TEST_F(HartTest64, Test2) {
    ASSERT_EQ(1, 1);
    bus.Write32(0x80000000, sizeof(InstructionADDIW_rv64gc_bytes), (char*)InstructionADDIW_rv64gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint64_t)0);
    RunAtLeast(3);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], 0xffff'ffff'ffff'ffff);
}
