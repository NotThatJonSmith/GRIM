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

    static constexpr unsigned int cacheBits = 8;
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
            ICacheEntry *inst = &icache[(state.pc >> 1) & ((1<<icacheBits)-1)];
            if (inst->full_pc == state.pc) [[ likely ]] {
                inst->instruction(inst->encoding, this);
                continue;
            }
            __uint32_t encoding;
            XLEN_t transferredSize = Transact<__uint32_t, AccessType::X>(state.pc, (char*)&encoding);
            if (transferredSize != sizeof(encoding)) {
                continue;
            }
            DecodedInstruction<XLEN_t> decoded = Decode(encoding);
            icache[(state.pc >> 1) & ((1<<icacheBits)-1)] = { state.pc, encoding, decoded };
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
        XLEN_t transferredSize = Transact<__uint32_t, AccessType::X>(state.pc, (char*)&encoding);
        if (transferredSize != sizeof(encoding)) {
            return 1;
        }

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

    template <typename MEM_TYPE_t, AccessType accessType>
    inline XLEN_t Transact(XLEN_t startAddress, char* buf) {
        TranslationCacheEntry* cache = accessType == AccessType::R ? cacheR : (accessType == AccessType::W ? cacheW : cacheX);
        unsigned int index = (startAddress >> 12) & ((1 << cacheBits) - 1); // TODO assumes 4k pages, need flex for supers
        if (cache[index].virtPageStart >> 12 == startAddress >> 12) [[ likely ]] {
            char *hostAddress = cache[index].hostPageStart + startAddress - cache[index].virtPageStart;
            if constexpr (accessType == AccessType::W) {
                *(MEM_TYPE_t*)hostAddress = *(MEM_TYPE_t*)buf;
            } else {
                *(MEM_TYPE_t*)buf = *(MEM_TYPE_t*)hostAddress;
            }
            return sizeof(MEM_TYPE_t);
        }
        Translation<XLEN_t> fresh_translation = TranslationAlgorithm<XLEN_t, accessType>(
            startAddress, target, state.satp.ppn, state.satp.pagingMode,
            state.mstatus.mprv ? state.mstatus.mpp : state.privilegeMode,
            state.mstatus.mxr, state.mstatus.sum);
        if (fresh_translation.generatedTrap != RISCV::TrapCause::NONE) [[ unlikely ]] {
            state.RaiseException(fresh_translation.generatedTrap, startAddress);
            return 0; // TODO this is a lie; how much was really transacted? Latent bug here I'm just lazy now.
        }

        XLEN_t translatedChunkStart = fresh_translation.translated + startAddress - fresh_translation.untranslated;
        /*XLEN_t transferredSize = TODO, errors if mismatch?*/
        target->template Transact<XLEN_t, accessType>(translatedChunkStart, sizeof(MEM_TYPE_t), buf);
        if (target->hint) {
            XLEN_t offset = fresh_translation.untranslated - fresh_translation.virtPageStart;
            cache[index].hostPageStart = (char*)target->hint - offset;
            cache[index].virtPageStart = fresh_translation.virtPageStart;
            cache[index].validThrough = fresh_translation.validThrough;
        }
        return sizeof(MEM_TYPE_t);
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
