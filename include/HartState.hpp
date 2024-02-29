#pragma once

#include <cstdint>
#include <functional>

#include <RiscV.hpp>

enum class HartCallbackArgument {
    ChangedPrivilege,
    ChangedMISA,
    ChangedMSTATUS,
    ChangedSATP,
    RequestedIfence,
    RequestedVMfence,
    TookTrap
};

template<typename XLEN_t>
struct HartState {

public:

    // TODO, bug, MIP MIE MIDELEG MEDELEG are MXLEN bits wide, not XLEN... etc.
    // TODO Maybe move counters and such out of the struct for speed - not used
    // often, and it's a 24->23 mips perf impact to keep them here! Or pareidolia.
    // TODO, an experiment with actually good scientific stats comparing packed
    // bits vs. broken out fields for these registers. First wrap in accessors.

    XLEN_t pc;
    XLEN_t resetVector;

    XLEN_t regs[RISCV::NumRegs];
    RISCV::PrivilegeMode privilegeMode = RISCV::PrivilegeMode::Machine;
    RISCV::misaReg misa;
    RISCV::mstatusReg mstatus;
    RISCV::interruptReg mie, mip;
    RISCV::causeReg<XLEN_t> mcause, scause, ucause;
    RISCV::tvecReg<XLEN_t> mtvec, stvec, utvec;
    XLEN_t mepc, sepc, uepc;
    XLEN_t mtval, stval, utval;
    XLEN_t mscratch, sscratch, uscratch;
    XLEN_t mideleg, medeleg, sideleg, sedeleg; // TODO are these "interruptReg"?
    RISCV::satpReg<XLEN_t> satp;
    RISCV::fcsrReg fcsr; // TODO float regs
    // __uint64_t counters[32];
    // __uint32_t mcounteren;
    // __uint32_t scounteren;
    // __uint32_t mcountinhibit;
    // XLEN_t hpmevents[32];
    // RISCV::pmpEntry pmpentry[16];

    std::function<void(HartCallbackArgument)> implCallback;
    void emptyCallback(HartCallbackArgument arg) { return; }

    HartState(__uint32_t allSupportedExtensions)
        : misa(allSupportedExtensions){
        implCallback = std::bind(&HartState::emptyCallback, this, std::placeholders::_1);
        privilegeMode = RISCV::PrivilegeMode::Machine;
        // TODO just reset instead?
    }

    void Reset() {

        pc = resetVector;

        for (unsigned int i = 0; i < RISCV::NumRegs; i++) {
            regs[i] = (XLEN_t)0;
        }

        privilegeMode = RISCV::PrivilegeMode::Machine;
        misa.Reset<XLEN_t>();
        mstatus.Reset<XLEN_t>();
        mie.Reset();
        mip.Reset();
        mcause.Reset();
        scause.Reset();
        ucause.Reset();
        mtvec.Reset();
        stvec.Reset();
        utvec.Reset();
        mepc = 0;
        sepc = 0;
        uepc = 0;
        mtval = 0;
        stval = 0;
        utval = 0;
        mscratch = 0;
        sscratch = 0;
        uscratch = 0;
        mideleg = 0;
        medeleg = 0;
        sideleg = 0;
        sedeleg = 0;
        satp.Reset();
        fcsr.Reset();
    }

    // Note, I think that any hardwiring has to happen on notify, not in reg.

    template<bool Writing>
    inline void BankedCSR(RISCV::CSRAddress csrAddress, XLEN_t* value) {
        if (csrAddress >= RISCV::CSRAddress::MCYCLE &&
            csrAddress <= RISCV::CSRAddress::MHPMCOUNTER31) {
            // unsigned int counterID = csrAddress - RISCV::CSRAddress::CYCLE;
            // TODO XLEN truncation
            // TODO MTIME carveout
        }

        if (csrAddress >= RISCV::CSRAddress::MCYCLEH &&
            csrAddress <= RISCV::CSRAddress::MHPMCOUNTER31H) {
            // unsigned int counterID = csrAddress - RISCV::CSRAddress::CYCLEH;
            // TODO shifting
            // TODO MTIME carveout
        }

        if (csrAddress >= RISCV::CSRAddress::CYCLE &&
            csrAddress <= RISCV::CSRAddress::HPMCOUNTER31) {
            // unsigned int counterID = csrAddress - RISCV::CSRAddress::CYCLE;
            // TODO XLEN truncation
            // TODO counter passthrough
        }

        if (csrAddress >= RISCV::CSRAddress::CYCLEH &&
            csrAddress <= RISCV::CSRAddress::HPMCOUNTER31H) {
            // unsigned int counterID = csrAddress - RISCV::CSRAddress::CYCLEH;
            // TODO shifting
            // TODO counter passthrough
        }

        if (csrAddress >= RISCV::CSRAddress::MHPMEVENT3 &&
            csrAddress <= RISCV::CSRAddress::MHPMEVENT31) {
            // unsigned int counterID = csrAddress - RISCV::CSRAddress::MHPMEVENT3 + 3;
            // TODO events
        }

        if (csrAddress >= RISCV::CSRAddress::PMPADDR0 &&
            csrAddress <= RISCV::CSRAddress::PMPADDR15) {
            // unsigned int pmpEntryID = csrAddress - RISCV::CSRAddress::PMPADDR0;
            // TODO pmp address
        }

        if (csrAddress >= RISCV::CSRAddress::PMPCFG0 &&
            csrAddress <= RISCV::CSRAddress::PMPCFG3) {
            // TODO four pmp entries at once
        }
    }

