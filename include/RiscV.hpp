#pragma once

#include <cstdint>
#include <type_traits>
#include <array>
#include <string>

namespace RISCV {

// -- Facts about the RISC-V specification this file describes --

constexpr const char* PrivilegedSpecDate = "20190608";
constexpr const char* UnprivilegedSpecDate = "20191213";

// -- Very generic ISA facts --

constexpr unsigned int NumRegs = 32;

// -- Relevant enums --

enum PrivilegeMode {
    User=0,
    Supervisor=1,
    Machine=3
};

enum XlenMode {
    None = 0,
    XL32 = 1,
    XL64 = 2,
    XL128 = 3
};

enum FloatingPointState {
    Off = 0,
    Initial = 1,
    Clean = 2,
    Dirty = 3
};

enum ExtensionState {
    AllOff = 0,
    NoneDirtyNoneClean = 1,
    NoneDirtySomeClean = 2,
    SomeDirty = 3
};

enum PagingMode {
    Bare = 0, Sv32 = 1, Sv39 = 8, Sv48 = 9, Sv57 = 10, Sv64 = 11
};

enum PTEBit {
    V = 0b00000001,
    R = 0b00000010,
    W = 0b00000100,
    X = 0b00001000,
    U = 0b00010000,
    G = 0b00100000,
    A = 0b01000000,
    D = 0b10000000
};

enum tvecMode {
    Direct = 0,
    Vectored = 1
};

enum pmpAddressMode {
    OFF = 0,
    TOR = 1,
    NA4 = 2,
    NAPOT = 3
};

enum fpRoundingMode {
    RNE = 0,
    RTZ = 1,
    RDN = 2,
    RUP = 3,
    RMM = 4,
    DYN = 7
};

enum TrapCause {

    NONE = -1,

    // Interrupts
    USER_SOFTWARE_INTERRUPT = 0,
    SUPERVISOR_SOFTWARE_INTERRUPT = 1,
    MACHINE_SOFTWARE_INTERRUPT = 3,
    USER_TIMER_INTERRUPT = 4,
    SUPERVISOR_TIMER_INTERRUPT = 5,
    MACHINE_TIMER_INTERRUPT = 7,
    USER_EXTERNAL_INTERRUPT = 8,
    SUPERVISOR_EXTERNAL_INTERRUPT = 9,
    MACHINE_EXTERNAL_INTERRUPT = 11,

    // Exceptions
    INSTRUCTION_ADDRESS_MISALIGNED = 0,
    INSTRUCTION_ACCESS_FAULT = 1,
    ILLEGAL_INSTRUCTION = 2,
    BREAKPOINT = 3,
    LOAD_ADDRESS_MISALIGNED = 4,
    LOAD_ACCESS_FAULT = 5,
    STORE_AMO_ADDRESS_MISALIGNED = 6,
    STORE_AMO_ACCESS_FAULT = 7,
    ECALL_FROM_U_MODE = 8,
    ECALL_FROM_S_MODE = 9,
    // ECALL_FROM_H_MODE = 10,
    ECALL_FROM_M_MODE = 11,
    INSTRUCTION_PAGE_FAULT = 12,
    LOAD_PAGE_FAULT = 13,
    STORE_AMO_PAGE_FAULT = 15
};

enum CSRAddress {

    USTATUS = 0x000, UIE = 0x004, UTVEC = 0x005, USCRATCH = 0x040, UEPC = 0x041,
    UCAUSE = 0x042, UTVAL = 0x043, UIP = 0x044,

    FFLAGS = 0x001, FRM = 0x002, FCSR = 0x003,

    CYCLE = 0xC00, TIME = 0xC01, INSTRET = 0xC02,
    HPMCOUNTER3 = 0xC03, HPMCOUNTER4 = 0xC04, HPMCOUNTER5 = 0xC05,
    HPMCOUNTER6 = 0xC06, HPMCOUNTER7 = 0xC07, HPMCOUNTER8 = 0xC08,
    HPMCOUNTER9 = 0xC09, HPMCOUNTER10 = 0xC0A, HPMCOUNTER11 = 0xC0B,
    HPMCOUNTER12 = 0xC0C, HPMCOUNTER13 = 0xC0D, HPMCOUNTER14 = 0xC0E,
    HPMCOUNTER15 = 0xC0F, HPMCOUNTER16 = 0xC10, HPMCOUNTER17 = 0xC11,
    HPMCOUNTER18 = 0xC12, HPMCOUNTER19 = 0xC13, HPMCOUNTER20 = 0xC14,
    HPMCOUNTER21 = 0xC15, HPMCOUNTER22 = 0xC16, HPMCOUNTER23 = 0xC17,
    HPMCOUNTER24 = 0xC18, HPMCOUNTER25 = 0xC19, HPMCOUNTER26 = 0xC1A,
    HPMCOUNTER27 = 0xC1B, HPMCOUNTER28 = 0xC1C, HPMCOUNTER29 = 0xC1D,
    HPMCOUNTER30 = 0xC1E, HPMCOUNTER31 = 0xC1F,

    CYCLEH = 0xC80, TIMEH = 0xC81, INSTRETH = 0xC82,
    HPMCOUNTER3H = 0xC83, HPMCOUNTER4H = 0xC84, HPMCOUNTER5H = 0xC85,
    HPMCOUNTER6H = 0xC86, HPMCOUNTER7H = 0xC87, HPMCOUNTER8H = 0xC88,
    HPMCOUNTER9H = 0xC89, HPMCOUNTER10H = 0xC8A, HPMCOUNTER11H = 0xC8B,
    HPMCOUNTER12H = 0xC8C, HPMCOUNTER13H = 0xC8D, HPMCOUNTER14H = 0xC8E,
    HPMCOUNTER15H = 0xC8F, HPMCOUNTER16H = 0xC90, HPMCOUNTER17H = 0xC91,
    HPMCOUNTER18H = 0xC92, HPMCOUNTER19H = 0xC93, HPMCOUNTER20H = 0xC94,
    HPMCOUNTER21H = 0xC95, HPMCOUNTER22H = 0xC96, HPMCOUNTER23H = 0xC97,
    HPMCOUNTER24H = 0xC98, HPMCOUNTER25H = 0xC99, HPMCOUNTER26H = 0xC9A,
    HPMCOUNTER27H = 0xC9B, HPMCOUNTER28H = 0xC9C, HPMCOUNTER29H = 0xC9D,
    HPMCOUNTER30H = 0xC9E, HPMCOUNTER31H = 0xC9F,

    SSTATUS = 0x100, SEDELEG = 0x102, SIDELEG = 0x103, SIE = 0x104,
    STVEC = 0x105, SCOUNTEREN = 0x106, SSCRATCH = 0x140, SEPC = 0x141,
    SCAUSE = 0x142, STVAL = 0x143, SIP = 0x144, SATP = 0x180,

