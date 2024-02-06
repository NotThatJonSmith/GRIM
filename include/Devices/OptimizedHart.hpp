#pragma once

#include <type_traits>
#include <cstdint>

#include <RiscVDecoder.hpp>
#include <RiscVTranslationAlgorithm.hpp>

// TODO accelerated WFI states?
template<typename XLEN_t>
class OptimizedHart {

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
            ICacheEntry *inst = &icache[icacheIndex];
            if (inst->full_pc == state.pc) [[ likely ]] {
                inst->instruction(inst->encoding, this);
                continue;
            }
            __uint32_t encoding;
            if (!Transact<__uint32_t, AccessType::X>(state.pc, (char*)&encoding))
                continue;
            DecodedInstruction<XLEN_t> decoded = Decode(encoding);
            icache[icacheIndex] = { state.pc, encoding, decoded };
            decoded(encoding, this);
        }
        return fastLoopTicks;
    };

    unsigned int TickOnceAndPrintDisasm(std::ostream* disasm_pipe) {

        ICacheEntry *inst = &icache[(state.pc >> 1) & ((1<<icacheBits)-1)];
        if (inst->full_pc == state.pc) [[ likely ]] {
            if constexpr (sizeof(XLEN_t) > 8) {
                std::cout << "128 bit printing not supported" << std::endl;
            } else {
                (*disasm_pipe) << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                               << inst->full_pc<< ":\t"
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

        __uint32_t encoding;
        if (!Transact<__uint32_t, AccessType::X>(state.pc, (char*)&encoding))
            return 1;

        if constexpr (sizeof(XLEN_t) > 8) {
            std::cout << "128 bit printing not supported" << std::endl;
        } else {
            (*disasm_pipe) << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                           << state.pc << ":\t"
                           << std::hex << std::setfill('0') << std::setw(sizeof(encoding)*2)
                           << encoding << "\t"
                           << std::dec;
        }
        Instruction<XLEN_t> dinstr = decode_instruction<XLEN_t>(encoding, state.misa.extensions, state.misa.mxlen);
        dinstr.disassemblyFunction(encoding, disasm_pipe);
        DecodedInstruction<XLEN_t> decoded = Decode(encoding);
        assert(decoded == dinstr.executionFunction);
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

    static constexpr std::ostream* memDebugStream = &std::cout;
    // static constexpr std::ostream* memDebugStream = nullptr;
    template <typename MEM_TYPE_t, AccessType accessType, bool fault=false>
    inline void PrintTransaction(XLEN_t startAddress, char* bytes) {
        if constexpr (sizeof(MEM_TYPE_t) >= 16)
            return;
        if constexpr(memDebugStream == nullptr)
            return;
        constexpr unsigned int alignBits = 4;
        (*memDebugStream) << (accessType == AccessType::R ? "Read" : (accessType == AccessType::W ? "Write" : "Fetch"))
                            << std::dec << std::setw(0) << sizeof(MEM_TYPE_t)*8
                            << ": 0x" << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                            << (__uint64_t)startAddress
                            << std::endl;
        (*memDebugStream) << "| ";
        XLEN_t bytesPerRow = 1 << alignBits;
        XLEN_t mask = ~((1 << alignBits) - 1);
        XLEN_t firstRowStartAddress = startAddress & mask;
        for (XLEN_t rowStartAddress = firstRowStartAddress; rowStartAddress < startAddress + sizeof(MEM_TYPE_t); rowStartAddress += bytesPerRow) {
            for (XLEN_t columnIndex = 0; columnIndex < bytesPerRow; columnIndex++) {
                XLEN_t rowIndex = (rowStartAddress - firstRowStartAddress) / bytesPerRow;
                if (columnIndex == 0) {
                    if (rowIndex != 0)
                        (*memDebugStream) << std::endl << "| ";
                    (*memDebugStream) << std::hex << std::setfill('0')
                                        << std::setw(sizeof(XLEN_t)*2)
                                        << (__uint64_t)rowStartAddress << ": ";
                }
                XLEN_t simAddressOfByte = rowStartAddress + columnIndex;
                if (simAddressOfByte < startAddress || simAddressOfByte >= startAddress + sizeof(MEM_TYPE_t))
                    (*memDebugStream) << "** ";
                else if (fault)
                    (*memDebugStream) << "?? ";
                else
                    (*memDebugStream) << std::hex << std::setfill('0') << std::setw(2)
                                        << ((unsigned int)bytes[simAddressOfByte - startAddress] & 0xff)
                                        << " ";
            }
        }
        (*memDebugStream) << std::endl;
    }

    template <typename MEM_TYPE_t, AccessType accessType>
    inline bool Transact(XLEN_t startAddress, char* buf) {
        TranslationCacheEntry* cache = accessType == AccessType::R ? cacheR : (accessType == AccessType::W ? cacheW : cacheX);
        XLEN_t index = (startAddress >> 12) & ((1 << cacheBits) - 1); // TODO assumes 4k pages, need flex for supers
        if ((cache[index].virtPageStart >> 12 == startAddress >> 12) && (cache[index].hostPageStart != nullptr)) [[ likely ]] {
            char *hostAddress = cache[index].hostPageStart + startAddress - cache[index].virtPageStart;
            if constexpr (accessType == AccessType::W) {
                *(MEM_TYPE_t*)hostAddress = *(MEM_TYPE_t*)buf;
                // memcpy(hostAddress, buf, sizeof(MEM_TYPE_t));
            } else {
                *(MEM_TYPE_t*)buf = *(MEM_TYPE_t*)hostAddress;
                // memcpy(buf, hostAddress, sizeof(MEM_TYPE_t));
            }
            if constexpr (memDebugStream != nullptr)
                PrintTransaction<MEM_TYPE_t, accessType>(startAddress, buf);
            return true;
        }

        Translation<XLEN_t> fresh_translation = TranslationAlgorithm<XLEN_t, accessType>(
            startAddress, target, state.satp.ppn, state.satp.pagingMode,
            state.mstatus.mprv ? state.mstatus.mpp : state.privilegeMode,
            state.mstatus.mxr, state.mstatus.sum);
        if (fresh_translation.generatedTrap != RISCV::TrapCause::NONE) [[ unlikely ]] {
            state.RaiseException(fresh_translation.generatedTrap, startAddress);
            if constexpr (memDebugStream != nullptr)
                PrintTransaction<MEM_TYPE_t, accessType, true>(startAddress, buf);
            return false; // TODO this is a lie; how much was really transacted? Latent bug here I'm just lazy now.
        }

        /*TODO what about errant Transaction?*/
        // TODO - Transact should be in 1,2,4,8,16 bytes, not in 4,8,16. Device class needs to change. Would be nice to
        //        collapse it all into one transact function for everything for every device.
        target->template Transact<XLEN_t, accessType>(fresh_translation.translated, sizeof(MEM_TYPE_t), buf);
        if (target->hint) {
            XLEN_t offset = startAddress - fresh_translation.virtPageStart;
            cache[index].hostPageStart = (char*)target->hint - offset;
            cache[index].virtPageStart = fresh_translation.virtPageStart;
            cache[index].validThrough = fresh_translation.validThrough;
            // TODO, sidechannel for TranslationAlgorithm to give us a whole RWX result
            if constexpr (accessType == AccessType::R) {
                Translation<XLEN_t> mirrorTranslation = TranslationAlgorithm<XLEN_t, AccessType::W>(
                    startAddress, target, state.satp.ppn, state.satp.pagingMode,
                    state.mstatus.mprv ? state.mstatus.mpp : state.privilegeMode,
                    state.mstatus.mxr, state.mstatus.sum);
                if (mirrorTranslation.generatedTrap == RISCV::TrapCause::NONE) {
                    cacheW[index].hostPageStart = cache[index].hostPageStart;
                    cacheW[index].virtPageStart = cache[index].virtPageStart;
                    cacheW[index].validThrough = cache[index].validThrough;
                }
            } else if constexpr (accessType == AccessType::W) {
                Translation<XLEN_t> mirrorTranslation = TranslationAlgorithm<XLEN_t, AccessType::R>(
                    startAddress, target, state.satp.ppn, state.satp.pagingMode,
                    state.mstatus.mprv ? state.mstatus.mpp : state.privilegeMode,
                    state.mstatus.mxr, state.mstatus.sum);
                if (mirrorTranslation.generatedTrap == RISCV::TrapCause::NONE) {
                    cacheR[index].hostPageStart = cache[index].hostPageStart;
                    cacheR[index].virtPageStart = cache[index].virtPageStart;
                    cacheR[index].validThrough = cache[index].validThrough;
                }
            }

        }
        if constexpr (memDebugStream != nullptr)
            PrintTransaction<MEM_TYPE_t, accessType>(startAddress, buf);
        return true;
    }


private:

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
