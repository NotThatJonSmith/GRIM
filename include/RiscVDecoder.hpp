#pragma once

/*
 * This file contains a "naive" implementation of a RISC-V ISA decoder. It's an
 * exhaustive tabular walkthrough of the whole encoding space of RISC-V. The
 * trick to making everything go fast is that this is all constexpr, so the
 * compiler can precompute tables that feed into faster decoder strategies.
 */

#include <RiscV.hpp>
#include <Instructions.hpp>

#define QUADRANT ExtendBits::Zero, 1, 0
#define OPCODE   ExtendBits::Zero, 6, 2
#define OP_MINOR ExtendBits::Zero, 31, 25, 14, 12
#define FUNCT3   ExtendBits::Zero, 14, 12
#define FUNCT7   ExtendBits::Zero, 31, 25
#define SHAMT    ExtendBits::Zero, 24, 20
#define FUNCT5   ExtendBits::Zero, 31, 27
#define C_FUNCT2 ExtendBits::Zero, 6, 5
#define C_FUNCT3 ExtendBits::Zero, 15, 13
#define C_FUNCT4 ExtendBits::Zero, 15, 12
#define C_FUNCT6 ExtendBits::Zero, 15, 10

template<typename XLEN_t>
constexpr Instruction<XLEN_t> decode_instruction(__uint32_t inst, __uint32_t extensionsVector, RISCV::XlenMode mxlen) {
    switch (swizzle<__uint32_t, QUADRANT>(inst)) {
    case RISCV::OpcodeQuadrant::UNCOMPRESSED:
        switch (swizzle<__uint32_t, OPCODE>(inst)) {
        case RISCV::MajorOpcode::LOAD:
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::MinorOpcode::LB: return inst_lb<XLEN_t>;
            case RISCV::MinorOpcode::LH: return inst_lh<XLEN_t>;
            case RISCV::MinorOpcode::LW: return inst_lw<XLEN_t>;
            case RISCV::MinorOpcode::LD: return inst_ld<XLEN_t>;
            case RISCV::MinorOpcode::LBU: return inst_lbu<XLEN_t>;
            case RISCV::MinorOpcode::LHU: return inst_lhu<XLEN_t>;
            case RISCV::MinorOpcode::LWU: return inst_lwu<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::LOAD_FP: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::CUSTOM_0: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::MISC_MEM: 
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::MinorOpcode::FENCE: return inst_fence<XLEN_t>;
            case RISCV::MinorOpcode::FENCE_I: return inst_fencei<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::OP_IMM:
            // TODO: strictly speaking, SLLI is only valid if FUNCT7 is all zeroes. There are a lot of little non-strict d/c encodings throughout the decoder.
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::MinorOpcode::ADDI: return inst_addi<XLEN_t>;
            case RISCV::MinorOpcode::SLLI:
                if (mxlen == RISCV::XlenMode::XL32) {
                    if (inst & 0xbe000000) return inst_illegal<XLEN_t>;
                } else if (inst & 0xbc000000) return inst_illegal<XLEN_t>;
                return inst_slli<XLEN_t>;
            case RISCV::MinorOpcode::SLTI: return inst_slti<XLEN_t>;
            case RISCV::MinorOpcode::SLTIU: return inst_sltiu<XLEN_t>;
            case RISCV::MinorOpcode::XORI: return inst_xori<XLEN_t>;
            case RISCV::MinorOpcode::SRI:
                if (mxlen == RISCV::XlenMode::XL32) {
                    if (inst & 0xbe000000) return inst_illegal<XLEN_t>;
                } else if (inst & 0xbc000000) return inst_illegal<XLEN_t>;
                if (inst & (0x40000000))
                    return inst_srai<XLEN_t>;
                return inst_srli<XLEN_t>;
            case RISCV::MinorOpcode::ORI: return inst_ori<XLEN_t>;
            case RISCV::MinorOpcode::ANDI: return inst_andi<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::AUIPC: return inst_auipc<XLEN_t>;
        case RISCV::MajorOpcode::OP_IMM_32:
            if (mxlen == RISCV::XlenMode::XL32)
                return inst_illegal<XLEN_t>; // Reserved encoding
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::MinorOpcode::ADDIW: return inst_addiw<XLEN_t>;
            case RISCV::MinorOpcode::SLLIW: return inst_slliw<XLEN_t>;
            case RISCV::MinorOpcode::SRI:
                if (mxlen == RISCV::XlenMode::XL32) {
                    if (inst & 0xbe000000) return inst_illegal<XLEN_t>;
                } else if (inst & 0xbc000000) return inst_illegal<XLEN_t>;
                if (inst & (0x40000000))
                    return inst_sraiw<XLEN_t>;
                return inst_srliw<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
        }
        case RISCV::MajorOpcode::LONG_48B_1: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::STORE:
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::MinorOpcode::SB: return inst_sb<XLEN_t>;
            case RISCV::MinorOpcode::SH: return inst_sh<XLEN_t>;
            case RISCV::MinorOpcode::SW: return inst_sw<XLEN_t>;
            case RISCV::MinorOpcode::SD: return inst_sd<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::STORE_FP: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::CUSTOM_1: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::AMO:
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::AmoWidth::AMO_W:
                switch (swizzle<__uint32_t, FUNCT5>(inst)) {
                case RISCV::MinorOpcode::AMOADD: return inst_amoaddw<XLEN_t>;
                case RISCV::MinorOpcode::AMOSWAP: return inst_amoswapw<XLEN_t>;
                case RISCV::MinorOpcode::LR: return inst_lrw<XLEN_t>;
                case RISCV::MinorOpcode::SC: return inst_scw<XLEN_t>;
                case RISCV::MinorOpcode::AMOXOR: return inst_amoxorw<XLEN_t>;
                case RISCV::MinorOpcode::AMOOR: return inst_amoorw<XLEN_t>;
                case RISCV::MinorOpcode::AMOAND: return inst_amoandw<XLEN_t>;
                case RISCV::MinorOpcode::AMOMIN: return inst_amominw<XLEN_t>;
                case RISCV::MinorOpcode::AMOMAX: return inst_amomaxw<XLEN_t>;
                case RISCV::MinorOpcode::AMOMINU: return inst_amominuw<XLEN_t>;
                case RISCV::MinorOpcode::AMOMAXU: return inst_amomaxuw<XLEN_t>;
                default: return inst_illegal<XLEN_t>;
                }
            case RISCV::AmoWidth::AMO_D:
                switch (swizzle<__uint32_t, FUNCT5>(inst)) {
                case RISCV::MinorOpcode::AMOADD: return inst_amoaddd<XLEN_t>;
                case RISCV::MinorOpcode::AMOSWAP: return inst_amoswapd<XLEN_t>;
                case RISCV::MinorOpcode::LR: return inst_lrd<XLEN_t>;
                case RISCV::MinorOpcode::SC: return inst_scd<XLEN_t>;
                case RISCV::MinorOpcode::AMOXOR: return inst_amoxord<XLEN_t>;
                case RISCV::MinorOpcode::AMOOR: return inst_amoord<XLEN_t>;
                case RISCV::MinorOpcode::AMOAND: return inst_amoandd<XLEN_t>;
                case RISCV::MinorOpcode::AMOMIN: return inst_amomind<XLEN_t>;
                case RISCV::MinorOpcode::AMOMAX: return inst_amomaxd<XLEN_t>;
                case RISCV::MinorOpcode::AMOMINU: return inst_amominud<XLEN_t>;
                case RISCV::MinorOpcode::AMOMAXU: return inst_amomaxud<XLEN_t>;
                default: return inst_illegal<XLEN_t>;
                }
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::OP:
            switch (swizzle<__uint32_t, OP_MINOR>(inst)) {
            case RISCV::MinorOpcode::ADD: return inst_add<XLEN_t>;
            case RISCV::MinorOpcode::SUB: return inst_sub<XLEN_t>;
            case RISCV::MinorOpcode::SLL: return inst_sll<XLEN_t>;
            case RISCV::MinorOpcode::SLT: return inst_slt<XLEN_t>;
            case RISCV::MinorOpcode::SLTU: return inst_sltu<XLEN_t>;
            case RISCV::MinorOpcode::XOR: return inst_xor<XLEN_t>;
            case RISCV::MinorOpcode::SRA: return inst_sra<XLEN_t>;
            case RISCV::MinorOpcode::SRL: return inst_srl<XLEN_t>;
            case RISCV::MinorOpcode::OR: return inst_or<XLEN_t>;
            case RISCV::MinorOpcode::AND: return inst_and<XLEN_t>;
            case RISCV::MinorOpcode::MUL: return inst_mul<XLEN_t>;
            case RISCV::MinorOpcode::MULH: return inst_mulh<XLEN_t>;
            case RISCV::MinorOpcode::MULHSU: return inst_mulhsu<XLEN_t>;
            case RISCV::MinorOpcode::MULHU: return inst_mulhu<XLEN_t>;
            case RISCV::MinorOpcode::DIV: return inst_div<XLEN_t>;
            case RISCV::MinorOpcode::DIVU: return inst_divu<XLEN_t>;
            case RISCV::MinorOpcode::REM: return inst_rem<XLEN_t>;
            case RISCV::MinorOpcode::REMU: return inst_remu<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::LUI: return inst_lui<XLEN_t>;
        case RISCV::MajorOpcode::OP_32:
            if (mxlen == RISCV::XlenMode::XL32)
                return inst_illegal<XLEN_t>;
            switch (swizzle<__uint32_t, OP_MINOR>(inst)) {
            case RISCV::MinorOpcode::ADDW: return inst_addw<XLEN_t>;
            case RISCV::MinorOpcode::SUBW: return inst_subw<XLEN_t>;
            case RISCV::MinorOpcode::SLLW: return inst_sllw<XLEN_t>;
            case RISCV::MinorOpcode::SRLW: return inst_srlw<XLEN_t>;
            case RISCV::MinorOpcode::SRAW: return inst_sraw<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::LONG_64B: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::MADD: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::MSUB: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::NMSUB: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::NMADD: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::OP_FP: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::RESERVED_0: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::CUSTOM_2: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::LONG_48B_2: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::BRANCH:
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::MinorOpcode::BEQ: return inst_beq<XLEN_t>;
            case RISCV::MinorOpcode::BNE: return inst_bne<XLEN_t>;
            case RISCV::MinorOpcode::BLT: return inst_blt<XLEN_t>;
            case RISCV::MinorOpcode::BGE: return inst_bge<XLEN_t>;
            case RISCV::MinorOpcode::BLTU: return inst_bltu<XLEN_t>;
            case RISCV::MinorOpcode::BGEU: return inst_bgeu<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::JALR: return inst_jalr<XLEN_t>;
        case RISCV::MajorOpcode::RESERVED_1: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::JAL: return inst_jal<XLEN_t>;
        case RISCV::MajorOpcode::SYSTEM:
            switch (swizzle<__uint32_t, FUNCT3>(inst)) {
            case RISCV::MinorOpcode::PRIV:
                switch (swizzle<__uint32_t, FUNCT7>(inst)) {
                case RISCV::SubMinorOpcode::ECALL_EBREAK_URET:
                    switch (swizzle<__uint32_t, RS2>(inst)) {
                    case RISCV::SubSubMinorOpcode::ECALL: return inst_ecall<XLEN_t>;
                    case RISCV::SubSubMinorOpcode::EBREAK: return inst_ebreak<XLEN_t>;
                    case RISCV::SubSubMinorOpcode::URET: return inst_uret<XLEN_t>;
                    default: return inst_illegal<XLEN_t>;
                    }
                case RISCV::SubMinorOpcode::SRET_WFI:
                    switch (swizzle<__uint32_t, RS2>(inst)) {
                    case RISCV::SubSubMinorOpcode::WFI: return inst_wfi<XLEN_t>;
                    case RISCV::SubSubMinorOpcode::SRET: return inst_sret<XLEN_t>;
                    default: return inst_illegal<XLEN_t>;
                    }
                case RISCV::SubMinorOpcode::MRET: return inst_mret<XLEN_t>;
                case RISCV::SFENCE_VMA: return inst_sfencevma<XLEN_t>;
                default: return inst_illegal<XLEN_t>;
                }
            case RISCV::MinorOpcode::CSRRW: return inst_csrrw<XLEN_t>;
            case RISCV::MinorOpcode::CSRRS: return inst_csrrs<XLEN_t>;
            case RISCV::MinorOpcode::CSRRC: return inst_csrrc<XLEN_t>;
            case RISCV::MinorOpcode::CSRRWI: return inst_csrrwi<XLEN_t>;
            case RISCV::MinorOpcode::CSRRSI: return inst_csrrsi<XLEN_t>;
            case RISCV::MinorOpcode::CSRRCI: return inst_csrrci<XLEN_t>;
            default: return inst_illegal<XLEN_t>;
            }
        case RISCV::MajorOpcode::RESERVED_2: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::CUSTOM_3: return inst_unimplemented<XLEN_t>;
        case RISCV::MajorOpcode::LONG_80B: return inst_unimplemented<XLEN_t>;
        default: return inst_illegal<XLEN_t>;
        }
    case RISCV::OpcodeQuadrant::Q0:
        switch(swizzle<__uint32_t, C_FUNCT3>(inst)) {
        case 0:
            if (swizzle<__uint32_t, ExtendBits::Zero, 10, 7, 12, 11, 5, 5, 6, 6, 2>(inst) == 0)
                return inst_illegal<XLEN_t>;
            return inst_caddi4spn<XLEN_t>;
        case 1:
            if (mxlen == RISCV::XlenMode::XL32 || mxlen == RISCV::XlenMode::XL64)
                return inst_unimplemented<XLEN_t>; // C.FLD TODO
            return inst_clq<XLEN_t>;
        case 2: return inst_clw<XLEN_t>;
        case 3:
            if (mxlen == RISCV::XlenMode::XL32)
                return inst_unimplemented<XLEN_t>; // C.FLW TODO
            return inst_cld<XLEN_t>;
        case 4: return inst_illegal<XLEN_t>; // Reserved encoding
        case 5:
            if (mxlen == RISCV::XlenMode::XL32 || mxlen == RISCV::XlenMode::XL64)
                return inst_unimplemented<XLEN_t>; // C.FSD TODO
            return inst_csq<XLEN_t>;
        case 6: return inst_csw<XLEN_t>;
        case 7:
            if (mxlen == RISCV::XlenMode::XL32)
                return inst_unimplemented<XLEN_t>; // C.FSW TODO
            return inst_csd<XLEN_t>;
        default: return inst_illegal<XLEN_t>;
        }
    case RISCV::OpcodeQuadrant::Q1:
        switch(swizzle<__uint32_t, C_FUNCT3>(inst)) {
        case 0: return inst_caddi<XLEN_t>; // Note that NOP is the same instruction
        case 1:
            if (mxlen == RISCV::XlenMode::XL32)
                return inst_cjal<XLEN_t>;
            return inst_caddiw<XLEN_t>;
        case 2: return inst_cli<XLEN_t>;
        case 3:
            if (swizzle<__uint32_t, CI_RD_RS1>(inst) != 2)
                return inst_clui<XLEN_t>;
            if (swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 4, 3, 5, 5, 2, 2, 6, 6, 4>(inst) != 0)
                return inst_caddi16sp<XLEN_t>;
            return inst_illegal<XLEN_t>; // Reserved encoding
        case 4:
            switch (swizzle<__uint32_t, ExtendBits::Zero, 11, 10>(inst)) {
            case 0:
                if (mxlen == RISCV::XlenMode::XL32)
                    if (swizzle<__uint32_t, CB_SHAMT>(inst) & 1 << 5)
                        return inst_illegal<XLEN_t>; // Reserved encoding
                return inst_csrli<XLEN_t>;
            case 1:
                if (mxlen == RISCV::XlenMode::XL32)
                    if (swizzle<__uint32_t, CB_SHAMT>(inst) & 1 << 5)
                        return inst_illegal<XLEN_t>; // Reserved encoding
                return inst_csrai<XLEN_t>;
            case 2: return inst_candi<XLEN_t>;
            case 3:
                switch(swizzle<__uint32_t, ExtendBits::Zero, 12, 12, 6, 5>(inst)) {
                case 0: return inst_csub<XLEN_t>;
                case 1: return inst_cxor<XLEN_t>;
                case 2: return inst_cor<XLEN_t>;
                case 3: return inst_cand<XLEN_t>;
                case 4: return inst_csubw<XLEN_t>;
                case 5: return inst_caddw<XLEN_t>;  
                case 6: return inst_illegal<XLEN_t>; // Reserved encoding
                case 7: return inst_illegal<XLEN_t>; // Reserved encoding
                default: return inst_illegal<XLEN_t>;
                }
            default: return inst_illegal<XLEN_t>;
            }
        case 5: return inst_cj<XLEN_t>;
        case 6: return inst_cbeqz<XLEN_t>;
        case 7: return inst_cbnez<XLEN_t>;
        default: return inst_illegal<XLEN_t>;
        }
    case RISCV::OpcodeQuadrant::Q2:
        switch(swizzle<__uint32_t, C_FUNCT3>(inst)) {
        case 0:
            if (mxlen == RISCV::XlenMode::XL32)
                if (swizzle<__uint32_t, CI_SHAMT>(inst) & 1 << 5)
                    return inst_illegal<XLEN_t>; // Reserved encoding
            return inst_cslli<XLEN_t>;
        case 1:
            if (mxlen == RISCV::XlenMode::XL32 || mxlen == RISCV::XlenMode::XL64)
                return inst_unimplemented<XLEN_t>; // C.FLDSP TODO
            return inst_clqsp<XLEN_t>;
        case 2: return inst_clwsp<XLEN_t>;
        case 3:
            if (sizeof(XLEN_t) == 8)
                return inst_cldsp<XLEN_t>;
            else
                return inst_unimplemented<XLEN_t>; // C.FLWSP TODO
        case 4:
            if (inst & 1 << 12) {
                if (swizzle<__uint32_t, ExtendBits::Zero, 6, 2>(inst) == 0) {
                    if (swizzle<__uint32_t, ExtendBits::Zero, 11, 7>(inst) == 0)
                        return inst_cebreak<XLEN_t>;
                    return inst_cjalr<XLEN_t>;
                }
                return inst_cadd<XLEN_t>;
            } else {
                if (swizzle<__uint32_t, ExtendBits::Zero, 6, 2>(inst) == 0) {
                    if (swizzle<__uint32_t, ExtendBits::Zero, 11, 7>(inst) == 0) 
                        return inst_illegal<XLEN_t>; // Reserved encoding
                    return inst_cjr<XLEN_t>;
                }
                return inst_cmv<XLEN_t>;
            }
        case 5: return inst_unimplemented<XLEN_t>; // C.FSDSP C.SQSP TODO
        case 6: return inst_cswsp<XLEN_t>;
        case 7:
            if (mxlen == RISCV::XlenMode::XL32)
                return inst_unimplemented<XLEN_t>; // C.FSWSP TODO
            return inst_csdsp<XLEN_t>;
        default: return inst_illegal<XLEN_t>;
        }
    default: return inst_illegal<XLEN_t>;
    }
}
