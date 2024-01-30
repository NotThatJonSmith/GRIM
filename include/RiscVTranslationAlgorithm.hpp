#pragma once

#include <RiscV.hpp>
#include <Swizzle.hpp>
#include <Translation.hpp>
#include <Transactor.hpp>
#include <HartState.hpp>

template<typename XLEN_t, CASK::AccessType accessType>
inline Translation<XLEN_t> PageFault(XLEN_t virt_addr) {
    if constexpr (accessType == CASK::AccessType::R) {
        return { virt_addr, 0, 0, 0, RISCV::TrapCause::LOAD_PAGE_FAULT };
    } else if constexpr (accessType == CASK::AccessType::W) {
        return { virt_addr, 0, 0, 0, RISCV::TrapCause::STORE_AMO_PAGE_FAULT };
    } else {
        return { virt_addr, 0, 0, 0, RISCV::TrapCause::INSTRUCTION_PAGE_FAULT };
    }
}

template<typename XLEN_t, CASK::AccessType accessType>
static inline Translation<XLEN_t> TranslationAlgorithm(
        XLEN_t virt_addr,
        Transactor<XLEN_t>* transactor,
        XLEN_t root_ppn,
        RISCV::PagingMode currentPagingMode,
        RISCV::PrivilegeMode translationPrivilege,
        bool mxrBit,
        bool sumBit
    ) {
    
    unsigned int i = 0;
    XLEN_t vpn[4];
    XLEN_t ptesize = 0;
    XLEN_t pagesize = 0;

    if (translationPrivilege == RISCV::PrivilegeMode::Machine ||
        currentPagingMode == RISCV::PagingMode::Bare) {
        return { virt_addr, virt_addr, (XLEN_t)0, (XLEN_t)~0, RISCV::TrapCause::NONE };
    } else if (currentPagingMode == RISCV::PagingMode::Sv32) {
        i = 1;
        vpn[1] = swizzle<XLEN_t, ExtendBits::Zero, 31, 22>(virt_addr);
        vpn[0] = swizzle<XLEN_t, ExtendBits::Zero, 21, 12>(virt_addr);
        ptesize = 4;
        pagesize = 1 << 12;
    } else if (currentPagingMode == RISCV::PagingMode::Sv39) {
        i = 2;
        vpn[2] = swizzle<XLEN_t, ExtendBits::Zero, 38, 30>(virt_addr);
        vpn[1] = swizzle<XLEN_t, ExtendBits::Zero, 29, 21>(virt_addr);
        vpn[0] = swizzle<XLEN_t, ExtendBits::Zero, 20, 12>(virt_addr);
        ptesize = 8;
        pagesize = 1 << 12;
    } else if (currentPagingMode == RISCV::PagingMode::Sv48) {
        i = 3;
        vpn[3] = swizzle<XLEN_t, ExtendBits::Zero, 47, 39>(virt_addr);
        vpn[2] = swizzle<XLEN_t, ExtendBits::Zero, 38, 30>(virt_addr);
        vpn[1] = swizzle<XLEN_t, ExtendBits::Zero, 29, 21>(virt_addr);
        vpn[0] = swizzle<XLEN_t, ExtendBits::Zero, 20, 12>(virt_addr);
        ptesize = 8;
        pagesize = 1 << 12;
    } // TODO wider paging modes

    XLEN_t a = root_ppn * pagesize;
    XLEN_t pte = 0; // TODO PTE should be Sv** determined, not XLEN_t sized...

    while (true) {

        XLEN_t pteaddr = a + (vpn[i] * ptesize);
        transactor->Read(pteaddr, ptesize, (char*)&pte);

        // TODO PMA & PMP checks

        if ( !(pte & RISCV::PTEBit::V) ||
            (!(pte & RISCV::PTEBit::R) && (pte & RISCV::PTEBit::W))) {
            return PageFault<XLEN_t, accessType>(virt_addr);
        }

        if ((pte & RISCV::PTEBit::R) || (pte & RISCV::PTEBit::X)) {
            break;
        }

        if (i == 0) {
            return PageFault<XLEN_t, accessType>(virt_addr);
        }
        i = i-1;

        if (currentPagingMode == RISCV::PagingMode::Sv39 ||
            currentPagingMode == RISCV::PagingMode::Sv48) {
            a = swizzle<XLEN_t, ExtendBits::Zero, 53, 10>(pte) * pagesize;
        } else {
            a = swizzle<XLEN_t, ExtendBits::Zero, 31, 10>(pte) * pagesize;
        }

    }

    // TODO this could be constexpr if not for pedantry wrt mxrBit
    if (accessType == CASK::AccessType::R) {
        if (!(pte & RISCV::PTEBit::R) &&
            !((pte & RISCV::PTEBit::X) && mxrBit)) {
            return PageFault<XLEN_t, accessType>(virt_addr);
        }
    } else if (accessType == CASK::AccessType::W) {
        if (!(pte & RISCV::PTEBit::W)) {
            return PageFault<XLEN_t, accessType>(virt_addr);
        }
    } else {
        if (!(pte & RISCV::PTEBit::X)) {
            return PageFault<XLEN_t, accessType>(virt_addr);
        }
    }

    if (translationPrivilege == RISCV::PrivilegeMode::User &&
        !(pte & RISCV::PTEBit::U)) {
        return PageFault<XLEN_t, accessType>(virt_addr);
    }

    if (translationPrivilege == RISCV::PrivilegeMode::Supervisor &&
        (pte & RISCV::PTEBit::U) &&
        !sumBit) {
        return PageFault<XLEN_t, accessType>(virt_addr);
    }

    XLEN_t ppn[4];
    if (currentPagingMode == RISCV::PagingMode::Sv32) {
        ppn[1] = swizzle<XLEN_t, ExtendBits::Zero, 31, 20>(pte);
        ppn[0] = swizzle<XLEN_t, ExtendBits::Zero, 19, 10>(pte);
    } else if (currentPagingMode == RISCV::PagingMode::Sv39) {
        ppn[2] = swizzle<XLEN_t, ExtendBits::Zero, 53, 28>(pte);
        ppn[1] = swizzle<XLEN_t, ExtendBits::Zero, 27, 19>(pte);
        ppn[0] = swizzle<XLEN_t, ExtendBits::Zero, 18, 10>(pte);
    } else if (currentPagingMode == RISCV::PagingMode::Sv48) {
        ppn[3] = swizzle<XLEN_t, ExtendBits::Zero, 53, 37>(pte);
        ppn[2] = swizzle<XLEN_t, ExtendBits::Zero, 36, 28>(pte);
        ppn[1] = swizzle<XLEN_t, ExtendBits::Zero, 27, 19>(pte);
        ppn[0] = swizzle<XLEN_t, ExtendBits::Zero, 18, 10>(pte);
    } // TODO all Sv** modes

    if (i > 0) {
        for (unsigned int lv = 0; lv < i; lv++) {
            if (ppn[lv] != 0) {
                return PageFault<XLEN_t, accessType>(virt_addr);
            }
        }
    }

    // TODO the alternative A/D bit scheme mentioned in the spec.

    if (!(pte & RISCV::PTEBit::A)) {
        return PageFault<XLEN_t, accessType>(virt_addr);
    }

    if constexpr (accessType == CASK::AccessType::W) {
        if (!(pte & RISCV::PTEBit::D)) {
            return PageFault<XLEN_t, accessType>(virt_addr);
        }
    }

    // TODO this is where we can determine a better validThrough.
    // Determine how big the page is given how super this page is :P

    for (; i > 0; i--) {
        ppn[i-1] = vpn[i-1];
    }

    XLEN_t page_offset = swizzle<XLEN_t, ExtendBits::Zero, 11, 0>(virt_addr);
    XLEN_t phys_addr = 0;
    if (currentPagingMode == RISCV::PagingMode::Sv32) {
        phys_addr = (ppn[1] << 22) | (ppn[0] << 12) | page_offset;
    } else if (currentPagingMode == RISCV::PagingMode::Sv39) {
        phys_addr = (ppn[2] << 30) | (ppn[1] << 21) | (ppn[0] << 12) | page_offset;
    }/* else if (currentPagingMode == RISCV::PagingMode::Sv48) {
        phys_addr = (ppn[3] << 39) | (ppn[2] << 30) | (ppn[1] << 21) | (ppn[0] << 12) | page_offset;
    }
    */ // TODO we're temporarily breaking Sv48 because of this shift overflow warning-as-error. There is a clean way...
    XLEN_t virt_page_start = virt_addr & ~(pagesize - 1);
    XLEN_t virt_valid_through = virt_addr | (pagesize - 1);
    // TODO this is inefficient for superpages because it only uses the size of regular pages.
    return { virt_addr, phys_addr, virt_page_start, virt_valid_through, RISCV::TrapCause::NONE };
}
