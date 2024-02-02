#pragma once

#include <type_traits>
#include <cstdint>

#include <Devices/VirtToHostTransactor.hpp>
#include <RiscVDecoder.hpp>

template<typename XLEN_t>
class PrecomputedDecoder {

private:

    __uint32_t configured_extensions;
    RISCV::XlenMode configured_mxlen;
    std::array<DecodedInstruction<XLEN_t>, 1 << 20> uncompressed_inst_lut;
    std::array<DecodedInstruction<XLEN_t>, 1 << 16> compressed_inst_lut;

public:

    PrecomputedDecoder(HartState<XLEN_t>* hartState) :
        configured_extensions(0),
        configured_mxlen(RISCV::XlenMode::XL128) {
        Configure(hartState);
    }

    void Configure(HartState<XLEN_t>* state) {

        // Skip reconfiguration when nothing has changed.
        if (state->misa.extensions == configured_extensions &&
            state->misa.mxlen == configured_mxlen) {
            return;
        }

        configured_extensions = state->misa.extensions;
        configured_mxlen = state->misa.mxlen;

        for (__uint32_t packed_instruction = 0; packed_instruction < (1<<20); packed_instruction++) {
            __uint32_t unpacked_encoding = 0b11 |
                ((0b00000000000000011111 & packed_instruction) << 2) |
                ((0b00000000000011100000 & packed_instruction) << 7) |
                ((0b11111111111100000000 & packed_instruction) << 12);
            uncompressed_inst_lut[packed_instruction] = decode_instruction<XLEN_t>(unpacked_encoding, state->misa.extensions, state->misa.mxlen).executionFunction;
        }

        for (__uint32_t encoded = 0; encoded < 1<<16; encoded++) {
            if ((encoded & 0b11) != 0b11) {
                compressed_inst_lut[encoded] = decode_instruction<XLEN_t>(encoded, state->misa.extensions, state->misa.mxlen).executionFunction;
            }
        }
    }

    DecodedInstruction<XLEN_t> Decode(__uint32_t encoded) {
        if (RISCV::isCompressed(encoded)) {
            return compressed_inst_lut[encoded & 0x0000ffff];
        }
        __uint32_t packed_instruction = swizzle<__uint32_t, ExtendBits::Zero, 31, 20, 14, 12, 6, 2>(encoded);
        return uncompressed_inst_lut[packed_instruction];
    }

};


// TODO accelerated WFI states?
template<typename XLEN_t>
class OptimizedHart final : public Device {

private:

    static constexpr unsigned int icacheBits = 14;
    static constexpr unsigned int virtHostCacheBits = 8;

    PrecomputedDecoder<XLEN_t> decoder;
    VirtToHostTransactor<XLEN_t, virtHostCacheBits> transactor;
    // TODO turn on bufferTransactions later on.

    struct SimplyCachedInstruction {
        XLEN_t full_pc;
        __uint32_t encoding = 0;
        DecodedInstruction<XLEN_t> instruction = nullptr;
    };
    SimplyCachedInstruction icache[1<<icacheBits];

public:

    HartState<XLEN_t> state;

    OptimizedHart(Device* bus, __uint32_t maximalExtensions) :
        decoder(&state),
        transactor(bus, &state),
        state(maximalExtensions) {
        state.implCallback = std::bind(&OptimizedHart::Callback, this, std::placeholders::_1);
        // TODO callback for changing XLENs
        Reset();
    };

    virtual inline unsigned int Tick() override {
        for (unsigned int i = 0; i < 10000; i++) {
            SimplyCachedInstruction *inst = &icache[(state.pc >> 1) & ((1<<icacheBits)-1)];
            if (inst->full_pc == state.pc) [[ likely ]] {
                inst->instruction(inst->encoding, &state, &transactor);
                continue;
            }
            __uint32_t encoding;
            XLEN_t transferredSize = transactor.template Fetch<XLEN_t>(state.pc, sizeof(encoding), (char*)&encoding);
            if (transferredSize != sizeof(encoding)) {
                continue;
            }
            DecodedInstruction<XLEN_t> decoded = decoder.Decode(encoding);
            icache[(state.pc >> 1) & ((1<<icacheBits)-1)] = { state.pc, encoding, decoded };
            decoded(encoding, &state, &transactor);
        }
        return 10000;
    };

    unsigned int TickOnceAndPrintDisasm(std::ostream* disasm_pipe) {

        SimplyCachedInstruction *inst = &icache[(state.pc >> 1) & ((1<<icacheBits)-1)];
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

            inst->instruction(inst->encoding, &state, &transactor);
            return 1;
        }

        __uint32_t encoding;
        XLEN_t transferredSize = transactor.template Fetch<XLEN_t>(state.pc, sizeof(encoding), (char*)&encoding);
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

        DecodedInstruction<XLEN_t> decoded = decoder.Decode(encoding);

        assert(decoded == dinstr.executionFunction);

        icache[(state.pc >> 1) & ((1<<icacheBits)-1)] = { state.pc, encoding, decoded };
        decoded(encoding, &state, &transactor);

        return 1;
    };

    virtual inline void Reset() override {
        state.Reset();
        transactor.Clear();
        decoder.Configure(&state);
        memset(icache, 0, sizeof(icache));
    };

    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* buf) override { return 0; }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* buf) override { return 0; }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* buf) override { return 0; }
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* buf) override { return 0; }
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* buf) override { return 0; }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* buf) override { return 0; }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* buf) override { return 0; }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* buf) override { return 0; }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* buf) override { return 0; }

private:

    inline void Callback(HartCallbackArgument arg) {
        if (arg == HartCallbackArgument::RequestedVMfence)
            transactor.Clear();
        if (arg == HartCallbackArgument::RequestedIfence || arg == HartCallbackArgument::RequestedVMfence)
            memset(icache, 0, sizeof(icache));
        if (arg == HartCallbackArgument::ChangedMISA) {
            memset(icache, 0, sizeof(icache));
            decoder.Configure(&state);
        }
        return;
    }

};
