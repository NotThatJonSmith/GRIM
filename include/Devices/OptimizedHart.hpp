#pragma once

#include <type_traits>
#include <cstdint>

#include <RiscVDecoder.hpp>

template<typename XLEN_t>
struct Translation {
    XLEN_t translated;
    XLEN_t virtPageStart;
    XLEN_t validThrough;
    RISCV::TrapCause generatedTrap;
};

// TODO accelerated WFI states?
template<typename XLEN_t>
class OptimizedHart {


    static constexpr bool print_physical_pc = true;
    static constexpr bool print_transactions = false;
    static constexpr bool icache_disabled = false;
    static constexpr bool memcache_disabled = false;
    static constexpr bool print_pagewalks = false && sizeof(XLEN_t) <= 8;

private:

    Device* target;

    static constexpr unsigned int cacheBits = 12;
    static constexpr unsigned int icacheBits = 12;
    static constexpr unsigned int fastLoopTicks = 1000;
    struct TranslationCacheEntry { char *hostPageStart; XLEN_t virtPageStart; XLEN_t validThrough; };
    TranslationCacheEntry cacheR[1 << cacheBits];
    TranslationCacheEntry cacheW[1 << cacheBits];
    TranslationCacheEntry cacheX[1 << cacheBits];
    struct ICacheEntry {
        XLEN_t full_pc;
        __uint32_t encoding = 0;
        DecodedInstruction<XLEN_t> instruction = nullptr;
    };
    ICacheEntry icache[1<<icacheBits];
    __uint32_t configured_extensions;
    RISCV::XlenMode configured_mxlen;
    std::array<DecodedInstruction<XLEN_t>, 1 << 20> uncompressed_inst_lut;
    std::array<DecodedInstruction<XLEN_t>, 1 << 16> compressed_inst_lut;

public:

    HartState<XLEN_t> state;

    OptimizedHart(Device* bus, __uint32_t maximalExtensions) :
        target(bus),
        configured_extensions(0),
        configured_mxlen(RISCV::XlenMode::XL128),
        state(maximalExtensions) {
        state.implCallback = std::bind(&OptimizedHart::Callback, this, std::placeholders::_1);
        // TODO callback for changing XLENs
        Reset();
    };

    inline unsigned int Tick() {
        for (unsigned int i = 0; i < fastLoopTicks; i++) {

            unsigned int icacheIndex = (state.pc >> 1) & ((1<<icacheBits)-1);
            if constexpr (!icache_disabled) {
                ICacheEntry *inst = &icache[icacheIndex];
                if (inst->full_pc == state.pc) [[ likely ]] {
                    inst->instruction(inst->encoding, this);
                    continue;
                }
            }
            __uint32_t encoding;
            if (!Transact<__uint32_t, AccessType::X>(state.pc, (char*)&encoding))
                continue;
            DecodedInstruction<XLEN_t> decoded = Decode(encoding);
            if constexpr (!icache_disabled)
                icache[icacheIndex] = { state.pc, encoding, decoded };
            decoded(encoding, this);
        }
        return fastLoopTicks;
    };