    MVENDORID = 0xF11, MARCHID = 0xF12, MIMPID = 0xF13, MHARTID = 0xF14,

    MSTATUS = 0x300, MISA = 0x301, MEDELEG = 0x302, MIDELEG = 0x303,
    MIE = 0x304, MTVEC = 0x305, MCOUNTEREN = 0x306, MSCRATCH = 0x340,
    MEPC = 0x341, MCAUSE = 0x342, MTVAL = 0x343, MIP = 0x344,

    PMPCFG0 = 0x3A0, PMPCFG1 = 0x3A1, PMPCFG2 = 0x3A2, PMPCFG3 = 0x3A3,
    PMPADDR0 = 0x3B0, PMPADDR1 = 0x3B1, PMPADDR2 = 0x3B2, PMPADDR3 = 0x3B3,
    PMPADDR4 = 0x3B4, PMPADDR5 = 0x3B5, PMPADDR6 = 0x3B6, PMPADDR7 = 0x3B7,
    PMPADDR8 = 0x3B8, PMPADDR9 = 0x3B9, PMPADDR10 = 0x3BA, PMPADDR11 = 0x3BB,
    PMPADDR12 = 0x3BC, PMPADDR13 = 0x3BD, PMPADDR14 = 0x3BE, PMPADDR15 = 0x3BF,

    MCYCLE = 0xB00, MINSTRET = 0xB02,
    MHPMCOUNTER3 = 0xB03, MHPMCOUNTER4 = 0xB04, MHPMCOUNTER5 = 0xB05,
    MHPMCOUNTER6 = 0xB06, MHPMCOUNTER7 = 0xB07, MHPMCOUNTER8 = 0xB08,
    MHPMCOUNTER9 = 0xB09, MHPMCOUNTER10 = 0xB0A, MHPMCOUNTER11 = 0xB0B,
    MHPMCOUNTER12 = 0xB0C, MHPMCOUNTER13 = 0xB0D, MHPMCOUNTER14 = 0xB0E,
    MHPMCOUNTER15 = 0xB0F, MHPMCOUNTER16 = 0xB10, MHPMCOUNTER17 = 0xB11,
    MHPMCOUNTER18 = 0xB12, MHPMCOUNTER19 = 0xB13, MHPMCOUNTER20 = 0xB14,
    MHPMCOUNTER21 = 0xB15, MHPMCOUNTER22 = 0xB16, MHPMCOUNTER23 = 0xB17,
    MHPMCOUNTER24 = 0xB18, MHPMCOUNTER25 = 0xB19, MHPMCOUNTER26 = 0xB1A,
    MHPMCOUNTER27 = 0xB1B, MHPMCOUNTER28 = 0xB1C, MHPMCOUNTER29 = 0xB1D,
    MHPMCOUNTER30 = 0xB1E, MHPMCOUNTER31 = 0xB1F,

    MCYCLEH = 0xB80, MINSTRETH = 0xB82,
    MHPMCOUNTER3H = 0xB83, MHPMCOUNTER4H = 0xB84, MHPMCOUNTER5H = 0xB85,
    MHPMCOUNTER6H = 0xB86, MHPMCOUNTER7H = 0xB87, MHPMCOUNTER8H = 0xB88,
    MHPMCOUNTER9H = 0xB89, MHPMCOUNTER10H = 0xB8A, MHPMCOUNTER11H = 0xB8B,
    MHPMCOUNTER12H = 0xB8C, MHPMCOUNTER13H = 0xB8D, MHPMCOUNTER14H = 0xB8E,
    MHPMCOUNTER15H = 0xB8F, MHPMCOUNTER16H = 0xB90, MHPMCOUNTER17H = 0xB91,
    MHPMCOUNTER18H = 0xB92, MHPMCOUNTER19H = 0xB93, MHPMCOUNTER20H = 0xB94,
    MHPMCOUNTER21H = 0xB95, MHPMCOUNTER22H = 0xB96, MHPMCOUNTER23H = 0xB97,
    MHPMCOUNTER24H = 0xB98, MHPMCOUNTER25H = 0xB99, MHPMCOUNTER26H = 0xB9A,
    MHPMCOUNTER27H = 0xB9B, MHPMCOUNTER28H = 0xB9C, MHPMCOUNTER29H = 0xB9D,
    MHPMCOUNTER30H = 0xB9E, MHPMCOUNTER31H = 0xB9F,

    MCOUNTINHIBIT = 0x320,
    MHPMEVENT3 = 0x323, MHPMEVENT4 = 0x324, MHPMEVENT5 = 0x325,
    MHPMEVENT6 = 0x326, MHPMEVENT7 = 0x327, MHPMEVENT8 = 0x328,
    MHPMEVENT9 = 0x329, MHPMEVENT10 = 0x32A, MHPMEVENT11 = 0x32B,
    MHPMEVENT12 = 0x32C, MHPMEVENT13 = 0x32D, MHPMEVENT14 = 0x32E,
    MHPMEVENT15 = 0x32F, MHPMEVENT16 = 0x330, MHPMEVENT17 = 0x331,
    MHPMEVENT18 = 0x332, MHPMEVENT19 = 0x333, MHPMEVENT20 = 0x334,
    MHPMEVENT21 = 0x335, MHPMEVENT22 = 0x336, MHPMEVENT23 = 0x337,
    MHPMEVENT24 = 0x338, MHPMEVENT25 = 0x339, MHPMEVENT26 = 0x33A,
    MHPMEVENT27 = 0x33B, MHPMEVENT28 = 0x33C, MHPMEVENT29 = 0x33D,
    MHPMEVENT30 = 0x33E, MHPMEVENT31 = 0x33F,

