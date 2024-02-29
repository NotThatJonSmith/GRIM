#include <gtest/gtest.h>

#include <Hart.hpp>
#include <Devices/Bus.hpp>
#include <Devices/MappedPhysicalMemory.hpp>

template<typename MXLEN_t>
class HartTest : public ::testing::Test {
protected:
    Hart<MXLEN_t> hart;
    MappedPhysicalMemory mem;
    Bus bus;

    HartTest() : hart(&bus, RISCV::stringToExtensions("imacsu")) {
        bus.AddDevice32((Device*)&mem, 0, 0xffffffff);
    }

    void RunAtLeast(unsigned int cycles) {
        unsigned int counter = 0;
        while (counter <= cycles)
            counter += hart.Tick();
    }

};

using HartTest64 = HartTest<__uint64_t>;

TEST_F(HartTest64, Test1) {
    ASSERT_EQ(1, 1);
}

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

/* @EncodeAsm: InstructionLUI.rv64gc
// LUI (load upper immediate) uses the same opcode as RV32I. LUI places the
// 20-bit U-immediate into bits 31–12 of register rd and places zero in the
// lowest 12 bits. The 32-bit result is sign-extended to 64 bits.
    lui a0, 0x00000
    lui a1, 0xfffff
    lui a2, 1
    lui a3, 0x80000
    lui a4, 0x7ffff
    lui a5, 0xa5a5a
    lui a6, 0x5a5a5
*/
#include <InstructionLUI.rv64gc.h>
TEST_F(HartTest64, InstructionLUI) {
    bus.Write32(0x80000000, sizeof(InstructionLUI_rv64gc_bytes), (char*)InstructionLUI_rv64gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    
    hart.Tick();
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint64_t)0x0000'0000'0000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint64_t)0xffff'ffff'ffff'f000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint64_t)0x0000'0000'0000'1000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a3], (__uint64_t)0xffff'ffff'8000'0000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a4], (__uint64_t)0x0000'0000'7fff'f000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a5], (__uint64_t)0xffff'ffff'a5a5'a000);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a6], (__uint64_t)0x0000'0000'5a5a'5000);
}