    unsigned int TickOnceAndPrintDisasm(std::ostream* disasm_pipe) {

        ICacheEntry *inst = &icache[(state.pc >> 1) & ((1<<icacheBits)-1)];
        XLEN_t print_pc = state.pc;
        if constexpr (!icache_disabled) {
            if (inst->full_pc == state.pc) [[ likely ]] {
                if constexpr (sizeof(XLEN_t) > 8) {
                    std::cout << "128 bit printing not supported" << std::endl;
                } else {
                    if constexpr (print_physical_pc) {
                        Translation<XLEN_t> fresh_translation = TranslationAlgorithm<AccessType::X>(state.pc, target);
                        if (fresh_translation.generatedTrap != RISCV::TrapCause::NONE) [[ unlikely ]] {
                            (*disasm_pipe) << "Translation fault that should not have happened" << std::endl;
                            exit(1);
                        }
                        print_pc = fresh_translation.translated;
                    }
                    (*disasm_pipe) << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                                << print_pc << ":\t"
                                << std::hex << std::setfill('0') << std::setw(sizeof(inst->encoding)*2)
                                << inst->encoding << "\t"
                                << std::dec;
                }
                Instruction<XLEN_t> dinstr = decode_instruction<XLEN_t>(inst->encoding, state.misa.extensions, state.misa.mxlen);
                dinstr.disassemblyFunction(inst->encoding, disasm_pipe);
                assert(inst->instruction == dinstr.executionFunction);
                inst->instruction(inst->encoding, this);
                return 1;
            }
        }

        __uint32_t encoding;
        if (!Transact<__uint32_t, AccessType::X>(state.pc, (char*)&encoding))
            return 1;

        if constexpr (print_physical_pc) {
            Translation<XLEN_t> fresh_translation = TranslationAlgorithm<AccessType::X>(state.pc, target);
            if (fresh_translation.generatedTrap != RISCV::TrapCause::NONE) [[ unlikely ]] {
                (*disasm_pipe) << "Translation fault that should not have happened" << std::endl;
                exit(1);
            }
            print_pc = fresh_translation.translated;
        }

        if constexpr (sizeof(XLEN_t) > 8) {
            std::cout << "128 bit printing not supported" << std::endl;
        } else {
            (*disasm_pipe) << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                           << print_pc << ":\t"
                           << std::hex << std::setfill('0') << std::setw(sizeof(encoding)*2)
                           << encoding << "\t"
                           << std::dec;
        }
        Instruction<XLEN_t> dinstr = decode_instruction<XLEN_t>(encoding, state.misa.extensions, state.misa.mxlen);
        dinstr.disassemblyFunction(encoding, disasm_pipe);
        DecodedInstruction<XLEN_t> decoded = Decode(encoding);
        assert(decoded == dinstr.executionFunction);
        if constexpr (!icache_disabled)
            icache[(state.pc >> 1) & ((1<<icacheBits)-1)] = { state.pc, encoding, decoded };
        decoded(encoding, this);
        return 1;
    };

    void ReconfigureDecodeTables() {
        // Skip reconfiguration when nothing has changed.
        if (state.misa.extensions == configured_extensions &&
            state.misa.mxlen == configured_mxlen) {
            return;
        }
        configured_extensions = state.misa.extensions;
        configured_mxlen = state.misa.mxlen;
        for (__uint32_t packed_instruction = 0; packed_instruction < (1<<20); packed_instruction++) {
            __uint32_t unpacked_encoding = 0b11 |
                ((0b00000000000000011111 & packed_instruction) << 2) |
                ((0b00000000000011100000 & packed_instruction) << 7) |
                ((0b11111111111100000000 & packed_instruction) << 12);
            uncompressed_inst_lut[packed_instruction] = decode_instruction<XLEN_t>(unpacked_encoding, state.misa.extensions, state.misa.mxlen).executionFunction;
        }
        for (__uint32_t encoded = 0; encoded < 1<<16; encoded++) {
            if ((encoded & 0b11) != 0b11) {
                compressed_inst_lut[encoded] = decode_instruction<XLEN_t>(encoded, state.misa.extensions, state.misa.mxlen).executionFunction;
            }
        }
    }

    inline void Reset() {
        state.Reset();
        memset(cacheR, 0, sizeof(cacheR));
        memset(cacheW, 0, sizeof(cacheW));
        memset(cacheX, 0, sizeof(cacheX));
        ReconfigureDecodeTables();
        memset(icache, 0, sizeof(icache));
    };

