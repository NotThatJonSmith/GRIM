#pragma once

#include <Decoder.hpp>
#include <RiscVDecoder.hpp>

template<typename XLEN_t>
class DirectDecoder final : public Decoder<XLEN_t> {

private:

    HartState<XLEN_t>* state;

public:

    DirectDecoder(HartState<XLEN_t>* hartState) {
        Configure(hartState);
    }

    void Configure(HartState<XLEN_t>* hartState) override {
        state = hartState;
    }

    DecodedInstruction<XLEN_t> Decode(__uint32_t encoded) override {
        return decode_instruction<XLEN_t>(encoded, state->misa.extensions, state->misa.mxlen).executionFunction;
    }

};