    inline XLEN_t ReadCSR(RISCV::CSRAddress csrAddress) {
        switch (csrAddress) {
            case RISCV::CSRAddress::MISA: return misa.Read<XLEN_t>(); break;
            case RISCV::CSRAddress::SATP: return satp.Read(); break;
            case RISCV::CSRAddress::MSTATUS: return mstatus.Read<XLEN_t, RISCV::PrivilegeMode::Machine>(); break;
            case RISCV::CSRAddress::SSTATUS: return mstatus.Read<XLEN_t, RISCV::PrivilegeMode::Supervisor>(); break;
            case RISCV::CSRAddress::USTATUS: return mstatus.Read<XLEN_t, RISCV::PrivilegeMode::User>(); break;
            case RISCV::CSRAddress::MIE: return mie.Read<XLEN_t, RISCV::PrivilegeMode::Machine>(); break;
            case RISCV::CSRAddress::SIE: return mie.Read<XLEN_t, RISCV::PrivilegeMode::Supervisor>(); break;
            case RISCV::CSRAddress::UIE: return mie.Read<XLEN_t, RISCV::PrivilegeMode::User>(); break;
            case RISCV::CSRAddress::MIP: return mip.Read<XLEN_t, RISCV::PrivilegeMode::Machine>(); break;
            case RISCV::CSRAddress::SIP: return mip.Read<XLEN_t, RISCV::PrivilegeMode::Supervisor>(); break;
            case RISCV::CSRAddress::UIP: return mip.Read<XLEN_t, RISCV::PrivilegeMode::User>(); break;
            case RISCV::CSRAddress::MTVEC: return mtvec.Read(); break;
            case RISCV::CSRAddress::MSCRATCH: return mscratch; break;
            case RISCV::CSRAddress::MEPC: return mepc; break;
            case RISCV::CSRAddress::MCAUSE: return mcause.Read(); break;
            case RISCV::CSRAddress::MTVAL: return mtval; break;
            case RISCV::CSRAddress::MEDELEG: return medeleg; break;
            case RISCV::CSRAddress::MIDELEG: return mideleg; break;
            case RISCV::CSRAddress::STVEC: return stvec.Read(); break;
            case RISCV::CSRAddress::SSCRATCH: return sscratch; break;
            case RISCV::CSRAddress::SEPC: return sepc; break;
            case RISCV::CSRAddress::SCAUSE: return scause.Read(); break;
            case RISCV::CSRAddress::STVAL: return stval; break;
            case RISCV::CSRAddress::SEDELEG: return sedeleg; break;
            case RISCV::CSRAddress::SIDELEG: return sideleg; break;
            case RISCV::CSRAddress::UTVEC: return utvec.Read(); break;
            case RISCV::CSRAddress::USCRATCH: return uscratch; break;
            case RISCV::CSRAddress::UEPC: return uepc; break;
            case RISCV::CSRAddress::UCAUSE: return ucause.Read(); break;
            case RISCV::CSRAddress::UTVAL: return utval; break;
            case RISCV::CSRAddress::MHARTID: return 0; break; // TODO multihart, non-zero mhartid option.
            case RISCV::CSRAddress::MVENDORID: break;
            case RISCV::CSRAddress::MARCHID: break;
            case RISCV::CSRAddress::MIMPID: break;
            case RISCV::CSRAddress::FFLAGS: break;
            case RISCV::CSRAddress::FRM: break;
            case RISCV::CSRAddress::FCSR: break;
            case RISCV::CSRAddress::SCOUNTEREN: break;
            case RISCV::CSRAddress::MCOUNTEREN: break;
            case RISCV::CSRAddress::MCOUNTINHIBIT: break;
            case RISCV::CSRAddress::TSELECT: break;
            case RISCV::CSRAddress::TDATA1: break;
            case RISCV::CSRAddress::TDATA2: break;
            case RISCV::CSRAddress::TDATA3: break;
            case RISCV::CSRAddress::DCSR: break;
            case RISCV::CSRAddress::DPC: break;
            case RISCV::CSRAddress::DSCRATCH0: break;
            case RISCV::CSRAddress::DSCRATCH1: break;
            case RISCV::CSRAddress::INVALID_CSR: break;
            default:
                XLEN_t value;
                BankedCSR<false>(csrAddress, &value);
                return value;
                break;
        }
        // TODO error here
        return 0;
    }

