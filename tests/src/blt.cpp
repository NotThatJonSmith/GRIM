#include <gtest/gtest.h>
#include <HartFixture.hpp>

/* @EncodeAsm: InstructionBLT.rv32gc InstructionBLT.rv64gc

    li a0, -2
    li a1, -1
    li a2, 0
    li a3, 1
    li a4, 2
    
    li a6, 0xdeadbeef

    blt a4, a4, test_failure
    blt a4, a3, test_failure
    blt a4, a2, test_failure
    blt a4, a1, test_failure
    blt a4, a0, test_failure

    blt a3, a4, continue1
    j test_failure
continue1:
    blt a3, a3, test_failure
    blt a3, a2, test_failure
    blt a3, a1, test_failure
    blt a3, a0, test_failure

    blt a2, a4, continue2
    j test_failure
continue2:
    blt a2, a3, continue3
    li a6, 0x0d15ea5e
continue3:
    blt a2, a2, test_failure
    blt a2, a1, test_failure
    blt a2, a0, test_failure

    blt a1, a4, continue4
    j test_failure
continue4:
    blt a1, a3, continue5
    j test_failure
continue5:
    blt a1, a2, continue6
    j test_failure
continue6:
    blt a1, a1, test_failure
    blt a1, a0, test_failure

    blt a0, a4, continue7
    j test_failure
continue7:
    blt a0, a3, continue8
    j test_failure
continue8:
    blt a0, a2, continue9
    j test_failure
continue9:
    blt a0, a1, continue10
    j test_failure
continue10:
    blt a0, a0, test_failure

    li a6, 0x0d15ea5e
    j 0

test_failure:
    li a6, 0xdeadbeef
    j 0
*/

#include <InstructionBLT.rv32gc.h>
TEST_F(HartTest32, InstructionBLT) {
    bus.Write32(0x80000000, sizeof(InstructionBLT_rv32gc_bytes), (char*)InstructionBLT_rv32gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    RunAtLeast(50);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a6], (__uint64_t)0x0d15ea5e);
}

#include <InstructionBLT.rv64gc.h>
TEST_F(HartTest64, InstructionBLT) {
    bus.Write32(0x80000000, sizeof(InstructionBLT_rv64gc_bytes), (char*)InstructionBLT_rv64gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    RunAtLeast(50);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a6], (__uint64_t)0x0d15ea5e);
}