    TSELECT = 0x7A0, TDATA1 = 0x7A1, TDATA2 = 0x7A2, TDATA3 = 0x7A3,
    DCSR = 0x7B0, DPC = 0x7B1, DSCRATCH0 = 0x7B2, DSCRATCH1 = 0x7B3,
    INVALID_CSR = 0x1000
};

// -- String names from enum values --

inline std::string floatingPointStateName(FloatingPointState fpState) {
    switch (fpState) {
    case Off:
        return "Off";
        break;
    case Initial:
        return "Initial";
        break;
    case Clean:
        return "Clean";
        break;
    case Dirty:
        return "Dirty";
        break;
    default:
        return "Nonsense FP state!";
        break;
    }
}

inline std::string extensionStateName(ExtensionState extState) {
    switch (extState) {
    case AllOff:
        return "All off";
        break;
    case NoneDirtyNoneClean:
        return "None dirty, none clean";
        break;
    case NoneDirtySomeClean:
        return "None dirty, some clean";
        break;
    case SomeDirty:
        return "Some dirty";
        break;
    default:
        return "Nonsense extension state!";
        break;
    }
}

inline std::string pagingModeName(PagingMode pagingMode) {
    switch (pagingMode) {
    case Bare:
        return "Bare";
        break;
    case Sv32:
        return "Sv32";
        break;
    case Sv39:
        return "Sv39";
        break;
    case Sv48:
        return "Sv48";
        break;
    case Sv57:
        return "Sv57";
        break;
    case Sv64:
        return "Sv64";
        break;
    default:
        return "Unknown";
        break;
    }
}

inline std::string tvecModeName(tvecMode mode) {
    return mode == Direct ? "Direct" : "Vectored";
};

inline std::string trapName(bool interrupt, TrapCause trapCause) {
    if (interrupt) {
        switch (trapCause) {
        case USER_SOFTWARE_INTERRUPT:
            return "User software interrupt";
            break;
        case SUPERVISOR_SOFTWARE_INTERRUPT:
            return "User software interrupt";
            break;
        case MACHINE_SOFTWARE_INTERRUPT:
            return "User software interrupt";
            break;
        case USER_TIMER_INTERRUPT:
            return "User software interrupt";
            break;
        case SUPERVISOR_TIMER_INTERRUPT:
            return "User software interrupt";
            break;
        case MACHINE_TIMER_INTERRUPT:
            return "User software interrupt";
            break;
        case USER_EXTERNAL_INTERRUPT:
            return "User software interrupt";
            break;
        case SUPERVISOR_EXTERNAL_INTERRUPT:
            return "User software interrupt";
            break;
        case MACHINE_EXTERNAL_INTERRUPT:
            return "User software interrupt";
            break;
        default:
            return "Unknown interrupt";
        }
    }
    switch (trapCause) {
    case INSTRUCTION_ADDRESS_MISALIGNED:
        return "Instruction address misaligned";
        break;
    case INSTRUCTION_ACCESS_FAULT:
        return "Instruction access fault";
        break;
    case ILLEGAL_INSTRUCTION:
        return "Illegal instruction";
        break;
    case BREAKPOINT:
        return "Breakpoint";
        break;
    case LOAD_ADDRESS_MISALIGNED:
        return "Load address misaligned";
        break;
    case LOAD_ACCESS_FAULT:
        return "Load access fault";
        break;
    case STORE_AMO_ADDRESS_MISALIGNED:
        return "Store/AMO address misaligned";
        break;
    case STORE_AMO_ACCESS_FAULT:
        return "Store/AMO access fault";
        break;
    case ECALL_FROM_U_MODE:
        return "ECall from user mode";
        break;
    case ECALL_FROM_S_MODE:
        return "ECall from supervisor mode";
        break;
    case ECALL_FROM_M_MODE:
        return "ECall from machine mode";
        break;
    case INSTRUCTION_PAGE_FAULT:
        return "Instruction page fault";
        break;
    case LOAD_PAGE_FAULT:
        return "Load page fault";
        break;
    case STORE_AMO_PAGE_FAULT:
        return "Store/AMO page fault";
        break;
    default:
        return "Unknown exception";
    }

}

inline std::string privilegeModeName(PrivilegeMode privilegeMode) {
    if (privilegeMode == PrivilegeMode::Machine)
        return "Machine";
    if (privilegeMode == PrivilegeMode::Supervisor)
        return "Supervisor";
    return "User";
}

constexpr std::array<const char*, NumRegs> registerAbiNames = {
    "zero", "ra", "sp", "gp", "tp",
    "t0", "t1", "t2",
    "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6"
};

constexpr std::array<const char*, NumRegs> registerFlatNames = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
    "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20", "x21", "x22",
    "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"
};

inline std::string regName(unsigned int regNum, bool flat=false) {
    if (regNum >= NumRegs)
        return "(invalid register #" + std::to_string(regNum) + ")";
    if (flat)
        return registerFlatNames[regNum];
    return registerAbiNames[regNum];
}

// -- Facts about RISC-V extension vectors --

constexpr inline __uint32_t stringToExtensions(const char *isa) {
    __uint32_t vec = 0;
    for (; *isa; isa++)
        vec |= (1 << (*isa - (((*isa >= 'a') && (*isa <= 'z')) ? 'a' : 'A')));
    return vec;
}

inline std::string extensionsToString(__uint32_t extensions) {
    // TODO this is stupid but I'm tired... make it less stupid
    char buf[33];
    int bufIdx = 0;
    for (unsigned int i = 0; i < 32; i++)
        if (extensions & (1<<i))
            buf[bufIdx++] = 'a'+i;
    buf[bufIdx] = 0;
    return std::string(buf);
}

// TODO case-insensitive please
constexpr inline bool vectorHasExtension(__uint32_t vector, char extension) {
    return vector & (1 <<  (extension - 'A'));
}

constexpr inline const char * xlenModeName(XlenMode xlenMode) {
    if (xlenMode == XlenMode::XL32)
        return "32";
    if (xlenMode == XlenMode::XL64)
        return "64";
    return "128";
}

inline XlenMode xlenNameToMode(std::string xlenName) {
    if (xlenName.compare("32") == 0 || xlenName.compare("XL32") == 0) {
        return XlenMode::XL32;
    } else if (xlenName.compare("64") == 0 || xlenName.compare("XL64") == 0) {
        return XlenMode::XL64;
    } else if (xlenName.compare("128") == 0 || xlenName.compare("XL128") == 0) {
        return XlenMode::XL128;
    }
    return XlenMode::None;
}

template<typename XLEN_t>
constexpr XlenMode xlenTypeToMode() {
    if (std::is_same<XLEN_t, __uint32_t>())
        return XlenMode::XL32;
    if (std::is_same<XLEN_t, __uint64_t>())
        return XlenMode::XL64;
    if (std::is_same<XLEN_t, __uint128_t>())
        return XlenMode::XL128;
    return XlenMode::None;
}

template <XlenMode xlen> struct XlenModeToType;
template <> struct XlenModeToType<XlenMode::XL32> { using type = __uint32_t; };
template <> struct XlenModeToType<XlenMode::XL64> { using type = __uint64_t; };
template <> struct XlenModeToType<XlenMode::XL128> { using type = __uint128_t; };

// TODO the direct name <-> type leg of this triangle

// -- Facts about RISC-V instruction encodings --

enum OpcodeQuadrant {
    Q0           = 0b00, Q1           = 0b01,
    Q2           = 0b10, UNCOMPRESSED = 0b11
};