    template <typename MEM_TYPE_t, AccessType accessType, bool fault, bool print_translated>
    inline void PrintTransaction(XLEN_t startAddress, XLEN_t translated, char* bytes) {
        if constexpr (sizeof(MEM_TYPE_t) >= 16)
            return;
        constexpr unsigned int alignBits = 4;

        if constexpr (print_translated) {
            std::cout << (accessType == AccessType::R ? "Read" : (accessType == AccessType::W ? "Write" : "Fetch"))
                                << std::dec << std::setw(0) << sizeof(MEM_TYPE_t)*8
                                << ": 0x" << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                                << (__uint64_t)startAddress
                                << " => 0x" << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                                << (__uint64_t)translated
                                << std::endl;
        } else {
            std::cout  << (accessType == AccessType::R ? "Read" : (accessType == AccessType::W ? "Write" : "Fetch"))
                              << std::dec << std::setw(0) << sizeof(MEM_TYPE_t)*8
                              << ": 0x" << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                              << (__uint64_t)startAddress
                              << std::endl;
        }

        std::cout  << "| ";
        XLEN_t bytesPerRow = 1 << alignBits;
        XLEN_t mask = ~((1 << alignBits) - 1);
        XLEN_t firstRowStartAddress = startAddress & mask;
        for (XLEN_t rowStartAddress = firstRowStartAddress; rowStartAddress < startAddress + sizeof(MEM_TYPE_t); rowStartAddress += bytesPerRow) {
            for (XLEN_t columnIndex = 0; columnIndex < bytesPerRow; columnIndex++) {
                XLEN_t rowIndex = (rowStartAddress - firstRowStartAddress) / bytesPerRow;
                if (columnIndex == 0) {
                    if (rowIndex != 0)
                        std::cout  << std::endl << "| ";
                    std::cout  << std::hex << std::setfill('0')
                                        << std::setw(sizeof(XLEN_t)*2)
                                        << (__uint64_t)rowStartAddress << ": ";
                }
                XLEN_t simAddressOfByte = rowStartAddress + columnIndex;
                if (simAddressOfByte < startAddress || simAddressOfByte >= startAddress + sizeof(MEM_TYPE_t))
                    std::cout << "** ";
                else if (fault)
                    std::cout << "?? ";
                else
                    std::cout << std::hex << std::setfill('0') << std::setw(2)
                                        << ((unsigned int)bytes[simAddressOfByte - startAddress] & 0xff)
                                        << " ";
            }
        }
        std::cout << std::endl;
    }

    template <typename MEM_TYPE_t, AccessType accessType>
    inline bool Transact(XLEN_t startAddress, char* buf) {
        TranslationCacheEntry* cache = accessType == AccessType::R ? cacheR : (accessType == AccessType::W ? cacheW : cacheX);
        XLEN_t index = (startAddress >> 12) & ((1 << cacheBits) - 1); // TODO assumes 4k pages, need flex for supers
        if constexpr (!memcache_disabled) {
            if ((cache[index].virtPageStart >> 12 == startAddress >> 12) && (cache[index].hostPageStart != nullptr)) [[ likely ]] {
                char *hostAddress = cache[index].hostPageStart + startAddress - cache[index].virtPageStart;
                if constexpr (accessType == AccessType::W) {
                    *(MEM_TYPE_t*)hostAddress = *(MEM_TYPE_t*)buf;
                    // memcpy(hostAddress, buf, sizeof(MEM_TYPE_t));
                } else {
                    *(MEM_TYPE_t*)buf = *(MEM_TYPE_t*)hostAddress;
                    // memcpy(buf, hostAddress, sizeof(MEM_TYPE_t));
                }
                if constexpr (print_transactions)
                    PrintTransaction<MEM_TYPE_t, accessType, false, false>(startAddress, 0, buf);
                return true;
            }
        }
        Translation<XLEN_t> fresh_translation = TranslationAlgorithm<accessType>(startAddress, target);
        if (fresh_translation.generatedTrap != RISCV::TrapCause::NONE) [[ unlikely ]] {
            state.RaiseException(fresh_translation.generatedTrap, startAddress);
            if constexpr (print_transactions)
                PrintTransaction<MEM_TYPE_t, accessType, true, true>(startAddress, fresh_translation.translated, buf);
            return false;
        }

        /*TODO what about errant Transaction?*/
        // TODO - Transact should be in 1,2,4,8,16 bytes, not in 4,8,16. Device class needs to change. Would be nice to
        //        collapse it all into one transact function for everything for every device.
        target->template Transact<XLEN_t, accessType>(fresh_translation.translated, sizeof(MEM_TYPE_t), buf);
        if constexpr (!memcache_disabled) {
            if (target->hint) {
                XLEN_t offset = startAddress - fresh_translation.virtPageStart;
                cache[index].hostPageStart = (char*)target->hint - offset;
                cache[index].virtPageStart = fresh_translation.virtPageStart;
                cache[index].validThrough = fresh_translation.validThrough;
                // TODO, sidechannel for TranslationAlgorithm to give us a whole RWX result
                if constexpr (accessType == AccessType::R) {
                    Translation<XLEN_t> mirrorTranslation = TranslationAlgorithm<AccessType::W>(startAddress, target);
                    if (mirrorTranslation.generatedTrap == RISCV::TrapCause::NONE) {
                        cacheW[index].hostPageStart = cache[index].hostPageStart;
                        cacheW[index].virtPageStart = cache[index].virtPageStart;
                        cacheW[index].validThrough = cache[index].validThrough;
                    }
                } else if constexpr (accessType == AccessType::W) {
                    Translation<XLEN_t> mirrorTranslation = TranslationAlgorithm<AccessType::R>(startAddress, target);
                    if (mirrorTranslation.generatedTrap == RISCV::TrapCause::NONE) {
                        cacheR[index].hostPageStart = cache[index].hostPageStart;
                        cacheR[index].virtPageStart = cache[index].virtPageStart;
                        cacheR[index].validThrough = cache[index].validThrough;
                    }
                }
            }
        }
        if constexpr (print_transactions)
            PrintTransaction<MEM_TYPE_t, accessType, false, true>(startAddress, fresh_translation.translated, buf);
        return true;
    }

private:

