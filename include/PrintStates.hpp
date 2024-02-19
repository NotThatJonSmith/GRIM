#pragma once

#include <iostream>
#include <iomanip>
#include <HartState.hpp>
#include <RiscV.hpp>
#include <RiscVDecoder.hpp>

template<typename XLEN_t>
void PrintArchDetails(HartState<XLEN_t>* state, std::ostream* out) {
    (*out) << "Details:" << std::endl;
    (*out) << "| misa: rv" << RISCV::xlenModeName(state->misa.mxlen)
           << RISCV::extensionsToString(state->misa.extensions)
           << std::endl;
    (*out) << "| Privilege=" << RISCV::privilegeModeName(state->privilegeMode)
           << std::endl;
    (*out) << "| mstatus: "
           << "mprv=" << (state->mstatus.mprv ? "1 " : "0 ")
           << "sum=" << (state->mstatus.sum ? "1 " : "0 ")
           << "tvm=" << (state->mstatus.tvm ? "1 " : "0 ")
           << "tw=" << (state->mstatus.tw ? "1 " : "0 ")
           << "tsr=" << (state->mstatus.tsr ? "1 " : "0 ")
           << "sd=" << (state->mstatus.sd ? "1 " : "0 ")
           << "fs=" << RISCV::floatingPointStateName(state->mstatus.fs) << " "
           << "xs=" << RISCV::extensionStateName(state->mstatus.xs) << " "
           << "sxl=" << RISCV::xlenModeName(state->mstatus.sxl) << " "
           << "uxl=" << RISCV::xlenModeName(state->mstatus.uxl)
           << std::endl << "|          "
           << "mie=" << (state->mstatus.mie ? "1 " : "0 ")
           << "mpie=" << (state->mstatus.mpie ? "1 " : "0 ")
           << "mpp=" << RISCV::privilegeModeName(state->mstatus.mpp) << " "
           << "sie=" << (state->mstatus.sie ? "1 " : "0 ")
           << "spie=" << (state->mstatus.spie ? "1 " : "0 ")
           << "spp=" << RISCV::privilegeModeName(state->mstatus.spp) << " "
           << "uie=" << (state->mstatus.uie ? "1 " : "0 ")
           << "upie=" << (state->mstatus.upie ? "1 " : "0 ")
           << std::endl;
    (*out) << "| satp: mode=" << RISCV::pagingModeName(state->satp.pagingMode)
           << ", ppn=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->satp.ppn << ", asid=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->satp.asid
           << std::endl;
    (*out) << "| mie=[ "
           << (state->mie.mei ? "mei " : "")
           << (state->mie.msi ? "msi " : "")
           << (state->mie.mti ? "mti " : "")
           << (state->mie.sei ? "sei " : "")
           << (state->mie.ssi ? "ssi " : "")
           << (state->mie.sti ? "sti " : "")
           << (state->mie.uei ? "uei " : "")
           << (state->mie.usi ? "usi " : "")
           << (state->mie.uti ? "uti " : "")
           << "] mip=[ "
           << (state->mip.mei ? (!state->mie.mei ? "*mei " : "mei ") : "")
           << (state->mip.msi ? (!state->mie.msi ? "*msi " : "msi ") : "")
           << (state->mip.mti ? (!state->mie.mti ? "*mti " : "mti ") : "")
           << (state->mip.sei ? (!state->mie.sei ? "*sei " : "sei ") : "")
           << (state->mip.ssi ? (!state->mie.ssi ? "*ssi " : "ssi ") : "")
           << (state->mip.sti ? (!state->mie.sti ? "*sti " : "sti ") : "")
           << (state->mip.uei ? (!state->mie.uei ? "*uei " : "uei ") : "")
           << (state->mip.usi ? (!state->mie.usi ? "*usi " : "usi ") : "")
           << (state->mip.uti ? (!state->mie.uti ? "*uti " : "uti ") : "")
           << "]" << std::endl;
    (*out) << "| mideleg=[ "
           << ((state->mideleg & RISCV::meiMask) ? "mei " : "")
           << ((state->mideleg & RISCV::msiMask) ? "msi " : "")
           << ((state->mideleg & RISCV::mtiMask) ? "mti " : "")
           << ((state->mideleg & RISCV::seiMask) ? "sei " : "")
           << ((state->mideleg & RISCV::ssiMask) ? "ssi " : "")
           << ((state->mideleg & RISCV::stiMask) ? "sti " : "")
           << ((state->mideleg & RISCV::ueiMask) ? "uei " : "")
           << ((state->mideleg & RISCV::usiMask) ? "usi " : "")
           << ((state->mideleg & RISCV::utiMask) ? "uti " : "")
           << "] sideleg=[ "
           << ((state->sideleg & RISCV::meiMask) ? "mei " : "")
           << ((state->sideleg & RISCV::msiMask) ? "msi " : "")
           << ((state->sideleg & RISCV::mtiMask) ? "mti " : "")
           << ((state->sideleg & RISCV::seiMask) ? "sei " : "")
           << ((state->sideleg & RISCV::ssiMask) ? "ssi " : "")
           << ((state->sideleg & RISCV::stiMask) ? "sti " : "")
           << ((state->sideleg & RISCV::ueiMask) ? "uei " : "")
           << ((state->sideleg & RISCV::usiMask) ? "usi " : "")
           << ((state->sideleg & RISCV::utiMask) ? "uti " : "")
           << "]" << std::endl;
    (*out) << "| medeleg=[ "
           << "TODO "
           << "] sedeleg=[ "
           << "TODO "
           << "]" << std::endl;
    (*out) << "| mtval=0x"
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->mtval;
    (*out) << " mscratch=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->mscratch;
    (*out) << " mepc=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->mepc
           << " mtvec=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->mtvec.base
           << " (" << RISCV::tvecModeName(state->mtvec.mode) << ")"
           << " mcause=" << RISCV::trapName(state->mcause.interrupt, state->mcause.exceptionCode)
           << std::endl;
    (*out) << "| stval=0x"
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->stval;
    (*out) << " sscratch=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->sscratch;
    (*out) << " sepc=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->sepc
           << " stvec=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->stvec.base
           << " (" << RISCV::tvecModeName(state->stvec.mode) << ")"
           << " scause=" << RISCV::trapName(state->scause.interrupt, state->scause.exceptionCode)
           << std::endl;
    (*out) << "| utval=0x"
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->utval;
    (*out) << " uscratch=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->uscratch;
    (*out) << " uepc=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->uepc
           << " utvec=0x" 
           << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
           << state->utvec.base
           << " (" << RISCV::tvecModeName(state->utvec.mode) << ")"
           << " ucause=" << RISCV::trapName(state->ucause.interrupt, state->ucause.exceptionCode)
           << std::endl;
    // TODO FP state
    // TODO counters and events
    // TODO PMP
}

template<>
void PrintArchDetails<__uint128_t>(HartState<__uint128_t>* state, std::ostream* out) {
    (*out) << "128-bit prints not supported";
}

template<typename XLEN_t>
void PrintRegisters(HartState<XLEN_t>* state, std::ostream* out, bool abi, unsigned int regsPerLine) {
    (*out) << "Registers:" << std::endl;
    for (unsigned int i = 0; i < 32; i++) {
        if (i % regsPerLine == 0) {
            (*out) << "| ";
        }
        if (abi) {
            (*out) << std::setfill(' ') << std::setw(4)
                << RISCV::registerAbiNames[i] << ": "
                << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                << state->regs[i];
        } else {
            (*out) << std::dec << std::setfill(' ') << std::setw(2) << i << ": "
                << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                << state->regs[i];
        }
        if ((i+1) % regsPerLine == 0) {
            std::cout << std::endl;
        } else {
            std::cout << "  ";
        }
    }
}

template<> 
void PrintRegisters<__uint128_t>(HartState<__uint128_t>* state, std::ostream* out, bool abi, unsigned int regsPerLine) {
    (*out) << "128-bit prints not supported";
}