enum MajorOpcode {
    LOAD       = 0b00000,    LOAD_FP    = 0b00001,    CUSTOM_0   = 0b00010,
    MISC_MEM   = 0b00011,    OP_IMM     = 0b00100,    AUIPC      = 0b00101,
    OP_IMM_32  = 0b00110,    LONG_48B_1 = 0b00111,    STORE      = 0b01000,
    STORE_FP   = 0b01001,    CUSTOM_1   = 0b01010,    AMO        = 0b01011,
    OP         = 0b01100,    LUI        = 0b01101,    OP_32      = 0b01110,
    LONG_64B   = 0b01111,    MADD       = 0b10000,    MSUB       = 0b10001,
    NMSUB      = 0b10010,    NMADD      = 0b10011,    OP_FP      = 0b10100,
    RESERVED_0 = 0b10101,    CUSTOM_2   = 0b10110,    LONG_48B_2 = 0b10111,
    BRANCH     = 0b11000,    JALR       = 0b11001,    RESERVED_1 = 0b11010,
    JAL        = 0b11011,    SYSTEM     = 0b11100,    RESERVED_2 = 0b11101,
    CUSTOM_3   = 0b11110,    LONG_80B   = 0b11111
};

enum MinorOpcode {

    // OP
    ADD = 0, SLL = 1, SLT = 2, SLTU = 3, XOR = 4, SRL = 5, OR = 6, AND = 7, SUB = 256, SRA = 261,

    // OP-32
    ADDW = 0, SLLW = 1, SRLW = 5, SUBW = 256, SRAW = 261,

    // OP-IMM
    ADDI = 0, SLLI = 1, SLTI = 2, SLTIU = 3, XORI = 4, SRI = 5, ORI = 6, ANDI = 7,

    // OP-IMM-32
    ADDIW = 0, SLLIW = 1, SRIW = 5,

    // MULDIV
    MUL = 8, MULH = 9, MULHSU = 10, MULHU = 11, DIV = 12, DIVU = 13, REM = 14, REMU = 15,

    // BRANCH
    BEQ = 0, BNE = 1, BLT = 4, BGE = 5, BLTU = 6, BGEU = 7,

    // LOAD
    LB = 0, LH = 1, LW = 2, LD = 3, LBU = 4, LHU = 5, LWU = 6,

    // STORE
    SB = 0, SH = 1, SW = 2, SD = 3,

    // SYSTEM
    PRIV = 0, CSRRW = 1, CSRRS = 2, CSRRC = 3, CSRRWI = 5, CSRRSI = 6, CSRRCI = 7,

    // MISC-MEM
    FENCE = 0, FENCE_I = 1,

    // Atomic operations standard extension
    AMOADD = 0, AMOSWAP = 1, LR = 2, SC = 3, AMOXOR = 4, AMOOR = 8, AMOAND = 12, AMOMIN = 16, AMOMAX = 20, AMOMINU = 24, AMOMAXU = 28
};

enum SubMinorOpcode {

    // SYSTEM / PRIV
    ECALL_EBREAK_URET = 0, SRET_WFI = 8, MRET = 24, SFENCE_VMA = 9

};

enum SubSubMinorOpcode {

    // SYSTEM / PRIV / ECALL_EBREAK_URET
    ECALL = 0, EBREAK = 1, URET = 2,

    // SYSTEM / PRIV / SRET_WFI
    SRET = 2, WFI = 5

};

enum AmoWidth {
    AMO_W = 2,
    AMO_D = 3
};

constexpr bool isCompressed(__uint32_t encodedInstruction) {
    return (encodedInstruction & 0x00000003) != 0x00000003;
}

constexpr unsigned int instructionLength(__uint32_t encodedInstruction) {
    return isCompressed(encodedInstruction) ? 2 : 4; // TODO extended lengths
}

// -- Facts about Configuration & Status Registers --

constexpr unsigned int NumCSRs = 0x1000;

