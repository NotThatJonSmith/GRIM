#pragma once

#include <Swizzle.hpp>

#include <RiscV.hpp>

#include <HartState.hpp>

template<typename XLEN_t>
class OptimizedHart;

template<typename XLEN_t>
using DecodedInstruction = void (*)(__uint32_t encoding, OptimizedHart<XLEN_t> *hart);

template<typename XLEN_t>
using DisassemblyFunction = void (*)(__uint32_t encoding, std::ostream* out);

template<typename XLEN_t>
struct Instruction {
    DecodedInstruction<XLEN_t> executionFunction;
    DisassemblyFunction<XLEN_t> disassemblyFunction;
};


#include <type_traits>

#define RD           ExtendBits::Zero, 11, 7
#define RS1          ExtendBits::Zero, 19, 15
#define RS2          ExtendBits::Zero, 24, 20
#define I_IMM        31, 20
#define S_IMM        ExtendBits::Sign, 31, 25, 11, 7
#define B_IMM        ExtendBits::Sign, 31, 31, 7, 7, 30, 25, 11, 8, 1
#define U_IMM        ExtendBits::Zero, 31, 12, 12
#define J_IMM        ExtendBits::Sign, 31, 31, 19, 12, 20, 20, 30, 21, 1

#define CR_RD_RS1    ExtendBits::Zero, 11, 7
#define CR_RS2       ExtendBits::Zero, 6, 2
#define CS_RD_RS1    ExtendBits::Zero, 11, 7
#define CS_RS1X      ExtendBits::Zero, 9, 7
#define CS_RS2X      ExtendBits::Zero, 4, 2
#define CSS_RS2      ExtendBits::Zero, 6, 2
#define CI_RD_RS1    ExtendBits::Zero, 11, 7
#define CIW_RDX      ExtendBits::Zero, 4, 2
#define CL_RDX       ExtendBits::Zero, 4, 2
#define CL_RS1X      ExtendBits::Zero, 9, 7
#define CA_RS2X      ExtendBits::Zero, 4, 2
#define CA_RDX_RS1X  ExtendBits::Zero, 9, 7
#define CB_RDX_RS1X  ExtendBits::Zero, 9, 7
#define CIW_RDX_RS1X ExtendBits::Zero, 11, 7
#define CB_SHAMT     ExtendBits::Zero, 12, 12, 6, 2
#define CI_SHAMT     ExtendBits::Zero, 12, 12, 6, 2

template<typename XLEN_t>
inline void ex_unimplemented(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    exit(1);
}

// Extra functional types not included in std::functional for some reason
template< class T = void >
struct left_shift { constexpr T operator()(const T& lhs, const T& rhs) const { return lhs << rhs; } };
template< class T = void >
struct right_shift { constexpr T operator()(const T& lhs, const T& rhs) const { return lhs >> rhs; } };
template< class T = void >
struct lhs { constexpr T operator()(const T& lhs, const T& rhs) const { return lhs; } };
template< class T = void >
struct min { constexpr T operator()(const T& lhs, const T& rhs) const { return (lhs < rhs) ? lhs : rhs; } };
template< class T = void >
struct max { constexpr T operator()(const T& lhs, const T& rhs) const { return (lhs > rhs) ? lhs : rhs; } };

// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    char value[N];
};

template<typename XLEN_t>
inline void ex_illegal(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
}

template<StringLiteral mnemonic>
inline void print_r_type_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    *out << mnemonic.value << " " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << RISCV::regName(rs2) << std::endl;
}

template<StringLiteral mnemonic, bool is_shift>
inline void print_i_type_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, I_IMM>(encoding);
    if constexpr (is_shift) imm &= 0b111111;
    *out << mnemonic.value << " " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<StringLiteral mnemonic, unsigned int down_shift_imm>
inline void print_u_type_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t imm = swizzle<__uint32_t, U_IMM>(encoding);
    *out << mnemonic.value << " " << RISCV::regName(rd) << ", " << (imm >> down_shift_imm) << std::endl;
}

template<StringLiteral mnemonic>
inline void print_b_type_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, B_IMM>(encoding);
    *out << mnemonic.value << " " << RISCV::regName(rs1) << ", " << RISCV::regName(rs2) << ", " << imm << std::endl;
}

template<typename XLEN_t, typename MEM_TYPE_t, bool load_unsigned>
inline void print_load_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, I_IMM>(encoding);
    const char *size_letters = "?bh?w???d???????q";
    *out << "l" << size_letters[sizeof(MEM_TYPE_t)] << (load_unsigned ? "u " : " ") << RISCV::regName(rd) << ",(" << imm << ")" << RISCV::regName(rs1) << std::endl;
}

template<typename XLEN_t, typename MEM_TYPE_t>
inline void print_store_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, S_IMM>(encoding);
    const char *size_letters = "?bh?w???d???????q";
    *out << "s" << size_letters[sizeof(MEM_TYPE_t)] << " " << RISCV::regName(rs2) << ",(" << imm << ")" << RISCV::regName(rs1) << std::endl;
}

template<StringLiteral mnemonic, bool immediate>
inline void print_csr_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t imm = (int32_t)swizzle<__uint32_t, ExtendBits::Zero, I_IMM>(encoding);
    if constexpr (immediate) {
        *out << mnemonic.value << " " << RISCV::regName(rd) << ", " << RISCV::csrName(imm) << ", " << rs1 << std::endl;
    } else {
        *out << mnemonic.value << " " << RISCV::regName(rd) << ", " << RISCV::csrName(imm) << ", " << RISCV::regName(rs1) << std::endl;
    }
}

template<StringLiteral mnemonic>
inline void print_just_mnemonic(__uint32_t encoding, std::ostream* out) {
    *out << mnemonic.value << std::endl;
}

template<typename XLEN_t>
inline void print_jal(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __int32_t imm = swizzle<__uint32_t, J_IMM>(encoding);
    *out << "jal " << RISCV::regName(rd) << ", " << imm << std::endl;
}

