#pragma once

#include <cstdint>

#include <Decoder.hpp>
#include <Decoders/DirectDecoder.hpp>

#include <RiscV.hpp>

template<typename XLEN_t>
class PrecomputedDecoder final : public Decoder<XLEN_t> {

private:

    __uint32_t configured_extensions;
    RISCV::XlenMode configured_mxlen;
    std::array<DecodedInstruction<XLEN_t>, 1 << 20> uncompressed;
    std::array<DecodedInstruction<XLEN_t>, 1 << 16> compressed;

public:

    PrecomputedDecoder(HartState<XLEN_t>* hartState) :
        configured_extensions(0),
        configured_mxlen(RISCV::XlenMode::XL128) {
        Configure(hartState);
    }

    void Configure(HartState<XLEN_t>* state) override {

        // Skip reconfiguration when nothing has changed.
        if (state->misa.extensions == configured_extensions &&
            state->misa.mxlen == configured_mxlen) {
            return;
        }

        configured_extensions = state->misa.extensions;
        configured_mxlen = state->misa.mxlen;

        DirectDecoder decoder(state);

        for (__uint32_t packed = 0; packed < (1<<20); packed++) {
            uncompressed[packed] = decoder.Decode(Unpack(packed));
        }

        for (__uint32_t encoded = 0; encoded < 1<<16; encoded++) {
            if ((encoded & 0b11) != 0b11) {
                compressed[encoded] = decoder.Decode(encoded);
            }
        }
    }

    DecodedInstruction<XLEN_t> Decode(__uint32_t encoded) override {
        if (RISCV::isCompressed(encoded)) {
            return compressed[encoded & 0x0000ffff];
        }
        return uncompressed[Pack(encoded)];
    }

private:

    __uint32_t Pack(__uint32_t encoded) {
        return swizzle<__uint32_t, ExtendBits::Zero, 31, 20, 14, 12, 6, 2>(encoded);
    }

    __uint32_t Unpack(__uint32_t packed) {
        return 0b11 |
            ((0b00000000000000011111 & packed) << 2) |
            ((0b00000000000011100000 & packed) << 7) |
            ((0b11111111111100000000 & packed) << 12);
    }

};