constexpr std::array<const char*, NumCSRs> getCSRNameTable() {
    std::array<const char*, NumCSRs> table = {0};
    table[CSRAddress::USTATUS] = "ustatus";
    table[CSRAddress::UIE] = "uie";
    table[CSRAddress::UTVEC] = "utvec";
    table[CSRAddress::USCRATCH] = "uscratch";
    table[CSRAddress::UEPC] = "uepc";
    table[CSRAddress::UCAUSE] = "ucause";
    table[CSRAddress::UTVAL] = "utval";
    table[CSRAddress::UIP] = "uip";
    table[CSRAddress::FFLAGS] = "fflags";
    table[CSRAddress::FRM] = "frm";
    table[CSRAddress::FCSR] = "fcsr";
    table[CSRAddress::CYCLE] = "cycle";
    table[CSRAddress::TIME] = "time";
    table[CSRAddress::INSTRET] = "instret";
    table[CSRAddress::HPMCOUNTER3] = "hpmcounter3";
    table[CSRAddress::HPMCOUNTER4] = "hpmcounter4";
    table[CSRAddress::HPMCOUNTER5] = "hpmcounter5";
    table[CSRAddress::HPMCOUNTER6] = "hpmcounter6";
    table[CSRAddress::HPMCOUNTER7] = "hpmcounter7";
    table[CSRAddress::HPMCOUNTER8] = "hpmcounter8";
    table[CSRAddress::HPMCOUNTER9] = "hpmcounter9";
    table[CSRAddress::HPMCOUNTER10] = "hpmcounter10";
    table[CSRAddress::HPMCOUNTER11] = "hpmcounter11";
    table[CSRAddress::HPMCOUNTER12] = "hpmcounter12";
    table[CSRAddress::HPMCOUNTER13] = "hpmcounter13";
    table[CSRAddress::HPMCOUNTER14] = "hpmcounter14";
    table[CSRAddress::HPMCOUNTER15] = "hpmcounter15";
    table[CSRAddress::HPMCOUNTER16] = "hpmcounter16";
    table[CSRAddress::HPMCOUNTER17] = "hpmcounter17";
    table[CSRAddress::HPMCOUNTER18] = "hpmcounter18";
    table[CSRAddress::HPMCOUNTER19] = "hpmcounter19";
    table[CSRAddress::HPMCOUNTER20] = "hpmcounter20";
    table[CSRAddress::HPMCOUNTER21] = "hpmcounter21";
    table[CSRAddress::HPMCOUNTER22] = "hpmcounter22";
    table[CSRAddress::HPMCOUNTER23] = "hpmcounter23";
    table[CSRAddress::HPMCOUNTER24] = "hpmcounter24";
    table[CSRAddress::HPMCOUNTER25] = "hpmcounter25";
    table[CSRAddress::HPMCOUNTER26] = "hpmcounter26";
    table[CSRAddress::HPMCOUNTER27] = "hpmcounter27";
    table[CSRAddress::HPMCOUNTER28] = "hpmcounter28";
    table[CSRAddress::HPMCOUNTER29] = "hpmcounter29";
    table[CSRAddress::HPMCOUNTER30] = "hpmcounter30";
    table[CSRAddress::HPMCOUNTER31] = "hpmcounter31";
    table[CSRAddress::CYCLEH] = "cycleh";
    table[CSRAddress::TIMEH] = "timeh";
    table[CSRAddress::INSTRETH] = "instreth";
    table[CSRAddress::HPMCOUNTER3H] = "hpmcounter3h";
    table[CSRAddress::HPMCOUNTER4H] = "hpmcounter4h";
    table[CSRAddress::HPMCOUNTER5H] = "hpmcounter5h";
    table[CSRAddress::HPMCOUNTER6H] = "hpmcounter6h";
    table[CSRAddress::HPMCOUNTER7H] = "hpmcounter7h";
    table[CSRAddress::HPMCOUNTER8H] = "hpmcounter8h";
    table[CSRAddress::HPMCOUNTER9H] = "hpmcounter9h";
    table[CSRAddress::HPMCOUNTER10H] = "hpmcounter10h";
    table[CSRAddress::HPMCOUNTER11H] = "hpmcounter11h";
    table[CSRAddress::HPMCOUNTER12H] = "hpmcounter12h";
    table[CSRAddress::HPMCOUNTER13H] = "hpmcounter13h";
    table[CSRAddress::HPMCOUNTER14H] = "hpmcounter14h";
    table[CSRAddress::HPMCOUNTER15H] = "hpmcounter15h";
    table[CSRAddress::HPMCOUNTER16H] = "hpmcounter16h";
    table[CSRAddress::HPMCOUNTER17H] = "hpmcounter17h";
    table[CSRAddress::HPMCOUNTER18H] = "hpmcounter18h";
    table[CSRAddress::HPMCOUNTER19H] = "hpmcounter19h";
    table[CSRAddress::HPMCOUNTER20H] = "hpmcounter20h";
    table[CSRAddress::HPMCOUNTER21H] = "hpmcounter21h";
    table[CSRAddress::HPMCOUNTER22H] = "hpmcounter22h";
    table[CSRAddress::HPMCOUNTER23H] = "hpmcounter23h";
    table[CSRAddress::HPMCOUNTER24H] = "hpmcounter24h";
    table[CSRAddress::HPMCOUNTER25H] = "hpmcounter25h";
    table[CSRAddress::HPMCOUNTER26H] = "hpmcounter26h";
    table[CSRAddress::HPMCOUNTER27H] = "hpmcounter27h";
    table[CSRAddress::HPMCOUNTER28H] = "hpmcounter28h";
    table[CSRAddress::HPMCOUNTER29H] = "hpmcounter29h";
    table[CSRAddress::HPMCOUNTER30H] = "hpmcounter30h";
    table[CSRAddress::HPMCOUNTER31H] = "hpmcounter31h";
    table[CSRAddress::SSTATUS] = "sstatus";
    table[CSRAddress::SEDELEG] = "sedeleg";
    table[CSRAddress::SIDELEG] = "sideleg";
    table[CSRAddress::SIE] = "sie";
    table[CSRAddress::STVEC] = "stvec";
    table[CSRAddress::SCOUNTEREN] = "scounteren";
    table[CSRAddress::SSCRATCH] = "sscratch";
    table[CSRAddress::SEPC] = "sepc";
    table[CSRAddress::SCAUSE] = "scause";
    table[CSRAddress::STVAL] = "stval";
    table[CSRAddress::SIP] = "sip";
    table[CSRAddress::SATP] = "satp";
    table[CSRAddress::MVENDORID] = "mvendorid";
    table[CSRAddress::MARCHID] = "marchid";
    table[CSRAddress::MIMPID] = "mimpid";
    table[CSRAddress::MHARTID] = "mhartid";
    table[CSRAddress::MSTATUS] = "mstatus";
    table[CSRAddress::MISA] = "misa";
    table[CSRAddress::MEDELEG] = "medeleg";
    table[CSRAddress::MIDELEG] = "mideleg";
    table[CSRAddress::MIE] = "mie";
    table[CSRAddress::MTVEC] = "mtvec";
    table[CSRAddress::MCOUNTEREN] = "mcounteren";
    table[CSRAddress::MSCRATCH] = "mscratch";
    table[CSRAddress::MEPC] = "mepc";
    table[CSRAddress::MCAUSE] = "mcause";
    table[CSRAddress::MTVAL] = "mtval";
    table[CSRAddress::MIP] = "mip";
    table[CSRAddress::PMPCFG0] = "pmpcfg0";
    table[CSRAddress::PMPCFG1] = "pmpcfg1";
    table[CSRAddress::PMPCFG2] = "pmpcfg2";
    table[CSRAddress::PMPCFG3] = "pmpcfg3";
    table[CSRAddress::PMPADDR0] = "pmpaddr0";
    table[CSRAddress::PMPADDR1] = "pmpaddr1";
    table[CSRAddress::PMPADDR2] = "pmpaddr2";
    table[CSRAddress::PMPADDR3] = "pmpaddr3";
    table[CSRAddress::PMPADDR4] = "pmpaddr4";
    table[CSRAddress::PMPADDR5] = "pmpaddr5";
    table[CSRAddress::PMPADDR6] = "pmpaddr6";
    table[CSRAddress::PMPADDR7] = "pmpaddr7";
    table[CSRAddress::PMPADDR8] = "pmpaddr8";
    table[CSRAddress::PMPADDR9] = "pmpaddr9";
    table[CSRAddress::PMPADDR10] = "pmpaddr10";
    table[CSRAddress::PMPADDR11] = "pmpaddr11";
    table[CSRAddress::PMPADDR12] = "pmpaddr12";
    table[CSRAddress::PMPADDR13] = "pmpaddr13";
    table[CSRAddress::PMPADDR14] = "pmpaddr14";
    table[CSRAddress::PMPADDR15] = "pmpaddr15";
    table[CSRAddress::MCYCLE] = "mcycle";
    table[CSRAddress::MINSTRET] = "minstret";
    table[CSRAddress::MHPMCOUNTER3] = "mhpmcounter3";
    table[CSRAddress::MHPMCOUNTER4] = "mhpmcounter4";
    table[CSRAddress::MHPMCOUNTER31] = "mhpmcounter31";
    table[CSRAddress::MCYCLEH] = "mcycleh";
    table[CSRAddress::MINSTRETH] = "minstreth";
    table[CSRAddress::MHPMCOUNTER3H] = "mhpmcounter3h";
    table[CSRAddress::MHPMCOUNTER4H] = "mhpmcounter4h";
    table[CSRAddress::MHPMCOUNTER5H] = "mhpmcounter5h";
    table[CSRAddress::MHPMCOUNTER6H] = "mhpmcounter6h";
    table[CSRAddress::MHPMCOUNTER7H] = "mhpmcounter7h";
    table[CSRAddress::MHPMCOUNTER8H] = "mhpmcounter8h";
    table[CSRAddress::MHPMCOUNTER9H] = "mhpmcounter9h";
    table[CSRAddress::MHPMCOUNTER10H] = "mhpmcounter10h";
    table[CSRAddress::MHPMCOUNTER11H] = "mhpmcounter11h";
    table[CSRAddress::MHPMCOUNTER12H] = "mhpmcounter12h";
    table[CSRAddress::MHPMCOUNTER13H] = "mhpmcounter13h";
    table[CSRAddress::MHPMCOUNTER14H] = "mhpmcounter14h";
    table[CSRAddress::MHPMCOUNTER15H] = "mhpmcounter15h";
    table[CSRAddress::MHPMCOUNTER16H] = "mhpmcounter16h";
    table[CSRAddress::MHPMCOUNTER17H] = "mhpmcounter17h";
    table[CSRAddress::MHPMCOUNTER18H] = "mhpmcounter18h";
    table[CSRAddress::MHPMCOUNTER19H] = "mhpmcounter19h";
    table[CSRAddress::MHPMCOUNTER20H] = "mhpmcounter20h";
    table[CSRAddress::MHPMCOUNTER21H] = "mhpmcounter21h";
    table[CSRAddress::MHPMCOUNTER22H] = "mhpmcounter22h";
    table[CSRAddress::MHPMCOUNTER23H] = "mhpmcounter23h";
    table[CSRAddress::MHPMCOUNTER24H] = "mhpmcounter24h";
    table[CSRAddress::MHPMCOUNTER25H] = "mhpmcounter25h";
    table[CSRAddress::MHPMCOUNTER26H] = "mhpmcounter26h";
    table[CSRAddress::MHPMCOUNTER27H] = "mhpmcounter27h";
    table[CSRAddress::MHPMCOUNTER28H] = "mhpmcounter28h";
    table[CSRAddress::MHPMCOUNTER29H] = "mhpmcounter29h";
    table[CSRAddress::MHPMCOUNTER30H] = "mhpmcounter30h";
    table[CSRAddress::MHPMCOUNTER31H] = "mhpmcounter31h";
    table[CSRAddress::MCOUNTINHIBIT] = "mcountinhibit";
    table[CSRAddress::MHPMEVENT3] = "mhpmevent3";
    table[CSRAddress::MHPMEVENT4] = "mhpmevent4";
    table[CSRAddress::MHPMEVENT5] = "mhpmevent5";
    table[CSRAddress::MHPMEVENT6] = "mhpmevent6";
    table[CSRAddress::MHPMEVENT7] = "mhpmevent7";
    table[CSRAddress::MHPMEVENT8] = "mhpmevent8";
    table[CSRAddress::MHPMEVENT9] = "mhpmevent9";
    table[CSRAddress::MHPMEVENT10] = "mhpmevent10";
    table[CSRAddress::MHPMEVENT11] = "mhpmevent11";
    table[CSRAddress::MHPMEVENT12] = "mhpmevent12";
    table[CSRAddress::MHPMEVENT13] = "mhpmevent13";
    table[CSRAddress::MHPMEVENT14] = "mhpmevent14";
    table[CSRAddress::MHPMEVENT15] = "mhpmevent15";
    table[CSRAddress::MHPMEVENT16] = "mhpmevent16";
    table[CSRAddress::MHPMEVENT17] = "mhpmevent17";
    table[CSRAddress::MHPMEVENT18] = "mhpmevent18";
    table[CSRAddress::MHPMEVENT19] = "mhpmevent19";
    table[CSRAddress::MHPMEVENT20] = "mhpmevent20";
    table[CSRAddress::MHPMEVENT21] = "mhpmevent21";
    table[CSRAddress::MHPMEVENT22] = "mhpmevent22";
    table[CSRAddress::MHPMEVENT23] = "mhpmevent23";
    table[CSRAddress::MHPMEVENT24] = "mhpmevent24";
    table[CSRAddress::MHPMEVENT25] = "mhpmevent25";
    table[CSRAddress::MHPMEVENT26] = "mhpmevent26";
    table[CSRAddress::MHPMEVENT27] = "mhpmevent27";
    table[CSRAddress::MHPMEVENT28] = "mhpmevent28";
    table[CSRAddress::MHPMEVENT29] = "mhpmevent29";
    table[CSRAddress::MHPMEVENT30] = "mhpmevent30";
    table[CSRAddress::MHPMEVENT31] = "mhpmevent31";
    table[CSRAddress::TSELECT] = "tselect";
    table[CSRAddress::TDATA1] = "tdata1";
    table[CSRAddress::TDATA2] = "tdata2";
    table[CSRAddress::TDATA3] = "tdata3";
    table[CSRAddress::DCSR] = "dcsr";
    table[CSRAddress::DPC] = "dpc";
    table[CSRAddress::DSCRATCH0] = "dscratch0";
    table[CSRAddress::DSCRATCH1] = "dscratch1";
    return table;
}