template<typename XLEN_t, typename OperandType, typename Operation, bool rhs_immediate>
inline void ex_op_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if constexpr (sizeof(OperandType) > sizeof(XLEN_t)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, I_IMM>(encoding);
    OperandType lhs = (~(OperandType)0) & hart->state.regs[rs1];
    OperandType rhs = (~(OperandType)0) & (rhs_immediate ? imm : hart->state.regs[rs2]);
    Operation operation;
    XLEN_t rd_value = operation(lhs, rhs);
    if constexpr (sizeof(XLEN_t) > sizeof(OperandType)) {
        bool result_sign_bit = rd_value >> ((sizeof(OperandType)*8)-1);
        rd_value |= result_sign_bit ? ((XLEN_t)~0 << sizeof(OperandType)*8) : 0;
    }
    hart->state.regs[rd] = rd_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 4;
}

template<typename XLEN_t, typename ComparisonOp>
inline void ex_branch_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, B_IMM>(encoding);
    ComparisonOp compare;
    hart->state.pc += compare(hart->state.regs[rs1], hart->state.regs[rs2]) ? imm : 4;
}

template<typename XLEN_t, bool add_pc>
inline void ex_upper_immediate_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t imm = swizzle<__uint32_t, U_IMM>(encoding);
    hart->state.regs[rd] = (add_pc ? hart->state.pc : 0) + imm;
    hart->state.regs[0] = 0;
    hart->state.pc += 4;
}

template<typename XLEN_t>
inline void ex_jal(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __int32_t imm = swizzle<__uint32_t, J_IMM>(encoding);
    hart->state.regs[rd] = hart->state.pc + 4;
    hart->state.regs[0] = 0;
    hart->state.pc = hart->state.pc + imm;
}

template<typename XLEN_t>
inline void ex_jalr(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    typedef std::make_signed_t<XLEN_t> SXLEN_t;
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __int32_t imm = (__int32_t)swizzle<__uint32_t, ExtendBits::Sign, I_IMM>(encoding);
    SXLEN_t imm_value = imm;
    imm_value &= ~(XLEN_t)1;
    hart->state.regs[rd] = hart->state.pc + 4;
    hart->state.regs[0] = 0;
    hart->state.pc = hart->state.regs[rs1] + imm_value;
}

// TODO endianness-agnostic impl; for now host and RV being both LE save us
template<typename XLEN_t, typename MEM_TYPE_t, bool ignore_immediate>
inline void ex_load_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if constexpr (sizeof(XLEN_t) < sizeof(MEM_TYPE_t)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __int32_t imm = 0;
    if constexpr (!ignore_immediate)
        imm = swizzle<__uint32_t, ExtendBits::Sign, I_IMM>(encoding);
    MEM_TYPE_t read_value;
    XLEN_t read_address = hart->state.regs[rs1] + imm;
    XLEN_t transferredSize = hart->template Transact<MEM_TYPE_t, AccessType::R>(read_address, (char*)&read_value);
    if (transferredSize != sizeof(MEM_TYPE_t))
        return;
    hart->state.regs[rd] = read_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 4;
}

template<typename XLEN_t, typename MEM_TYPE_t>
inline void ex_store_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if constexpr (sizeof(XLEN_t) < sizeof(MEM_TYPE_t)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, S_IMM>(encoding);
    XLEN_t write_addr = hart->state.regs[rs1] + imm;
    MEM_TYPE_t write_value = hart->state.regs[rs2] & (MEM_TYPE_t)~0;
    XLEN_t transferredSize = hart->template Transact<MEM_TYPE_t, AccessType::W>(write_addr, (char*)&write_value);
    if (transferredSize != sizeof(MEM_TYPE_t))
        return;
    hart->state.pc += 4;
}

template<typename XLEN_t>
inline void ex_scw(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    XLEN_t tmp = hart->state.regs[rs2];
    XLEN_t write_address = hart->state.regs[rs1];
    XLEN_t write_size = 4;
    XLEN_t transferredSize = hart->template Transact<__uint32_t, AccessType::W>(write_address, (char*)&tmp);
    if (transferredSize != sizeof(write_size))
        return;
    hart->state.regs[rd] = 0; // NOTE, zero means sc succeeded - for now it always does!
    hart->state.pc += 4;
}

template<typename XLEN_t, typename MEM_TYPE_t, typename Operation>
inline void ex_amo_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if constexpr (sizeof(XLEN_t) < sizeof(MEM_TYPE_t)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, RS2>(encoding);
    MEM_TYPE_t mem_value = 0;
    XLEN_t mem_address = hart->state.regs[rs1];
    XLEN_t transferredSize = hart->template Transact<MEM_TYPE_t, AccessType::R>(mem_address, (char*)&mem_value);
    if (transferredSize != sizeof(MEM_TYPE_t))
        return;
    hart->state.regs[rd] = mem_value;
    hart->state.regs[0] = 0;
    Operation operation;
    mem_value = operation(mem_value, hart->state.regs[rs2]);
    hart->template Transact<MEM_TYPE_t, AccessType::W>(mem_address, (char*)&mem_value);
    hart->state.pc += 4;
}

template<typename XLEN_t>
inline void ex_fence(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    hart->state.pc += 4; // NOP for now.
}

template<typename XLEN_t>
inline void ex_fencei(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    hart->state.implCallback(HartCallbackArgument::RequestedIfence);
    hart->state.pc += 4;
}

template<typename XLEN_t>
inline void ex_ecall(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    RISCV::TrapCause cause =
        hart->state.privilegeMode == RISCV::PrivilegeMode::Machine ? RISCV::TrapCause::ECALL_FROM_M_MODE :
        hart->state.privilegeMode == RISCV::PrivilegeMode::Supervisor ? RISCV::TrapCause::ECALL_FROM_S_MODE :
        RISCV::TrapCause::ECALL_FROM_U_MODE;
    hart->state.RaiseException(cause, encoding);
}

template<typename XLEN_t>
inline void ex_ebreak(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    hart->state.RaiseException(RISCV::TrapCause::BREAKPOINT, encoding);
}

