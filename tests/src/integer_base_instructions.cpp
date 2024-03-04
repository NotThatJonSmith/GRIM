#include <gtest/gtest.h>
#include <HartFixture.hpp>

// LUI - done
// AUIPC - done
// JAL - done
// JALR - mostly done, PC overflow stuff is TODO

// BEQ
// BNE
// BLT - done
// BGE
// BLTU
// BGEU

// LB
// LH
// LW
// LD
// LBU
// LHU
// LWU

// SB
// SH
// SW

// ADDI
// ADDIW
// SLTI
// SLTIU
// XORI
// ORI
// ANDI
// SLLI
// SLLIW
// SRLI
// SRLIW
// SRAI
// SRAIW

// ADD
// ADDW
// SUB
// SUBW
// SLL
// SLLW
// SLT
// SLTU
// XOR
// SRL
// SRLW
// SRA
// SRAW
// OR
// AND

// FENCE
// FENCE.I
// ECALL
// EBREAK

// CSRRW
// CSRRS
// CSRRC
// CSRRWI
// CSRRSI
// CSRRCI

// MUL
// MULH
// MULHSU
// MULHU
// DIV
// DIVU
// REM
// REMU
// MULW
// DIVW
// DIVUW
// REMW
// REMUW

// LR.W
// SC.W
// AMOSWAP.W
// AMOADD.W
// AMOXOR.W
// AMOAND.W
// AMOOR.W
// AMOMIN.W
// AMOMAX.W
// AMOMINU.W
// AMOMAXU.W
// LR.D
// SC.D
// AMOSWAP.D
// AMOADD.D
// AMOXOR.D
// AMOAND.D
// AMOOR.D
// AMOMIN.D
// AMOMAX.D
// AMOMINU.D
// AMOMAXU.D

// FLW
// FSW
// FMADD.S
// FMSUB.S
// FNMSUB.S
// FNMADD.S
// FADD.S
// FSUB.S
// FMUL.S
// FDIV.S
// FSQRT.S
// FSGNJ.S
// FSGNJN.S
// FSGNJX.S
// FMIN.S
// FMAX.S
// FCVT.W.S
// FCVT.WU.S
// FMV.X.W
// FEQ.S
// FLT.S
// FLE.S
// FCLASS.S
// FCVT.S.W
// FCVT.S.WU
// FMV.W.X
// FCVT.L.S 
// FCVT.LU.S 
// FCVT.S.L 
// FCVT.S.LU
// FLD
// FSD 
// FMADD.D 
// FMSUB.D 
// FNMSUB.D 
// FNMADD.D 
// FADD.D 
// FSUB.D 
// FMUL.D 
// FDIV.D 
// FSQRT.D 
// FSGNJ.D 
// FSGNJN.D 
// FSGNJX.D 
// FMIN.D 
// FMAX.D 
// FCVT.S.D 
// FCVT.D.S 
// FEQ.D 
// FLT.D 
// FLE.D 
// FCLASS.D 
// FCVT.W.D 
// FCVT.WU.D 
// FCVT.D.W 
// FCVT.D.WU
// FCVT.L.D 
// FCVT.LU.D 
// FMV.X.D 
// FCVT.D.L 
// FCVT.D.LU 
// FMV.D.X
// FLQ
// FSQ 
// FMADD.Q 
// FMSUB.Q 
// FNMSUB.Q 
// FNMADD.Q 
// FADD.Q 
// FSUB.Q 
// FMUL.Q 
// FDIV.Q 
// FSQRT.Q 
// FSGNJ.Q 
// FSGNJN.Q 
// FSGNJX.Q 
// FMIN.Q 
// FMAX.Q 
// FCVT.S.Q 
// FCVT.Q.S 
// FCVT.D.Q 
// FCVT.Q.D 
// FEQ.Q 
// FLT.Q 
// FLE.Q 
// FCLASS.Q 
// FCVT.W.Q 
// FCVT.WU.Q 
// FCVT.Q.W 
// FCVT.Q.WU
// FCVT.L.Q 
// FCVT.LU.Q 
// FCVT.Q.L 
// FCVT.Q.LU

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