constexpr std::array<const char*, NumCSRs> csrNames = getCSRNameTable();

inline std::string csrName(unsigned int address) {
    if (address >= NumCSRs)
        return "(invalid CSR #" + std::to_string(address) + ")";
    return csrNames[address];
}

inline constexpr PrivilegeMode csrRequiredPrivilege(CSRAddress addr) {
    return (PrivilegeMode) ((addr & 0b001100000000) >> 8);
}

inline bool csrIsReadOnly(CSRAddress addr) {
    return (addr & 0b110000000000) == 0b110000000000;
}

// -- Facts about interrupts, exceptions, and traps --

// TODO move to interruptReg after making m/s,e/ideleg interruptReg...
// ugh, crap, the exceptions aren't ireg.
constexpr static __uint32_t usiMask = 0b0000000000001;
constexpr static __uint32_t ssiMask = 0b0000000000010;
constexpr static __uint32_t msiMask = 0b0000000001000;
constexpr static __uint32_t utiMask = 0b0000000010000;
constexpr static __uint32_t stiMask = 0b0000000100000;
constexpr static __uint32_t mtiMask = 0b0000010000000;
constexpr static __uint32_t ueiMask = 0b0000100000000;
constexpr static __uint32_t seiMask = 0b0001000000000;
constexpr static __uint32_t meiMask = 0b0100000000000;

template<typename XLEN_t>
PrivilegeMode DestinedPrivilegeForCause(TrapCause cause, XLEN_t mdeleg, XLEN_t sdeleg, __uint32_t extensions) {

    XLEN_t causeMask = 1 << cause;

    // Without U mode, there is no S mode either, so M mode takes it.
    if (!vectorHasExtension(extensions, 'U')) {
        return PrivilegeMode::Machine;
    }

    // If M mode hasn't delegated the trap, it takes it.
    if (!(mdeleg & causeMask)) {
        return PrivilegeMode::Machine;
    }

    // If there is no S mode, U mode takes it.
    if (!vectorHasExtension(extensions, 'S')) {
        return PrivilegeMode::User;
    }

    // If S mode hasn't delegated the trap, it takes it.
    if (!(sdeleg & causeMask)) {
        return PrivilegeMode::Supervisor;
    }

    // M mode has delegated, and S mode has either also delegated or doesn't
    // exist, so U mode takes it.
    return PrivilegeMode::User;
}