template<typename XLEN_t, bool sets_bits, bool clears_bits, bool rs1_is_immediate>
inline void ex_csr_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    RISCV::CSRAddress csr = (RISCV::CSRAddress)swizzle<__uint32_t, ExtendBits::Zero, I_IMM>(encoding);
    if (hart->state.privilegeMode < RISCV::csrRequiredPrivilege(csr)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rd = swizzle<__uint32_t, RD>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, RS1>(encoding);
    XLEN_t regVal = rs1_is_immediate ? rs1 : hart->state.regs[rs1];
    XLEN_t csrValue = 0;
    bool read_required = sets_bits || clears_bits || rd;
    bool write_required = !(sets_bits || clears_bits) || rs1;
    if (read_required) {
        XLEN_t csrValue = hart->state.ReadCSR(csr);
        hart->state.regs[rd] = csrValue;
        hart->state.regs[0] = 0;
    }
    if constexpr (sets_bits) csrValue |= regVal;
    if constexpr (clears_bits) csrValue = ~csrValue & regVal;
    if (write_required) {
        if (RISCV::csrIsReadOnly(csr)) {
            hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
            return;
        }
        hart->state.WriteCSR(csr, regVal);
    }
    hart->state.pc += 4;
}

template<typename XLEN_t>
inline void ex_wfi(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    hart->state.pc += 4; // NOP for now. TODO something smarter with the hart's interrupt pins
}

// TODO URET is only provided if user-mode traps are supported, and should raise an illegal encodingruction otherwise.
// TODO SRET must be provided if supervisor mode is supported, and should raise an
// illegal encodingruction exception otherwise. SRET should also raise an illegal encodingruction exception when TSR=1
// in mstatus, as described in Section 3.1.6.4.
template<typename XLEN_t, RISCV::PrivilegeMode from_mode>
inline void ex_trap_return(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if (hart->state.privilegeMode < from_mode) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    hart->state.template ReturnFromTrap<from_mode>();
}

template<typename XLEN_t>
inline void ex_sfencevma(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    hart->state.implCallback(HartCallbackArgument::RequestedVMfence);
    hart->state.pc += 4;
}