    template<AccessType accessType>
    Translation<XLEN_t> TranslationAlgorithm(XLEN_t va, Device* mem) {
        if (state.satp.pagingMode == RISCV::PagingMode::Bare)
            return TranslationAlgorithm<RISCV::PagingMode::Bare, accessType>(va, mem);
        if (state.satp.pagingMode == RISCV::PagingMode::Sv32)
            return TranslationAlgorithm<RISCV::PagingMode::Sv32, accessType>(va, mem);
        if (state.satp.pagingMode == RISCV::PagingMode::Sv39)
            return TranslationAlgorithm<RISCV::PagingMode::Sv39, accessType>(va, mem);
        if (state.satp.pagingMode == RISCV::PagingMode::Sv48)
            return TranslationAlgorithm<RISCV::PagingMode::Sv48, accessType>(va, mem);
        return { }; // TODO something fatal?
    }

    // What's happening right now is that pk64 takes a routing Instruction Page Fault
    // Then, when saving the hart context onto the stack, the stack pointer ends
    // up pointing to memory near the very page table we're executing in
    // and then the context's SP reg smashes our current PTE.
    // This feels.... really unlikely. There's something weird going on.
    template<RISCV::PagingMode pagingMode, AccessType accessType>
    Translation<XLEN_t> TranslationAlgorithm(XLEN_t va, Device* mem) {
        RISCV::PrivilegeMode translationPrivilege = state.mstatus.mprv ? state.mstatus.mpp : state.privilegeMode;
        if (translationPrivilege == RISCV::PrivilegeMode::Machine || pagingMode == RISCV::PagingMode::Bare)
            return { va, (XLEN_t)0, (XLEN_t)~0, RISCV::TrapCause::NONE };
        static constexpr Translation<XLEN_t> page_fault =
            { 0, 0, 0, accessType == AccessType::R ? RISCV::TrapCause::LOAD_PAGE_FAULT :
                      (accessType == AccessType::W ? RISCV::TrapCause::STORE_AMO_PAGE_FAULT : 
                                                     RISCV::TrapCause::INSTRUCTION_PAGE_FAULT) }; 
        XLEN_t vpn[4];
        constexpr XLEN_t ptesize = (pagingMode == RISCV::PagingMode::Sv32) ? 4 : 8;
        constexpr XLEN_t levels = (pagingMode == RISCV::PagingMode::Sv32) ? 2 : 
                                  (pagingMode == RISCV::PagingMode::Sv39) ? 3 : 4;
        constexpr XLEN_t pagesize = 1 << 12;

        if (pagingMode == RISCV::PagingMode::Sv32) {
            vpn[1] = swizzle<XLEN_t, ExtendBits::Zero, 31, 22>(va);
            vpn[0] = swizzle<XLEN_t, ExtendBits::Zero, 21, 12>(va);
        } else if (pagingMode == RISCV::PagingMode::Sv39) {
            vpn[2] = swizzle<XLEN_t, ExtendBits::Zero, 38, 30>(va);
            vpn[1] = swizzle<XLEN_t, ExtendBits::Zero, 29, 21>(va);
            vpn[0] = swizzle<XLEN_t, ExtendBits::Zero, 20, 12>(va);
        } else if (pagingMode == RISCV::PagingMode::Sv48) {
            vpn[3] = swizzle<XLEN_t, ExtendBits::Zero, 47, 39>(va);
            vpn[2] = swizzle<XLEN_t, ExtendBits::Zero, 38, 30>(va);
            vpn[1] = swizzle<XLEN_t, ExtendBits::Zero, 29, 21>(va);
            vpn[0] = swizzle<XLEN_t, ExtendBits::Zero, 20, 12>(va);
        }
        XLEN_t page_offset = swizzle<XLEN_t, ExtendBits::Zero, 11, 0>(va);

        XLEN_t a = state.satp.ppn * pagesize;
        unsigned int i = levels - 1;

        if constexpr (print_pagewalks)
            std::cout << "Translate: ";
        
        XLEN_t pte = 0; // TODO PTE should be Sv** determined, not XLEN_t sized...
        while (true) {

            XLEN_t pteaddr = a + (vpn[i] * ptesize);
            if constexpr (print_pagewalks) {
                std::cout << "0x"
                          << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                          << pteaddr << " => ";
            }
            mem->Read<XLEN_t>(pteaddr, ptesize, (char*)&pte);
            // TODO PMA & PMP checks
            if (!(pte & RISCV::PTEBit::V) || (!(pte & RISCV::PTEBit::R) && (pte & RISCV::PTEBit::W))) {
                if constexpr (print_pagewalks) {
                    std::cout << "Fault; Invalid PTE 0x"
                              << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                              << pte << std::endl;
                }
                return page_fault;
            }
            if ((pte & RISCV::PTEBit::R) || (pte & RISCV::PTEBit::X)) {
                if constexpr (print_pagewalks) {
                    std::cout << "Leaf PTE 0x"
                              << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                              << pte << std::endl;
                }
                break;
            }
            if (i == 0) {
                if constexpr (print_pagewalks) {
                    std::cout << "Fault; last level PTE is a pointer" << std::endl;
                }
                return page_fault;
            }
            i = i-1;
            if (pagingMode == RISCV::PagingMode::Sv39 || pagingMode == RISCV::PagingMode::Sv48)
                a = swizzle<XLEN_t, ExtendBits::Zero, 53, 10>(pte) * pagesize;
            else
                a = swizzle<XLEN_t, ExtendBits::Zero, 31, 10>(pte) * pagesize;
        }
        if constexpr (print_pagewalks) {
            std::cout << "| ";
        }
        if (accessType == AccessType::R && !(pte & RISCV::PTEBit::R) && !((pte & RISCV::PTEBit::X) && state.mstatus.mxr)) {
            if constexpr (print_pagewalks)
                std::cout << "Page fault: illegal read" << std::endl;
            return page_fault;
        }
        if (accessType == AccessType::W && !(pte & RISCV::PTEBit::W)) {
            if constexpr (print_pagewalks)
                std::cout << "Page fault: illegal write" << std::endl;
            return page_fault;
        }
        if (accessType == AccessType::X && !(pte & RISCV::PTEBit::X)) {
            if constexpr (print_pagewalks)
                std::cout << "Page fault: illegal fetch" << std::endl;
            return page_fault;
        }
        if (translationPrivilege == RISCV::PrivilegeMode::User && !(pte & RISCV::PTEBit::U)) {
            if constexpr (print_pagewalks)
                std::cout << "Page fault: disallowed for user mode" << std::endl;
            return page_fault;
        }
        if (translationPrivilege == RISCV::PrivilegeMode::Supervisor && (pte & RISCV::PTEBit::U) && !state.mstatus.sum) {
            if constexpr (print_pagewalks)
                std::cout << "Page fault: disallowed for supervisor mode" << std::endl;
            return page_fault;
        }
        XLEN_t ppn[4];
        if (pagingMode == RISCV::PagingMode::Sv32) {
            ppn[1] = swizzle<XLEN_t, ExtendBits::Zero, 31, 20>(pte);
            ppn[0] = swizzle<XLEN_t, ExtendBits::Zero, 19, 10>(pte);
        } else if (pagingMode == RISCV::PagingMode::Sv39) {
            ppn[2] = swizzle<XLEN_t, ExtendBits::Zero, 53, 28>(pte);
            ppn[1] = swizzle<XLEN_t, ExtendBits::Zero, 27, 19>(pte);
            ppn[0] = swizzle<XLEN_t, ExtendBits::Zero, 18, 10>(pte);
        } else if (pagingMode == RISCV::PagingMode::Sv48) {
            ppn[3] = swizzle<XLEN_t, ExtendBits::Zero, 53, 37>(pte);
            ppn[2] = swizzle<XLEN_t, ExtendBits::Zero, 36, 28>(pte);
            ppn[1] = swizzle<XLEN_t, ExtendBits::Zero, 27, 19>(pte);
            ppn[0] = swizzle<XLEN_t, ExtendBits::Zero, 18, 10>(pte);
        }
        for (unsigned int lv = 0; (i > 0) && (lv < i); lv++)
            if (ppn[lv] != 0) {
                if constexpr (print_pagewalks)
                    std::cout << "Page fault: misaligned superpage" << std::endl;
                return page_fault;
            }
        if (!(pte & RISCV::PTEBit::A) || (accessType == AccessType::W && !(pte & RISCV::PTEBit::D))) {
            if constexpr (print_pagewalks)
                std::cout << "Page fault: Bad access/dirty bits" << std::endl;
            return page_fault;
        }
        for (; i > 0; i--)
            ppn[i-1] = vpn[i-1];
        XLEN_t phys_addr = 0;
        if (pagingMode == RISCV::PagingMode::Sv32) {
            phys_addr = (ppn[1] << 22) | (ppn[0] << 12) | page_offset;
        } else if (pagingMode == RISCV::PagingMode::Sv39) {
            phys_addr = (ppn[2] << 30ul) | (ppn[1] << 21ul) | (ppn[0] << 12ul) | page_offset;
        } else if (pagingMode == RISCV::PagingMode::Sv48) {
            if constexpr (sizeof(XLEN_t) >= 8)
                phys_addr = (ppn[3] << 39) | (ppn[2] << 30ul) | (ppn[1] << 21) | (ppn[0] << 12) | page_offset;
        }
        if constexpr (print_pagewalks)
            std::cout << "Successful translation: PA=0x"
                      << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                      << phys_addr << std::endl;
        return { phys_addr, va & ~(pagesize - 1), va | (pagesize - 1), RISCV::TrapCause::NONE };
    }

    inline DecodedInstruction<XLEN_t> Decode(__uint32_t encoded) {
        if (RISCV::isCompressed(encoded)) {
            return compressed_inst_lut[encoded & 0x0000ffff];
        }
        __uint32_t packed_instruction = swizzle<__uint32_t, ExtendBits::Zero, 31, 20, 14, 12, 6, 2>(encoded);
        return uncompressed_inst_lut[packed_instruction];
    }

    inline void Callback(HartCallbackArgument arg) {
        if (arg == HartCallbackArgument::RequestedVMfence){
            memset(cacheR, 0, sizeof(cacheR));
            memset(cacheW, 0, sizeof(cacheW));
        }
        if (arg == HartCallbackArgument::RequestedIfence || arg == HartCallbackArgument::RequestedVMfence)
            memset(icache, 0, sizeof(icache));
        if (arg == HartCallbackArgument::ChangedMISA) {
            memset(icache, 0, sizeof(icache));
            ReconfigureDecodeTables();
        }
        return;
    }
};