template<typename XLEN_t>
TrapCause highestPriorityInterrupt(XLEN_t interruptsToService) {
    // The order in the spec is: MEI MSI MTI SEI SSI STI UEI USI UTI
    if (interruptsToService & (1 << TrapCause::MACHINE_EXTERNAL_INTERRUPT)) {
        return TrapCause::MACHINE_EXTERNAL_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::MACHINE_SOFTWARE_INTERRUPT)) {
        return TrapCause::MACHINE_SOFTWARE_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::MACHINE_TIMER_INTERRUPT)) {
        return TrapCause::MACHINE_TIMER_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::SUPERVISOR_EXTERNAL_INTERRUPT)) {
        return TrapCause::SUPERVISOR_EXTERNAL_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::SUPERVISOR_SOFTWARE_INTERRUPT)) {
        return TrapCause::SUPERVISOR_SOFTWARE_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::SUPERVISOR_TIMER_INTERRUPT)) {
        return TrapCause::SUPERVISOR_TIMER_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::USER_EXTERNAL_INTERRUPT)) {
        return TrapCause::USER_EXTERNAL_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::USER_SOFTWARE_INTERRUPT)) {
        return TrapCause::USER_SOFTWARE_INTERRUPT;
    } else if (interruptsToService & (1 << TrapCause::USER_TIMER_INTERRUPT)) {
        return TrapCause::USER_TIMER_INTERRUPT;
    } else {
        // This indicates custom interrupts... TODO ?
        return TrapCause::NONE;
    }
}

// TODO comment for what this section of the spec-knowledge is. In general I need to sort this doc...

struct misaReg {

    XlenMode mxlen;
    __uint32_t extensions;
    const __uint32_t maximalExtensions;

    misaReg(__uint32_t maximalExtensions) :
        maximalExtensions(maximalExtensions) { }

    template <typename XLEN_t>
    void Write(XLEN_t value) {
        unsigned int shift = (sizeof(XLEN_t)*8)-2;
        extensions = value & 0x3ffffff & maximalExtensions;
        mxlen = (RISCV::XlenMode)(value >> shift);
    }

    template <typename XLEN_t>
    XLEN_t Read() {
        unsigned int shift = (sizeof(XLEN_t)*8)-2;
        return (((XLEN_t)mxlen) << shift) | extensions;
    }

    template <typename MXLEN_t>
    void Reset() {
        mxlen = xlenTypeToMode<MXLEN_t>();
        extensions = maximalExtensions;
    }
};

struct mstatusReg {

    constexpr static __uint32_t uieMask  = 0b00000000000000000000001;
    constexpr static __uint32_t sieMask  = 0b00000000000000000000010;
    constexpr static __uint32_t mieMask  = 0b00000000000000000001000;
    constexpr static __uint32_t upieMask = 0b00000000000000000010000;
    constexpr static __uint32_t spieMask = 0b00000000000000000100000;
    constexpr static __uint32_t mpieMask = 0b00000000000000010000000;
    constexpr static __uint32_t sppMask  = 0b00000000000000100000000;
    constexpr static __uint32_t mppMask  = 0b00000000001100000000000;
    constexpr static __uint32_t fsMask   = 0b00000000110000000000000;
    constexpr static __uint32_t xsMask   = 0b00000011000000000000000;
    constexpr static __uint32_t mprvMask = 0b00000100000000000000000;
    constexpr static __uint32_t sumMask  = 0b00001000000000000000000;
    constexpr static __uint32_t mxrMask  = 0b00010000000000000000000;
    constexpr static __uint32_t tvmMask  = 0b00100000000000000000000;
    constexpr static __uint32_t twMask   = 0b01000000000000000000000;
    constexpr static __uint32_t tsrMask  = 0b10000000000000000000000;
    constexpr static __uint32_t sdMask32 = 0x80000000;
    constexpr static __uint64_t sdMask64 = 0x8000000000000000;
    constexpr static __uint64_t uxlMask  = 0x0000000300000000;
    constexpr static __uint64_t sxlMask  = 0x0000000C00000000;
    // constexpr static __uint128_t sdMask128 = 0x80000000000000000000000000000000;
    constexpr static __uint32_t sppShift = 8;
    constexpr static __uint32_t mppShift = 11;
    constexpr static __uint32_t fsShift  = 13;
    constexpr static __uint32_t xsShift  = 15;
    constexpr static __uint32_t uxlShift = 32;
    constexpr static __uint32_t sxlShift = 34;

    // TODO accessor pattern and option to store as packed form instead?
    bool uie, sie, mie, upie, spie, mpie;
    PrivilegeMode spp, mpp;
    FloatingPointState fs;
    ExtensionState xs;
    bool mprv, sum, mxr, tvm, tw, tsr;
    XlenMode sxl, uxl;
    bool sd;

    template <typename XLEN_t, PrivilegeMode viewPrivilege>
    inline void Write(XLEN_t value) {
        uie = uieMask & value;
        upie = upieMask & value;
        if constexpr (viewPrivilege != PrivilegeMode::User) {
            sie = sieMask & value;
            spie = spieMask & value;
            spp = (PrivilegeMode)((sppMask & value) >> sppShift);
            fs = (FloatingPointState)((fsMask & value) >> fsShift);
            xs = (ExtensionState)((xsMask & value) >> xsShift);
            sum = sumMask & value;
            mxr = mxrMask & value;
            if constexpr (std::is_same<XLEN_t, __uint32_t>()) {
                sd = value & sdMask32;
            } else {
                sd = value & sdMask64;
            } // TODO, bug, 128-bit sd mask is 1<<127, hard to express
        }
        if constexpr (viewPrivilege == PrivilegeMode::Machine) {
            mie = mieMask & value;
            mpie = mpieMask & value;
            mpp = (PrivilegeMode)((mppMask & value) >> mppShift);
            if constexpr (std::is_same<XLEN_t, __uint32_t>()) {
                uxl = XlenMode::XL32;
                sxl = XlenMode::XL32;
            } else {
                // TODO changing SXLEN and UXLEN is not yet supported.
                // uxl = (XlenMode)((uxlMask & value) >> uxlShift);
                // sxl = (XlenMode)((sxlMask & value) >> sxlShift);
            }
            mprv = mprvMask & value;
            tvm = tvmMask & value;
            tw = twMask & value;
            tsr = tsrMask & value;
        }
    }

