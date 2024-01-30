#pragma once

#include <DecodedInstruction.hpp>

template<typename XLEN_t>
class Decoder {
    virtual void Configure(HartState<XLEN_t>* state) = 0;
    virtual DecodedInstruction<XLEN_t> Decode(__uint32_t encoded) = 0;
};
