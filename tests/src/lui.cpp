#include <gtest/gtest.h>
#include <HartFixture.hpp>

/* @EncodeAsm: InstructionLUI.rv32gc
// LUI (load upper immediate) is used to build 32-bit constants and uses the
// U-type format. LUI places the U-immediate value in the top 20 bits of the
// destination register rd, filling in the lowest 12 bits with zeros.
    lui a0, 0x00000
    lui a1, 0xfffff
    lui a2, 0x00001
    lui a3, 0x80000
    lui a4, 0x7ffff
    j 0
*/
#include <InstructionLUI.rv32gc.h>
TEST_F(HartTest32, InstructionLUI) {
    bus.Write32(0x80000000, sizeof(InstructionLUI_rv32gc_bytes), (char*)InstructionLUI_rv32gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    hart.state.regs[RISCV::abiRegNum::a0] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a1] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a2] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a3] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a4] = 0xa0a0'a0a0;
    RunAtLeast(5);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint32_t)0x0000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint32_t)0xffff'f000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint32_t)0x0000'1000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a3], (__uint32_t)0x8000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a4], (__uint32_t)0x7fff'f000);
}

/* @EncodeAsm: InstructionLUI.rv64gc
// LUI (load upper immediate) uses the same opcode as RV32I. LUI places the
// 20-bit U-immediate into bits 31–12 of register rd and places zero in the
// lowest 12 bits. The 32-bit result is sign-extended to 64 bits.
    lui a0, 0x00000
    lui a1, 0xfffff
    lui a2, 0x00001
    lui a3, 0x80000
    lui a4, 0x7ffff
    lui a5, 0xa5a5a
    lui a6, 0x5a5a5
    j 0
*/
#include <InstructionLUI.rv64gc.h>
TEST_F(HartTest64, InstructionLUI) {

    bus.Write32(0x80000000, sizeof(InstructionLUI_rv64gc_bytes), (char*)InstructionLUI_rv64gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    
    RunAtLeast(7);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint64_t)0x0000'0000'0000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint64_t)0xffff'ffff'ffff'f000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint64_t)0x0000'0000'0000'1000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a3], (__uint64_t)0xffff'ffff'8000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a4], (__uint64_t)0x0000'0000'7fff'f000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a5], (__uint64_t)0xffff'ffff'a5a5'a000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a6], (__uint64_t)0x0000'0000'5a5a'5000);
}
