#include <gtest/gtest.h>
#include <HartFixture.hpp>

// AUIPC (add upper immediate to pc) is used to build pc-relative addresses and
// uses the U-type format. AUIPC forms a 32-bit offset from the 20-bit
// U-immediate, filling in the lowest 12 bits with zeros, adds this offset to
// the address of the AUIPC instruction, then places the result in register rd.

/* @EncodeAsm: InstructionAUIPC.rv32gc
    auipc a0, 0x00000
    auipc a1, 0xfffff
    auipc a2, 0x00001
    auipc a3, 0x80000
    auipc a4, 0x7ffff
    j 0
*/
#include <InstructionAUIPC.rv32gc.h>
TEST_F(HartTest32, InstructionAUIPC) {

    bus.Write32(0x80000000, sizeof(InstructionAUIPC_rv32gc_bytes), (char*)InstructionAUIPC_rv32gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    hart.state.regs[RISCV::abiRegNum::a0] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a1] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a2] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a3] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a4] = 0xa0a0'a0a0;
    RunAtLeast(5);

    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint32_t)0x8000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint32_t)0x7fff'f004);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint32_t)0x8000'1008);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a3], (__uint32_t)0x0000'000c);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a4], (__uint32_t)0xffff'f010);

    bus.Write32(0x00000000, sizeof(InstructionAUIPC_rv32gc_bytes), (char*)InstructionAUIPC_rv32gc_bytes);
    hart.state.resetVector = 0x00000000;
    hart.Reset();
    hart.state.regs[RISCV::abiRegNum::a0] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a1] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a2] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a3] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a4] = 0xa0a0'a0a0;
    RunAtLeast(5);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint32_t)0x0000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint32_t)0xffff'f004);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint32_t)0x0000'1008);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a3], (__uint32_t)0x8000'000c);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a4], (__uint32_t)0x7fff'f010);
}

/* @EncodeAsm: InstructionAUIPC.rv64gc
// AUIPC (add upper immediate to pc) uses the same opcode as RV32I. AUIPC is
// used to build pc- relative addresses and uses the U-type format. AUIPC
// appends 12 low-order zero bits to the 20-bit U-immediate, sign-extends the
// result to 64 bits, adds it to the address of the AUIPC instruction, then
// places the result in register rd.
    auipc a0, 0x00000
    auipc a1, 0xfffff
    auipc a2, 0x00001
    auipc a3, 0x80000
    auipc a4, 0x7ffff
    j 0
*/
#include <InstructionAUIPC.rv64gc.h>
TEST_F(HartTest64, InstructionAUIPC) {

    bus.Write32(0x80000000, sizeof(InstructionAUIPC_rv64gc_bytes), (char*)InstructionAUIPC_rv64gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    hart.state.regs[RISCV::abiRegNum::a0] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a1] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a2] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a3] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a4] = 0xa0a0'a0a0;
    RunAtLeast(5);

    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint64_t)0x0000'0000'8000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint64_t)0x0000'0000'7fff'f004);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint64_t)0x0000'0000'8000'1008);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a3], (__uint64_t)0x0000'0000'0000'000c);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a4], (__uint64_t)0x0000'0000'ffff'f010);

    bus.Write32(0x00000000, sizeof(InstructionAUIPC_rv32gc_bytes), (char*)InstructionAUIPC_rv32gc_bytes);
    hart.state.resetVector = 0x00000000;
    hart.Reset();
    hart.state.regs[RISCV::abiRegNum::a0] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a1] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a2] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a3] = 0xa0a0'a0a0;
    hart.state.regs[RISCV::abiRegNum::a4] = 0xa0a0'a0a0;
    RunAtLeast(5);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint64_t)0x0000'0000'0000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint64_t)0xffff'ffff'ffff'f004);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint64_t)0x0000'0000'0000'1008);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a3], (__uint64_t)0xffff'ffff'8000'000c);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a4], (__uint64_t)0x0000'0000'7fff'f010);
}