    inline void WriteCSR(RISCV::CSRAddress csrAddress, XLEN_t value) {
        switch (csrAddress) {
            case RISCV::CSRAddress::MISA:
                misa.Write<XLEN_t>(value);
                implCallback(HartCallbackArgument::ChangedMISA);
                break;
            case RISCV::CSRAddress::SATP:
                satp.Write(value);
                implCallback(HartCallbackArgument::ChangedSATP);
                break;
            case RISCV::CSRAddress::MSTATUS:
                mstatus.Write<XLEN_t, RISCV::PrivilegeMode::Machine>(value);
                implCallback(HartCallbackArgument::ChangedMSTATUS);
                break;
            case RISCV::CSRAddress::SSTATUS:
                mstatus.Write<XLEN_t, RISCV::PrivilegeMode::Supervisor>(value);
                implCallback(HartCallbackArgument::ChangedMSTATUS);
                break;
            case RISCV::CSRAddress::USTATUS:
                mstatus.Write<XLEN_t, RISCV::PrivilegeMode::User>(value);
                implCallback(HartCallbackArgument::ChangedMSTATUS);
                break;
            case RISCV::CSRAddress::MIE: mie.Write<XLEN_t, RISCV::PrivilegeMode::Machine>(value); break;
            case RISCV::CSRAddress::SIE: mie.Write<XLEN_t, RISCV::PrivilegeMode::Supervisor>(value); break;
            case RISCV::CSRAddress::UIE: mie.Write<XLEN_t, RISCV::PrivilegeMode::User>(value); break;
            case RISCV::CSRAddress::MIP: mip.Write<XLEN_t, RISCV::PrivilegeMode::Machine>(value); break;
            case RISCV::CSRAddress::SIP: mip.Write<XLEN_t, RISCV::PrivilegeMode::Supervisor>(value); break;
            case RISCV::CSRAddress::UIP: mip.Write<XLEN_t, RISCV::PrivilegeMode::User>(value); break;
            case RISCV::CSRAddress::MTVEC: mtvec.Write(value); break;
            case RISCV::CSRAddress::MSCRATCH: mscratch = value; break;
            case RISCV::CSRAddress::MEPC: mepc = value; break;
            case RISCV::CSRAddress::MCAUSE: mcause.Write(value); break;
            case RISCV::CSRAddress::MTVAL: mtval = value; break;
            case RISCV::CSRAddress::MEDELEG: medeleg = value; break;
            case RISCV::CSRAddress::MIDELEG: mideleg = value; break;
            case RISCV::CSRAddress::STVEC: stvec.Write(value); break;
            case RISCV::CSRAddress::SSCRATCH: sscratch = value; break;
            case RISCV::CSRAddress::SEPC: sepc = value; break;
            case RISCV::CSRAddress::SCAUSE: scause.Write(value); break;
            case RISCV::CSRAddress::STVAL: stval = value; break;
            case RISCV::CSRAddress::SEDELEG: sedeleg = value; break;
            case RISCV::CSRAddress::SIDELEG: sideleg = value; break;
            case RISCV::CSRAddress::UTVEC: utvec.Write(value); break;
            case RISCV::CSRAddress::USCRATCH: uscratch = value; break;
            case RISCV::CSRAddress::UEPC: uepc = value; break;
            case RISCV::CSRAddress::UCAUSE: ucause.Write(value); break;
            case RISCV::CSRAddress::UTVAL: utval = value; break;
            case RISCV::CSRAddress::MHARTID: break; // TODO invalid? read the spec, what happens when you write to MHARTID
            case RISCV::CSRAddress::MVENDORID: break; // TODO all the others
            case RISCV::CSRAddress::MARCHID: break;
            case RISCV::CSRAddress::MIMPID: break;
            case RISCV::CSRAddress::FFLAGS: break;
            case RISCV::CSRAddress::FRM: break;
            case RISCV::CSRAddress::FCSR: break;
            case RISCV::CSRAddress::SCOUNTEREN: break;
            case RISCV::CSRAddress::MCOUNTEREN: break;
            case RISCV::CSRAddress::MCOUNTINHIBIT: break;
            case RISCV::CSRAddress::TSELECT: break;
            case RISCV::CSRAddress::TDATA1: break;
            case RISCV::CSRAddress::TDATA2: break;
            case RISCV::CSRAddress::TDATA3: break;
            case RISCV::CSRAddress::DCSR: break;
            case RISCV::CSRAddress::DPC: break;
            case RISCV::CSRAddress::DSCRATCH0: break;
            case RISCV::CSRAddress::DSCRATCH1: break;
            case RISCV::CSRAddress::INVALID_CSR: break;
            default:
                BankedCSR<true>(csrAddress, &value);
                break;
        }
        // TODO error here
    }

