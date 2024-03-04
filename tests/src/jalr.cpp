#include <gtest/gtest.h>
#include <HartFixture.hpp>

// The indirect jump instruction JALR (jump and link register) uses the I-type
// encoding. The target address is obtained by adding the sign-extended 12-bit
// I-immediate to the register rs1, then setting the least-significant bit of
// the result to zero. The address of the instruction following the jump (pc+4)
// is written to register rd. Register x0 can be used as the destination if the
// result is not required.

// Basic functionality - done
// Bit 0 of the target address is always zero {imm even, imm odd} X {rs1 even, rs1 odd}
// jump targets are sane with respect to overflows

/* @EncodeAsm: InstructionJALR.rv32gc

    li a0, 0x80000000
    li a2, 0
    j test_start

    addi a2, a2, 1
    jr a1

test_start:
    jalr a1, a0, 8
    jalr a1, a0, 9
    li a0, 0x80000001
    jalr a1, a0, 7
    jalr a1, a0, 8
    j 0

*/
#include <InstructionJALR.rv32gc.h>
TEST_F(HartTest32, InstructionJALR) {
    bus.Write32(0x80000000, sizeof(InstructionJALR_rv32gc_bytes), (char*)InstructionJALR_rv32gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    RunAtLeast(20);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint32_t)0x4);
}