template<typename XLEN_t>
inline void ex_caddi4spn(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CIW_RDX>(encoding)+8;
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Zero, 10, 7, 12, 11, 5, 5, 6, 6, 2>(encoding);
    hart->state.regs[rd] = hart->state.regs[2] + imm;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_caddi4spn(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CIW_RDX>(encoding)+8;
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Zero, 10, 7, 12, 11, 5, 5, 6, 6, 2>(encoding);
    *out << "(C.ADDI4SPN) addi " << RISCV::regName(rd) << ", " << RISCV::regName(2) << ", " << imm << std::endl;
}

// TODO maybe combine cl and cs ?
template<typename XLEN_t, typename MEM_TYPE_t>
inline void ex_cl_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if constexpr (sizeof(MEM_TYPE_t) > sizeof(XLEN_t)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rd = swizzle<__uint32_t, CL_RDX>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CL_RS1X>(encoding)+8;
    __int32_t imm;
    if constexpr (sizeof(MEM_TYPE_t) >= 16) imm = swizzle<__uint32_t, ExtendBits::Zero, 10, 10, 5, 6, 12, 11, 4>(encoding);
    else if constexpr (sizeof(MEM_TYPE_t) >= 8) imm = swizzle<__uint32_t, ExtendBits::Zero, 6, 5, 12, 10, 3>(encoding);
    else imm = swizzle<__uint32_t, ExtendBits::Zero, 5, 5, 12, 10, 6, 6, 2>(encoding);
    MEM_TYPE_t mem_value;
    XLEN_t read_address = hart->state.regs[rs1] + imm;
    XLEN_t transferredSize = hart->template Transact<MEM_TYPE_t, AccessType::R>(read_address, (char*)&mem_value);
    if (transferredSize != sizeof(MEM_TYPE_t))
        return;
    hart->state.regs[rd] = mem_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t, typename MEM_TYPE_t, StringLiteral mnemonic>
inline void print_cl_generic(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CL_RDX>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CL_RS1X>(encoding)+8;
    __int32_t imm;
    if constexpr (sizeof(MEM_TYPE_t) >= 16) imm = swizzle<__uint32_t, ExtendBits::Zero, 10, 10, 5, 6, 12, 11, 4>(encoding);
    else if constexpr (sizeof(MEM_TYPE_t) >= 8) imm = swizzle<__uint32_t, ExtendBits::Zero, 6, 5, 12, 10, 3>(encoding);
    else imm = swizzle<__uint32_t, ExtendBits::Zero, 5, 5, 12, 10, 6, 6, 2>(encoding);
    *out << mnemonic.value << " " << RISCV::regName(rd) << ",(" << imm << ")" << RISCV::regName(rs1) << std::endl;
}

template<typename XLEN_t, typename MEM_TYPE_t>
inline void ex_cs_generic(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if constexpr (sizeof(MEM_TYPE_t) > sizeof(XLEN_t)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rs1 = swizzle<__uint32_t, CS_RS1X>(encoding)+8;
    __uint32_t rs2 = swizzle<__uint32_t, CS_RS2X>(encoding)+8;
    __int32_t imm;
    if constexpr (sizeof(MEM_TYPE_t) >= 16) imm = swizzle<__uint32_t, ExtendBits::Zero, 10, 10, 5, 6, 12, 11, 4>(encoding);
    else if constexpr (sizeof(MEM_TYPE_t) >= 8) imm = swizzle<__uint32_t, ExtendBits::Zero, 6, 5, 12, 10, 3>(encoding);
    else imm = swizzle<__uint32_t, ExtendBits::Zero, 5, 5, 12, 10, 6, 6, 2>(encoding);
    XLEN_t write_addr = hart->state.regs[rs1] + imm;
    XLEN_t transferredSize = hart->template Transact<MEM_TYPE_t, AccessType::W>(write_addr, (char*)&hart->state.regs[rs2]);
    if (transferredSize != sizeof(MEM_TYPE_t))
        return;
    hart->state.pc += 2;
}

template<typename XLEN_t, typename MEM_TYPE_t, StringLiteral mnemonic>
inline void print_cs_generic(__uint32_t encoding, std::ostream* out) {
    __uint32_t rs1 = swizzle<__uint32_t, CS_RS1X>(encoding)+8;
    __uint32_t rs2 = swizzle<__uint32_t, CS_RS2X>(encoding)+8;
    __int32_t imm;
    if constexpr (sizeof(MEM_TYPE_t) >= 16) imm = swizzle<__uint32_t, ExtendBits::Zero, 10, 10, 5, 6, 12, 11, 4>(encoding);
    else if constexpr (sizeof(MEM_TYPE_t) >= 8) imm = swizzle<__uint32_t, ExtendBits::Zero, 6, 5, 12, 10, 3>(encoding);
    else imm = swizzle<__uint32_t, ExtendBits::Zero, 5, 5, 12, 10, 6, 6, 2>(encoding);
    *out << mnemonic.value << " " << RISCV::regName(rs2) << ",(" << imm << ")" << RISCV::regName(rs1) << std::endl;
}

template<typename XLEN_t>
inline void ex_caddi(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __int32_t imm = (__int32_t)swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 2>(encoding);
    XLEN_t rs1_value = hart->state.regs[rs1];
    XLEN_t rd_value = rs1_value + imm;
    hart->state.regs[rd] = rd_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_caddi(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __int32_t imm = (__int32_t)swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 2>(encoding);
    *out << "(C.ADDI) addi " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_caddi16sp(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __int32_t imm = (__int32_t)swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 4, 3, 5, 5, 2, 2, 6, 6, 4>(encoding);
    hart->state.regs[2] += imm;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_caddi16sp(__uint32_t encoding, std::ostream* out) {
    __int32_t imm = (__int32_t)swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 4, 3, 5, 5, 2, 2, 6, 6, 4>(encoding);
    *out << "(C.ADDI16SP) addi " << RISCV::regName(2) << ", " << RISCV::regName(2) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_cjal(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 8, 8, 10, 9, 6, 6, 7, 7, 2, 2, 11, 11, 5, 3, 1>(encoding);
    hart->state.regs[1] = hart->state.pc + 2;
    hart->state.pc += imm;
}

template<typename XLEN_t>
inline void print_cjal(__uint32_t encoding, std::ostream* out) {
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 8, 8, 10, 9, 6, 6, 7, 7, 2, 2, 11, 11, 5, 3, 1>(encoding);
    *out << "(C.JAL) jal " << RISCV::regName(1) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_cli(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 2>(encoding);
    hart->state.regs[rd] = imm;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_cli(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 2>(encoding);
    *out << "(C.LI) addi " << RISCV::regName(rd) << ", " << RISCV::regName(0) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_clui(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __uint32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 2, 12>(encoding);
    hart->state.regs[rd] = imm;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_clui(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __uint32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 2, 12>(encoding);
    *out << "(C.LUI) lui " << RISCV::regName(rd) << ", " << (imm >> 12) << std::endl;
}

template<typename XLEN_t, typename Operation>
inline void ex_ca_format_op(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CA_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CA_RDX_RS1X>(encoding)+8;
    __uint32_t rs2 = swizzle<__uint32_t, CA_RS2X>(encoding)+8;
    Operation operation;
    hart->state.regs[rd] = operation(hart->state.regs[rs1], hart->state.regs[rs2]);
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<StringLiteral mnemonic>
inline void print_ca_format_instr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CA_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CA_RDX_RS1X>(encoding)+8;
    __uint32_t rs2 = swizzle<__uint32_t, CA_RS2X>(encoding)+8;
    *out << mnemonic.value << " " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << RISCV::regName(rs2) << std::endl;
}

template<typename XLEN_t>
inline void ex_cj(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 8, 8, 10, 9, 6, 6, 7, 7, 2, 2, 11, 11, 5, 3, 1>(encoding);
    __uint32_t rd = 0;
    hart->state.regs[rd] = hart->state.pc + 2;
    hart->state.regs[0] = 0;
    hart->state.pc += imm;
}

template<typename XLEN_t>
inline void print_cj(__uint32_t encoding, std::ostream* out) {
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 8, 8, 10, 9, 6, 6, 7, 7, 2, 2, 11, 11, 5, 3, 1>(encoding);
    __uint32_t rd = 0;
    *out << "(C.J) jal " << RISCV::regName(rd) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_cbeqz(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 5, 2, 2, 11, 10, 4, 3, 1>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    hart->state.pc += hart->state.regs[rs1] ? 2 : imm;
}

template<typename XLEN_t>
inline void print_cbeqz(__uint32_t encoding, std::ostream* out) {
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 5, 2, 2, 11, 10, 4, 3, 1>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    *out << "(C.BEQZ) beq " << RISCV::regName(rs1) << ", " << RISCV::regName(0) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_cbnez(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 5, 2, 2, 11, 10, 4, 3, 1>(encoding);
    hart->state.pc += hart->state.regs[rs1] ? imm : 2;
}

template<typename XLEN_t>
inline void print_cbnez(__uint32_t encoding, std::ostream* out) {
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Sign, 12, 12, 6, 5, 2, 2, 11, 10, 4, 3, 1>(encoding);
    *out << "(C.BNEZ) bne " << RISCV::regName(rs1) << ", " << RISCV::regName(0) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_candi(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    typedef std::make_signed_t<XLEN_t> SXLEN_t;
    __uint32_t rd = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __int32_t imm = swizzle<__int32_t, ExtendBits::Sign, 12, 12, 6, 2>(encoding);
    XLEN_t rs1_value = hart->state.regs[rs1];
    SXLEN_t imm_value_signed = imm;
    XLEN_t imm_value = *(XLEN_t*)&imm_value_signed;
    XLEN_t rd_value = rs1_value & imm_value;
    hart->state.regs[rd] = rd_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_candi(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __int32_t imm = swizzle<__int32_t, ExtendBits::Sign, 12, 12, 6, 2>(encoding);
    *out << "(C.ANDI) andi " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_clwsp(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    typedef std::make_signed_t<XLEN_t> SXLEN_t;
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __uint32_t rs1 = 2;
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Zero, 3, 2, 12, 12, 6, 4, 2>(encoding);
    __uint32_t word;
    XLEN_t rs1_value = hart->state.regs[rs1];
    SXLEN_t imm_value = imm;
    XLEN_t read_address = rs1_value + imm_value;
    XLEN_t read_size = 4;
    XLEN_t transferredSize = hart->template Transact<__uint32_t, AccessType::R>(read_address, (char*)&word);
    if (transferredSize != read_size)
        return;
    hart->state.regs[rd] = word;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_clwsp(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Zero, 3, 2, 12, 12, 6, 4, 2>(encoding);
    *out << "(C.LWSP) lw " << RISCV::regName(rd) << ",(" << imm << ")" << RISCV::regName(2) << std::endl;
}

template<typename XLEN_t, typename MEM_TYPE_t>
inline void ex_cs_sp(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    if constexpr (sizeof(XLEN_t) < sizeof(MEM_TYPE_t)) {
        hart->state.RaiseException(RISCV::TrapCause::ILLEGAL_INSTRUCTION, encoding);
        return;
    }
    __uint32_t rs2 = swizzle<__uint32_t, CSS_RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Zero, 8, 7, 12, 9, 2>(encoding);
    XLEN_t write_addr = hart->state.regs[2] + imm;
    MEM_TYPE_t write_value = hart->state.regs[rs2] & ~(MEM_TYPE_t)0;
    XLEN_t transferredSize = hart->template Transact<MEM_TYPE_t, AccessType::W>(write_addr, (char*)&write_value);
    if (transferredSize != sizeof(MEM_TYPE_t))
        return;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_cswsp(__uint32_t encoding, std::ostream* out) {
    __uint32_t rs1 = 2;
    __uint32_t rs2 = swizzle<__uint32_t, CSS_RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Zero, 8, 7, 12, 9, 2>(encoding);
    *out << "(C.SWSP) sw " << RISCV::regName(rs2) << ",(" << imm << ")" << RISCV::regName(rs1) << std::endl;
}

template<typename XLEN_t>
inline void print_csdsp(__uint32_t encoding, std::ostream* out) {
    __uint32_t rs1 = 2;
    __uint32_t rs2 = swizzle<__uint32_t, CSS_RS2>(encoding);
    __int32_t imm = swizzle<__uint32_t, ExtendBits::Zero, 8, 7, 12, 9, 2>(encoding);
    *out << "(C.SDSP) sw " << RISCV::regName(rs2) << ",(" << imm << ")" << RISCV::regName(rs1) << std::endl;
}

template<typename XLEN_t>
inline void ex_cjalr(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    typedef std::make_signed_t<XLEN_t> SXLEN_t;
    __int32_t imm = 0;
    __uint32_t rd = 1;
    __uint32_t rs1 = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    XLEN_t rs1_value = hart->state.regs[rs1];
    SXLEN_t imm_value = imm;
    imm_value &= ~(XLEN_t)1;
    hart->state.regs[rd] = hart->state.pc + 2;
    hart->state.regs[0] = 0;
    hart->state.pc = rs1_value + imm_value;
}

template<typename XLEN_t>
inline void print_cjalr(__uint32_t encoding, std::ostream* out) {
    __int32_t imm = 0;
    __uint32_t rd = 1;
    __uint32_t rs1 = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    *out << "(C.JALR) jalr " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_cjr(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    typedef std::make_signed_t<XLEN_t> SXLEN_t;
    __uint32_t rs1 = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __int32_t imm = 0;
    __uint32_t rd = 0;
    XLEN_t rs1_value = hart->state.regs[rs1];
    SXLEN_t imm_value = imm;
    imm_value &= ~(XLEN_t)1;
    hart->state.regs[rd] = hart->state.pc + 2;
    hart->state.regs[0] = 0;
    hart->state.pc = rs1_value + imm_value;
}

template<typename XLEN_t>
inline void print_cjr(__uint32_t encoding, std::ostream* out) {
    __uint32_t rs1 = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __int32_t imm = 0;
    __uint32_t rd = 0;
    *out << "(C.JR) jalr " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_cadd(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CR_RD_RS1>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, CR_RD_RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, CR_RS2>(encoding);
    XLEN_t rs1_value = hart->state.regs[rs1];
    XLEN_t rs2_value = hart->state.regs[rs2];
    XLEN_t rd_value = rs1_value + rs2_value;
    hart->state.regs[rd] = rd_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_cadd(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CR_RD_RS1>(encoding);
    __uint32_t rs1 = swizzle<__uint32_t, CR_RD_RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, CR_RS2>(encoding);
    *out << "(C.ADD) add " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << RISCV::regName(rs2) << std::endl;
}

template<typename XLEN_t>
inline void ex_cmv(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CR_RD_RS1>(encoding);
    __uint32_t rs2 = swizzle<__uint32_t, CR_RS2>(encoding);
    hart->state.regs[rd] = hart->state.regs[rs2];
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_cmv(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CR_RD_RS1>(encoding);
    __uint32_t rs1 = 0;
    __uint32_t rs2 = swizzle<__uint32_t, CR_RS2>(encoding);
    *out << "(C.MV) add " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << RISCV::regName(rs2) << std::endl;
}

template<typename XLEN_t>
inline void ex_cslli(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t imm = swizzle<__uint32_t, CI_SHAMT>(encoding);
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __uint32_t rs1 = rd;
    if constexpr (sizeof(XLEN_t) == 16) imm = imm == 0 ? 64 : imm;
    XLEN_t rs1_value = hart->state.regs[rs1];
    XLEN_t rd_value = rs1_value << imm;
    hart->state.regs[rd] = rd_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_cslli(__uint32_t encoding, std::ostream* out) {
    __uint32_t imm = swizzle<__uint32_t, CI_SHAMT>(encoding);
    __uint32_t rd = swizzle<__uint32_t, CI_RD_RS1>(encoding);
    __uint32_t rs1 = rd;
    *out << "(C.SLLI) slli " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_csrli(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t imm = swizzle<__uint32_t, CB_SHAMT>(encoding);
    if constexpr (sizeof(XLEN_t) == 16) imm = imm == 0 ? 64 : imm;
    XLEN_t rs1_value = hart->state.regs[rs1];
    XLEN_t rd_value = rs1_value >> imm;
    hart->state.regs[rd] = rd_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_csrli(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t imm = swizzle<__uint32_t, CB_SHAMT>(encoding);
    *out << "(C.SRLI) srli " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<typename XLEN_t>
inline void ex_csrai(__uint32_t encoding, OptimizedHart<XLEN_t> *hart) {
    __uint32_t rd = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t imm = swizzle<__uint32_t, CB_SHAMT>(encoding);
    if constexpr (sizeof(XLEN_t) == 16) imm = imm == 0 ? 64 : imm;
    XLEN_t rs1_value = hart->state.regs[rs1];
    __uint16_t imm_value = imm;
    XLEN_t rd_value = rs1_value >> imm_value;
    // Spec says: the original sign bit is copied into the vacated upper bits
    if (rs1_value & ((XLEN_t)1 << ((sizeof(XLEN_t)*8)-1)))
        rd_value |= (XLEN_t)((1 << imm_value)-1) << ((sizeof(XLEN_t)*8)-imm_value);
    hart->state.regs[rd] = rd_value;
    hart->state.regs[0] = 0;
    hart->state.pc += 2;
}

template<typename XLEN_t>
inline void print_csrai(__uint32_t encoding, std::ostream* out) {
    __uint32_t rd = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t rs1 = swizzle<__uint32_t, CB_RDX_RS1X>(encoding)+8;
    __uint32_t imm = swizzle<__uint32_t, CB_SHAMT>(encoding);
    *out << "(C.SRAI) srai " << RISCV::regName(rd) << ", " << RISCV::regName(rs1) << ", " << imm << std::endl;
}

template<typename XLEN_t> Instruction<XLEN_t> inst_illegal { ex_illegal<XLEN_t>, print_just_mnemonic<"illegal"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_unimplemented { ex_unimplemented<XLEN_t>, print_just_mnemonic<"unimplemented"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_add    { ex_op_generic<XLEN_t, XLEN_t,                     std::plus<XLEN_t>,       false>, print_r_type_instr<"add"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_addw   { ex_op_generic<XLEN_t, __uint32_t,                 std::plus<XLEN_t>,       false>, print_r_type_instr<"addw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_addi   { ex_op_generic<XLEN_t, XLEN_t,                     std::plus<XLEN_t>,       true>,  print_i_type_instr<"addi", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_addiw  { ex_op_generic<XLEN_t, __uint32_t,                 std::plus<XLEN_t>,       true>,  print_i_type_instr<"addiw", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sub    { ex_op_generic<XLEN_t, XLEN_t,                     std::minus<XLEN_t>,      false>, print_r_type_instr<"sub"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_subw   { ex_op_generic<XLEN_t, __uint32_t,                 std::minus<XLEN_t>,      false>, print_r_type_instr<"subw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sll    { ex_op_generic<XLEN_t, XLEN_t,                     left_shift<XLEN_t>,      false>, print_r_type_instr<"sll"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sllw   { ex_op_generic<XLEN_t, __uint32_t,                 left_shift<XLEN_t>,      false>, print_r_type_instr<"sllw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_slli   { ex_op_generic<XLEN_t, XLEN_t,                     left_shift<XLEN_t>,      true>,  print_i_type_instr<"slli", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_slliw  { ex_op_generic<XLEN_t, __uint32_t,                 left_shift<XLEN_t>,      true>,  print_i_type_instr<"slliw", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_srl    { ex_op_generic<XLEN_t, XLEN_t,                     right_shift<XLEN_t>,     false>, print_r_type_instr<"srl"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_srlw   { ex_op_generic<XLEN_t, __uint32_t,                 right_shift<XLEN_t>,     false>, print_r_type_instr<"srlw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_srli   { ex_op_generic<XLEN_t, XLEN_t,                     right_shift<XLEN_t>,     true>,  print_i_type_instr<"srli", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_srliw  { ex_op_generic<XLEN_t, __uint32_t,                 right_shift<XLEN_t>,     true>,  print_i_type_instr<"srliw", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sra    { ex_op_generic<XLEN_t, std::make_signed_t<XLEN_t>, right_shift<XLEN_t>,     false>, print_r_type_instr<"sra"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_srai   { ex_op_generic<XLEN_t, std::make_signed_t<XLEN_t>, right_shift<XLEN_t>,     true>,  print_i_type_instr<"srai", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sraw   { ex_op_generic<XLEN_t, __int32_t,                  right_shift<XLEN_t>,     false>, print_r_type_instr<"sraw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sraiw  { ex_op_generic<XLEN_t, __int32_t,                  right_shift<XLEN_t>,     true>,  print_i_type_instr<"sraiw", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_slt    { ex_op_generic<XLEN_t, std::make_signed_t<XLEN_t>, std::less<XLEN_t>,       false>, print_r_type_instr<"slt"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sltu   { ex_op_generic<XLEN_t, XLEN_t,                     std::less<XLEN_t>,       false>, print_r_type_instr<"sltu"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_slti   { ex_op_generic<XLEN_t, std::make_signed_t<XLEN_t>, std::less<XLEN_t>,       true>,  print_i_type_instr<"slti", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sltiu  { ex_op_generic<XLEN_t, XLEN_t,                     std::less<XLEN_t>,       true>,  print_i_type_instr<"sltiu", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_xor    { ex_op_generic<XLEN_t, XLEN_t,                     std::bit_xor<XLEN_t>,    false>, print_r_type_instr<"xor"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_xori   { ex_op_generic<XLEN_t, XLEN_t,                     std::bit_xor<XLEN_t>,    true>,  print_i_type_instr<"xori", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_or     { ex_op_generic<XLEN_t, XLEN_t,                     std::bit_or<XLEN_t>,     false>, print_r_type_instr<"or"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_ori    { ex_op_generic<XLEN_t, XLEN_t,                     std::bit_or<XLEN_t>,     true>,  print_i_type_instr<"ori", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_and    { ex_op_generic<XLEN_t, XLEN_t,                     std::bit_and<XLEN_t>,    false>, print_r_type_instr<"and"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_andi   { ex_op_generic<XLEN_t, XLEN_t,                     std::bit_and<XLEN_t>,    true>,  print_i_type_instr<"andi", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_mul    { ex_op_generic<XLEN_t, std::make_signed_t<XLEN_t>, std::multiplies<XLEN_t>, false>, print_i_type_instr<"mul", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_mulh   { ex_unimplemented<XLEN_t>, /*TODO*/                                                 print_i_type_instr<"mulh", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_mulhsu { ex_unimplemented<XLEN_t>, /*TODO*/                                                 print_i_type_instr<"mulhsu", false>  };
template<typename XLEN_t> Instruction<XLEN_t> inst_mulhu  { ex_unimplemented<XLEN_t>, /*TODO*/                                                 print_i_type_instr<"mulhu", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_div    { ex_op_generic<XLEN_t, std::make_signed_t<XLEN_t>, std::divides<XLEN_t>,    false>, print_i_type_instr<"div", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_divu   { ex_op_generic<XLEN_t, XLEN_t,                     std::divides<XLEN_t>,    false>, print_i_type_instr<"divu", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_rem    { ex_op_generic<XLEN_t, std::make_signed_t<XLEN_t>, std::modulus<XLEN_t>,    false>, print_i_type_instr<"rem", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_remu   { ex_op_generic<XLEN_t, XLEN_t,                     std::modulus<XLEN_t>,    false>, print_i_type_instr<"remu", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_beq  { ex_branch_generic<XLEN_t, std::equal_to<XLEN_t>>, print_b_type_instr<"beq"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_bne  { ex_branch_generic<XLEN_t, std::not_equal_to<XLEN_t>>, print_b_type_instr<"bne"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_blt  { ex_branch_generic<XLEN_t, std::less<std::make_signed_t<XLEN_t>>>,  print_b_type_instr<"blt"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_bge  { ex_branch_generic<XLEN_t, std::greater_equal<std::make_signed_t<XLEN_t>>>,  print_b_type_instr<"bge"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_bltu { ex_branch_generic<XLEN_t, std::less<XLEN_t>>, print_b_type_instr<"bltu"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_bgeu { ex_branch_generic<XLEN_t, std::greater_equal<XLEN_t>>, print_b_type_instr<"bgeu"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lb  { ex_load_generic<XLEN_t, __int8_t,   false>, print_load_instr<XLEN_t, __uint8_t,  false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lh  { ex_load_generic<XLEN_t, __int16_t,  false>, print_load_instr<XLEN_t, __uint16_t, false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lw  { ex_load_generic<XLEN_t, __int32_t,  false>, print_load_instr<XLEN_t, __uint32_t, false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_ld  { ex_load_generic<XLEN_t, __int64_t,  false>, print_load_instr<XLEN_t, __uint64_t, false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lbu { ex_load_generic<XLEN_t, __uint8_t,  false>, print_load_instr<XLEN_t, __uint8_t,  true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lhu { ex_load_generic<XLEN_t, __uint16_t, false>, print_load_instr<XLEN_t, __uint16_t, true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lwu { ex_load_generic<XLEN_t, __uint32_t, false>, print_load_instr<XLEN_t, __uint32_t, true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lrw { ex_load_generic<XLEN_t, __int32_t,  true>,  print_r_type_instr<"lrw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lrd { ex_load_generic<XLEN_t, __int64_t,  true>,  print_r_type_instr<"lrd"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sb  { ex_store_generic<XLEN_t, __uint8_t>,  print_store_instr<XLEN_t, __uint8_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sh  { ex_store_generic<XLEN_t, __uint16_t>, print_store_instr<XLEN_t, __uint16_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sw  { ex_store_generic<XLEN_t, __uint32_t>, print_store_instr<XLEN_t, __uint32_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sd  { ex_store_generic<XLEN_t, __uint64_t>, print_store_instr<XLEN_t, __uint64_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_scw { ex_scw<XLEN_t>, print_r_type_instr<"scw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_scd { ex_unimplemented<XLEN_t>, print_r_type_instr<"scd"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoaddw  { ex_amo_generic<XLEN_t, __uint32_t, std::plus<XLEN_t>>, print_r_type_instr<"amoadd.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoaddd  { ex_amo_generic<XLEN_t, __uint64_t, std::plus<XLEN_t>>, print_r_type_instr<"amoadd.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoswapw { ex_amo_generic<XLEN_t, __uint32_t, lhs<XLEN_t>>, print_r_type_instr<"amoswap.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoswapd { ex_amo_generic<XLEN_t, __uint64_t, lhs<XLEN_t>>, print_r_type_instr<"amoswap.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoxorw  { ex_amo_generic<XLEN_t, __uint32_t, std::bit_xor<XLEN_t>>, print_r_type_instr<"amoxor.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoxord  { ex_amo_generic<XLEN_t, __uint64_t, std::bit_xor<XLEN_t>>, print_r_type_instr<"amoxor.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoorw   { ex_amo_generic<XLEN_t, __uint32_t, std::bit_or<XLEN_t>>, print_r_type_instr<"amoor.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoord   { ex_amo_generic<XLEN_t, __uint64_t, std::bit_or<XLEN_t>>, print_r_type_instr<"amoor.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoandw  { ex_amo_generic<XLEN_t, __uint32_t, std::bit_and<XLEN_t>>, print_r_type_instr<"amoand.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amoandd  { ex_amo_generic<XLEN_t, __uint64_t, std::bit_and<XLEN_t>>, print_r_type_instr<"amoand.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amominw  { ex_amo_generic<XLEN_t, __int32_t,  min<XLEN_t>>, print_r_type_instr<"amomin.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amomind  { ex_amo_generic<XLEN_t, __int64_t,  min<XLEN_t>>, print_r_type_instr<"amomin.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amomaxw  { ex_amo_generic<XLEN_t, __int32_t,  max<XLEN_t>>, print_r_type_instr<"amomax.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amomaxd  { ex_amo_generic<XLEN_t, __int64_t,  max<XLEN_t>>, print_r_type_instr<"amomax.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amominuw { ex_amo_generic<XLEN_t, __uint32_t, min<XLEN_t>>, print_r_type_instr<"amominu.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amominud { ex_amo_generic<XLEN_t, __uint64_t, min<XLEN_t>>, print_r_type_instr<"amominu.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amomaxuw { ex_amo_generic<XLEN_t, __uint32_t, max<XLEN_t>>, print_r_type_instr<"amomaxu.w"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_amomaxud { ex_amo_generic<XLEN_t, __uint64_t, max<XLEN_t>>, print_r_type_instr<"amomaxu.d"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_lui   { ex_upper_immediate_generic<XLEN_t, false>, print_u_type_instr<"lui", 12> };
template<typename XLEN_t> Instruction<XLEN_t> inst_auipc { ex_upper_immediate_generic<XLEN_t, true>,  print_u_type_instr<"auipc", 0> };
template<typename XLEN_t> Instruction<XLEN_t> inst_jal { ex_jal<XLEN_t>, print_jal<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_jalr { ex_jalr<XLEN_t>, print_i_type_instr<"jalr", false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_fence { ex_fence<XLEN_t>, print_just_mnemonic<"fence"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_fencei { ex_fencei<XLEN_t>, print_just_mnemonic<"fencei"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_ecall { ex_ecall<XLEN_t>, print_just_mnemonic<"ecall"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_ebreak { ex_ebreak<XLEN_t>, print_just_mnemonic<"ebreak"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrrw  { ex_csr_generic<XLEN_t, false, false, false>,  print_csr_instr<"csrrw",  false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrrs  { ex_csr_generic<XLEN_t, true,  false, false>,  print_csr_instr<"csrrs",  false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrrc  { ex_csr_generic<XLEN_t, false, true,  false>,  print_csr_instr<"csrrc",  false> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrrwi { ex_csr_generic<XLEN_t, false, false, true>,   print_csr_instr<"csrrwi", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrrsi { ex_csr_generic<XLEN_t, true,  false, true>,   print_csr_instr<"csrrsi", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrrci { ex_csr_generic<XLEN_t, false, true,  true>,   print_csr_instr<"csrrci", true> };
template<typename XLEN_t> Instruction<XLEN_t> inst_caddi4spn { ex_caddi4spn<XLEN_t>, print_caddi4spn<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_caddi { ex_caddi<XLEN_t>, print_caddi<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cjal { ex_cjal<XLEN_t>, print_cjal<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cli { ex_cli<XLEN_t>, print_cli<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_clui { ex_clui<XLEN_t>, print_clui<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_caddi16sp { ex_caddi16sp<XLEN_t>, print_caddi16sp<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cadd { ex_cadd<XLEN_t>, print_cadd<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csub { ex_ca_format_op<XLEN_t, std::minus<XLEN_t>>, print_ca_format_instr<"(C.SUB) sub"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cxor { ex_ca_format_op<XLEN_t, std::bit_xor<XLEN_t>>, print_ca_format_instr<"(C.XOR) xor"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cor { ex_ca_format_op<XLEN_t, std::bit_or<XLEN_t>>, print_ca_format_instr<"(C.OR) or"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cand { ex_ca_format_op<XLEN_t, std::bit_and<XLEN_t>>, print_ca_format_instr<"(C.AND) and"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cj { ex_cj<XLEN_t>, print_cj<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cbeqz { ex_cbeqz<XLEN_t>, print_cbeqz<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cbnez { ex_cbnez<XLEN_t>, print_cbnez<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_candi { ex_candi<XLEN_t>, print_candi<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cslli { ex_cslli<XLEN_t>, print_cslli<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csw { ex_cs_generic<XLEN_t, __uint32_t>, print_cs_generic<XLEN_t, __uint32_t, "(C.SW) sw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csd { ex_cs_generic<XLEN_t, __uint64_t>, print_cs_generic<XLEN_t, __uint64_t, "(C.SD) sd"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csq { ex_cs_generic<XLEN_t, __uint128_t>, print_cs_generic<XLEN_t, __uint128_t, "(C.SQ) sq"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_clwsp { ex_clwsp<XLEN_t>, print_clwsp<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_clw { ex_cl_generic<XLEN_t, __uint32_t>, print_cl_generic<XLEN_t, __uint32_t, "(C.LW) lw"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cld { ex_cl_generic<XLEN_t, __uint64_t>, print_cl_generic<XLEN_t, __uint64_t, "(C.LD) ld"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_clq { ex_cl_generic<XLEN_t, __uint128_t>, print_cl_generic<XLEN_t, __uint128_t, "(C.LQ) lq"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cswsp { ex_cs_sp<XLEN_t, __uint32_t>, print_cswsp<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csdsp { ex_cs_sp<XLEN_t, __uint64_t>, print_csdsp<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cjalr { ex_cjalr<XLEN_t>, print_cjalr<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cjr { ex_cjr<XLEN_t>, print_cjr<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cmv { ex_cmv<XLEN_t>, print_cmv<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_cebreak { ex_ebreak<XLEN_t>, print_just_mnemonic<"(C.EBREAK) ebreak"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrli { ex_csrli<XLEN_t>, print_csrli<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_csrai { ex_csrai<XLEN_t>, print_csrai<XLEN_t> };
template<typename XLEN_t> Instruction<XLEN_t> inst_wfi { ex_wfi<XLEN_t>, print_just_mnemonic<"wfi"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_uret { ex_trap_return<XLEN_t, RISCV::PrivilegeMode::User>, print_just_mnemonic<"uret"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sret { ex_trap_return<XLEN_t, RISCV::PrivilegeMode::Supervisor>, print_just_mnemonic<"sret"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_mret { ex_trap_return<XLEN_t, RISCV::PrivilegeMode::Machine>, print_just_mnemonic<"mret"> };
template<typename XLEN_t> Instruction<XLEN_t> inst_sfencevma { ex_sfencevma<XLEN_t>, print_just_mnemonic<"sfence.vma"> };