    inline void RaiseException(RISCV::TrapCause cause, XLEN_t tval) {

        // std::cout << "Raising exception: " << RISCV::trapName(false, cause) << std::endl;

        if (cause == RISCV::TrapCause::NONE) {
            return;
        }

        RISCV::PrivilegeMode targetPrivilege = RISCV::DestinedPrivilegeForCause<XLEN_t>(
            cause, medeleg, sedeleg, misa.extensions);
        TakeTrap<false>(cause, targetPrivilege, tval);

        implCallback(HartCallbackArgument::TookTrap);
    }

    inline void ServiceInterrupts() {

        XLEN_t interruptsForM = 0;
        XLEN_t interruptsForS = 0;
        XLEN_t interruptsForU = 0;

        // TODO do this without bits, just use raw values. Faster...
        XLEN_t mipBits = mip.Read<XLEN_t, RISCV::PrivilegeMode::Machine>();
        XLEN_t mieBits = mie.Read<XLEN_t, RISCV::PrivilegeMode::Machine>();

        for (unsigned int bit = 0; bit < 8*sizeof(XLEN_t); bit++) {

            // A deasserted or disabled interrupt is not serviceable
            if (!(mipBits & (1<<bit)) || !(mieBits & (1<<bit))) {
                continue;
            }

            // Figure out the destined privilege level for the interrupt
            RISCV::PrivilegeMode destinedPrivilege =
                RISCV::DestinedPrivilegeForCause<XLEN_t>(
                    (RISCV::TrapCause)bit, mideleg, sideleg, misa.extensions);

            // Set the interrupt's bit in the correct mask for its privilege
            if (destinedPrivilege == RISCV::PrivilegeMode::Machine) {
                interruptsForM |= 1<<bit;
            } else if (destinedPrivilege == RISCV::PrivilegeMode::Supervisor) {
                interruptsForS |= 1<<bit;
            } else if (destinedPrivilege == RISCV::PrivilegeMode::User) {
                interruptsForU |= 1<<bit;
            }
        }

        // Select the highest-privilege enabled non-empty interrupt vector that is at or higher than our own privilege, and
        RISCV::PrivilegeMode targetPrivilege;
        XLEN_t interruptsToService = 0;
        if (privilegeMode <= RISCV::PrivilegeMode::Machine && interruptsForM != 0 && mstatus.mie) {
            targetPrivilege = RISCV::PrivilegeMode::Machine;
            interruptsToService = interruptsForM;
        } else if (privilegeMode <= RISCV::PrivilegeMode::Supervisor && interruptsForS != 0 && mstatus.sie) {
            targetPrivilege = RISCV::PrivilegeMode::Supervisor;
            interruptsToService = interruptsForS;
        } else if (privilegeMode <= RISCV::PrivilegeMode::User && interruptsForU != 0 && mstatus.uie) {
            targetPrivilege = RISCV::PrivilegeMode::User;
            interruptsToService = interruptsForU;
        } else {
            return;
        }

        RISCV::TrapCause cause = RISCV::highestPriorityInterrupt(interruptsToService);
        TakeTrap<true>(cause, targetPrivilege, 0);
    }

