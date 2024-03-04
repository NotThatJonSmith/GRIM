#include <gtest/gtest.h>
#include <HartFixture.hpp>

// The jump and link (JAL) instruction uses the J-type format, where the
// J-immediate encodes a signed offset in multiples of 2 bytes. The offset is
// sign-extended and added to the address of the jump instruction to form the
// jump target address. Jumps can therefore target a ±1 MiB range. JAL stores
// the address of the instruction following the jump (pc+4) into register rd.

/* @EncodeAsm: InstructionJAL.short.rv32gc
a:  jal a0, c
b:  jal a1, d
c:  jal a2, b
d:  j 0
*/
#include <InstructionJAL.short.rv32gc.h>
TEST_F(HartTest32, InstructionJAL_short) {
    bus.Write32(0x80000000, sizeof(InstructionJAL_short_rv32gc_bytes), (char*)InstructionJAL_short_rv32gc_bytes);
    hart.state.resetVector = 0x80000000;
    hart.Reset();
    RunAtLeast(4);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint32_t)0x8000'0004);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint32_t)0x8000'0008);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint32_t)0x8000'000c);
}

/* @EncodeAsm: InstructionJAL.short.rv64gc
a:  jal a0, c
b:  jal a1, d
c:  jal a2, b
d:  j 0
*/
#include <InstructionJAL.short.rv64gc.h>
TEST_F(HartTest64, InstructionJAL_short) {
    bus.Write32(0x80000000, sizeof(InstructionJAL_short_rv32gc_bytes), (char*)InstructionJAL_short_rv32gc_bytes);
    hart.state.resetVector = 0x80000000; // TODO put the test somewhere in high rv64gc-only PCs
    hart.Reset();
    RunAtLeast(4);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a0], (__uint64_t)0x0000'0000'8000'0004);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a1], (__uint64_t)0x0000'0000'8000'0008);
    ASSERT_EQ(hart.state.regs[RISCV::abiRegNum::a2], (__uint64_t)0x0000'0000'8000'000c);
}

/* @EncodeAsm: j0.rv32gc
    j 0
*/
#include <j0.rv32gc.h>
const unsigned char longest_fwd_jump[] = { 0x6F, 0xf5, 0xff, 0x7f };
const unsigned char longest_bwd_jump[] = { 0xEF, 0x05, 0x00, 0x80 };
TEST_F(HartTest32, InstructionJAL_long) {
    // Longest forward jump, no PC overflow
    bus.Write32(0x40000000, sizeof(longest_fwd_jump), (char*)longest_fwd_jump);
    bus.Write32(0x400ffffe, sizeof(j0_rv32gc_bytes), (char*)j0_rv32gc_bytes);
    hart.state.resetVector = 0x40000000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint32_t)0x400ffffe);
    // Longest backward jump, no PC overflow
    bus.Write32(0x40000000, sizeof(longest_bwd_jump), (char*)longest_bwd_jump);
    bus.Write32(0x3ff00000, sizeof(j0_rv32gc_bytes), (char*)j0_rv32gc_bytes);
    hart.state.resetVector = 0x40000000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint32_t)0x3ff00000);
    // Longest forward jump, PC overflow
    bus.Write32(0xfff10000, sizeof(longest_fwd_jump), (char*)longest_fwd_jump);
    bus.Write32(0x0000fffe, sizeof(j0_rv32gc_bytes), (char*)j0_rv32gc_bytes);
    hart.state.resetVector = 0xfff10000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint32_t)0x0000fffe);
    // Longest backward jump, PC underflow
    bus.Write32(0x00000000, sizeof(longest_bwd_jump), (char*)longest_bwd_jump);
    bus.Write32(0xfff00000, sizeof(j0_rv32gc_bytes), (char*)j0_rv32gc_bytes);
    hart.state.resetVector = 0x00000000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint32_t)0xfff00000);
}

/* @EncodeAsm: j0.rv64gc
    j 0
*/
#include <j0.rv64gc.h>
TEST_F(HartTest64, InstructionJAL_long) {
    // Longest forward jump, no PC overflow - same as rv32gc
    bus.Write32(0x40000000, sizeof(longest_fwd_jump), (char*)longest_fwd_jump);
    bus.Write32(0x400ffffe, sizeof(j0_rv64gc_bytes), (char*)j0_rv64gc_bytes);
    hart.state.resetVector = 0x40000000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint64_t)0x400ffffe);
    // Longest backward jump, no PC overflow
    bus.Write32(0x40000000, sizeof(longest_bwd_jump), (char*)longest_bwd_jump);
    bus.Write32(0x3ff00000, sizeof(j0_rv64gc_bytes), (char*)j0_rv64gc_bytes);
    hart.state.resetVector = 0x40000000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint64_t)0x3ff00000);
    // Longest forward jump, still no PC overflow because 64-bit; TODO adapt
    bus.Write32(0xfff10000, sizeof(longest_fwd_jump), (char*)longest_fwd_jump);
    bus.Write64(0x10000fffe, sizeof(j0_rv64gc_bytes), (char*)j0_rv64gc_bytes);
    hart.state.resetVector = 0xfff10000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint64_t)0x10000fffe);
    // Longest backward jump, PC underflow
    bus.Write32(0x00000000, sizeof(longest_bwd_jump), (char*)longest_bwd_jump);
    MappedPhysicalMemory f_mem(0x20000);
    bus.AddDevice64(&f_mem, 0xffff'ffff'fff0'0000, 0x20000);
    bus.Write64(0xffff'ffff'fff0'0000, sizeof(j0_rv64gc_bytes), (char*)j0_rv64gc_bytes);
    hart.state.resetVector = 0x00000000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint64_t)0xfffffffffff00000);
    // Longest forward jump, PC overflow
    bus.Write64(0xffff'ffff'fff1'0000, sizeof(longest_fwd_jump), (char*)longest_fwd_jump);
    bus.Write32(0x0000fffe, sizeof(j0_rv32gc_bytes), (char*)j0_rv32gc_bytes);
    hart.state.resetVector = 0xffff'ffff'fff1'0000;
    hart.Reset();
    RunAtLeast(2);
    ASSERT_EQ(hart.state.pc, (__uint32_t)0x0000fffe);
}