    template <typename XLEN_t, PrivilegeMode viewPrivilege>
    inline XLEN_t Read() {
        XLEN_t value = 0;
        value |= uie ? uieMask : 0;
        value |= upie ? upieMask : 0;
        if constexpr (viewPrivilege != PrivilegeMode::User) {
            value |= sie ? sieMask : 0;
            value |= spie ? spieMask : 0;
            value |= spp << sppShift;
            value |= fs << fsShift;
            value |= xs << xsShift;
            value |= sum ? sumMask : 0;
            value |= mxr ? mxrMask : 0;
            if constexpr (std::is_same<XLEN_t, __uint32_t>()) {
                value |= sd ? sdMask32 : 0;
            } else {
                value |= sd ? sdMask64 : 0;
            } // TODO 128
        }
        if constexpr (viewPrivilege == PrivilegeMode::Machine) {
            value |= mie ? mieMask : 0;
            value |= mpie ? mpieMask : 0;
            value |= mpp << mppShift;
            if constexpr (!std::is_same<XLEN_t, __uint32_t>()) {
                value |= (__uint64_t)(uxl) << uxlShift;
                value |= (__uint64_t)(sxl) << sxlShift;
            }
            value |= mprv ? mprvMask : 0;
            value |= tvm ? tvmMask : 0;
            value |= tw ? twMask : 0;
            value |= tsr ? tsrMask : 0;
        }
        return value;
    }

    template <typename MXLEN_t>
    inline void Reset() {
        mie = false;
        sie = false;
        uie = false;
        mpie = false;
        spie = false;
        upie = false;
        mprv = false;
        mpp = PrivilegeMode::Machine;
        spp = PrivilegeMode::Machine;
        fs = FloatingPointState::Off;
        xs = ExtensionState::AllOff;
        mprv = false;
        sum = false;
        mxr = false;
        tvm = false;
        tw = false;
        tsr = false;
        sxl = xlenTypeToMode<MXLEN_t>();
        uxl = xlenTypeToMode<MXLEN_t>();
        sd = false;
    }
};

struct interruptReg {

    bool usi, ssi, msi, uti, sti, mti, uei, sei, mei;

    template<typename XLEN_t, PrivilegeMode viewPrivilege>
    void Write(XLEN_t value) {
        usi = usiMask & value;
        uti = utiMask & value;
        uei = ueiMask & value;
        if constexpr (viewPrivilege != PrivilegeMode::User) {
            ssi = ssiMask & value;
            sti = stiMask & value;
            sei = seiMask & value;
        }
        if constexpr (viewPrivilege == PrivilegeMode::Machine) {
            msi = msiMask & value;
            mti = mtiMask & value;
            mei = meiMask & value;
        }
    }

    template<typename XLEN_t, PrivilegeMode viewPrivilege>
    XLEN_t Read() {
        XLEN_t value = 0;
        value |= usi ? usiMask : 0;
        value |= uti ? utiMask : 0;
        value |= uei ? ueiMask : 0;
        if constexpr (viewPrivilege != PrivilegeMode::User) {
            value |= ssi ? ssiMask : 0;
            value |= sti ? stiMask : 0;
            value |= sei ? seiMask : 0;
        }
        if constexpr (viewPrivilege == PrivilegeMode::Machine) {
            value |= msi ? msiMask : 0;
            value |= mti ? mtiMask : 0;
            value |= mei ? meiMask : 0;
        }
        return value;
    }

    void Reset() {
        usi = false;
        ssi = false;
        msi = false;
        uti = false;
        sti = false;
        mti = false;
        uei = false;
        sei = false;
        mei = false;
    }
};

template<typename XLEN_t>
struct tvecReg {
    XLEN_t base;
    tvecMode mode;
    void Reset() {
        base = 0;
        mode = tvecMode::Direct;
    }

    void Write(XLEN_t value) {
        base = value & (~(XLEN_t)0 << 2);
        mode = (RISCV::tvecMode)(value & 0x3);
    }

    XLEN_t Read() {
        return ((XLEN_t)base) | ((XLEN_t)mode);
    }
};

template<typename XLEN_t>
struct causeReg {
    bool interrupt;
    TrapCause exceptionCode;

    void Reset() {
        interrupt = false;
        exceptionCode = TrapCause::NONE;
    }

    void Write(XLEN_t value) {
        if constexpr (std::is_same<XLEN_t, __uint32_t>()) {
            interrupt = value & ((XLEN_t)1 << 31);
            exceptionCode = (RISCV::TrapCause)(value & ~((XLEN_t)1 << 31));
        } else {
            interrupt = value & ((XLEN_t)1 << 63);
            exceptionCode = (RISCV::TrapCause)(value & ~((XLEN_t)1 << 63));
        }
    }

    XLEN_t Read() {
        XLEN_t value = exceptionCode;
        if (interrupt) {
            if constexpr (std::is_same<XLEN_t, __uint32_t>()) {
                value |= ((XLEN_t)1 << 31);
            } else {
                value |= ((XLEN_t)1 << 63);
            }
        }
        return value;
    }
};

template<typename XLEN_t>
struct satpReg {
    PagingMode pagingMode;
    XLEN_t ppn;
    XLEN_t asid;
    void Reset() {
        pagingMode = PagingMode::Bare;
        ppn = 0;
        asid = 0;
    }

    void Write(XLEN_t value) {
        if constexpr (!std::is_same<XLEN_t, __uint32_t>()) {
            pagingMode = (RISCV::PagingMode)((value >> 60) & 0x0f);
            asid = (value >> 44) & 0x0ffff;
            ppn = value & 0x0fff'ffff'ffff;
        } else {
            pagingMode = (RISCV::PagingMode)(value >> 31);
            asid = (value >> 22) & 0x1ff;
            ppn = value & 0x3f'ffff;
        }
    }

    XLEN_t Read() {
        if constexpr (std::is_same<XLEN_t, __uint32_t>()) {
            return ((pagingMode & 0x000001) << 31)|
                        (( asid & 0x0001ff) << 22)|
                        ((  ppn & 0x3fffff) << 00);
        } else {
            return (((__uint64_t)pagingMode & 0x0000000000f) << 60)|
                         (((__uint64_t)asid & 0x0000000ffff) << 44)|
                          (((__uint64_t)ppn & 0xfffffffffff) << 00);
        }
    }
};

struct fcsrReg {
    fpRoundingMode frm;
    struct {
        bool nx, uf, of, dz, nv;
    } fflags;
    void Reset() {
        frm = fpRoundingMode::RNE;
        fflags.nx = false;
        fflags.uf = false;
        fflags.of = false;
        fflags.dz = false;
        fflags.nv = false;
    }
    // TODO R/W
};

struct pmpEntry {
    bool r, w, x, locked;
    pmpAddressMode aMode;
    __uint64_t address;
} pmpentry[16];

} // namespace RISCV