    template<bool isInterrupt>
    inline void TakeTrap(RISCV::TrapCause cause, RISCV::PrivilegeMode targetPrivilege, XLEN_t tval) {

        if (targetPrivilege < privilegeMode) {
            targetPrivilege = privilegeMode;
        }

        // When a trap is taken from privilege mode y into privilege mode x,
        // xPIE is set to the value of xIE;
        // xIE is set to 0;
        // and xPP is set to y.

        // When a trap is delegated to a less-privileged mode x:
        // the xcause register is written with the trap cause,
        // the xepc register is written with the virtual address of the
        //     instruction that took the trap,
        // the xtval register is written with an exception-specific datum,
        // the xPP field of mstatus is written with the active privilege mode
        //     at the time of the trap,
        // the xPIE field of mstatus is written with the value of the xIE field
        //     at the time of the trap, and
        // the xIE field of mstatus is cleared.

        switch (targetPrivilege) {
        case RISCV::PrivilegeMode::Machine:
            mcause.exceptionCode = cause;
            mcause.interrupt = isInterrupt;
            mepc = pc;
            mtval = tval;
            mstatus.mpp = privilegeMode;
            mstatus.mpie = mstatus.mie;
            mstatus.mie = false;
            pc = mtvec.base;
            if (mtvec.mode == RISCV::tvecMode::Vectored && !isInterrupt) {
                pc += 4*mcause.exceptionCode;
            }
            break;
        case RISCV::PrivilegeMode::Supervisor:
            scause.exceptionCode = cause;
            scause.interrupt = isInterrupt;
            sepc = pc;
            stval = tval;
            mstatus.spp = privilegeMode;
            mstatus.spie = mstatus.sie;
            mstatus.sie = false;
            pc = stvec.base;
            if (stvec.mode == RISCV::tvecMode::Vectored && !isInterrupt) {
                pc += 4*scause.exceptionCode;
            }
            break;
        case RISCV::PrivilegeMode::User:
            ucause.exceptionCode = cause;
            ucause.interrupt = isInterrupt;
            uepc = pc;
            utval = tval;
            mstatus.upie = mstatus.uie;
            mstatus.uie = false;
            pc = utvec.base;
            if (utvec.mode == RISCV::tvecMode::Vectored && !isInterrupt) {
                pc += 4*ucause.exceptionCode;
            }
            break;
        default:
            // fatal("Trap destined for nonsense privilege mode."); // TODO
            break;
        }
        privilegeMode = targetPrivilege;
        implCallback(HartCallbackArgument::ChangedPrivilege);
    }

    // TODO move this to instruction code
    template<RISCV::PrivilegeMode trapPrivilege>
    inline void ReturnFromTrap() {

        // TODO verify the below logic against the following spec snippet:
        // To return after handling a trap, there are separate trap return instructions per privilege level: MRET, SRET,
        // and URET. MRET is always provided. SRET must be provided if supervisor mode is supported, and should raise an
        // illegal instruction exception otherwise. SRET should also raise an illegal instruction exception when TSR=1
        // in mstatus, as described in Section 3.1.6.4. URET is only provided if user-mode traps are supported, and
        // should raise an illegal instruction otherwise. An x RET instruction can be executed in privilege mode x or
        // higher, where executing a lower-privilege x RET instruction will pop the relevant lower-privilege interrupt
        // enable and privilege mode stack. In addition to manipulating the privilege stack as described in Section
        // 3.1.6.1, x RET sets the pc to the value stored in the x epc register.

        if constexpr (trapPrivilege == RISCV::PrivilegeMode::Machine) {
            mstatus.mie = mstatus.mpie;
            privilegeMode = mstatus.mpp;
            mstatus.mpie = true;
            if (RISCV::vectorHasExtension(misa.extensions, 'U')) {
                mstatus.mpp = RISCV::PrivilegeMode::User;
            } else {
                mstatus.mpp = RISCV::PrivilegeMode::Machine;
            }
            pc = mepc;
        } else if constexpr (trapPrivilege == RISCV::PrivilegeMode::Supervisor) {
            mstatus.sie = mstatus.spie;
            privilegeMode = mstatus.spp;
            mstatus.spie = true;
            if (RISCV::vectorHasExtension(misa.extensions, 'U')) {
                mstatus.spp = RISCV::PrivilegeMode::User;
            } else {
                mstatus.spp = RISCV::PrivilegeMode::Machine;
            }
            pc = sepc;
        } else if constexpr (trapPrivilege == RISCV::PrivilegeMode::User) {
            mstatus.uie = mstatus.upie;
            mstatus.upie = true;
            pc = uepc;
        } else {
            // fatal("Return from nonsense-privilege-mode trap"); // TODO
        }

        implCallback(HartCallbackArgument::ChangedPrivilege);
    }

};
