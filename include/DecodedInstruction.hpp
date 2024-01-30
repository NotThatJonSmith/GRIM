#pragma once

#include <cstdint>

#include <Transactor.hpp>

template<typename XLEN_t>
class HartState;

template<typename XLEN_t>
using DecodedInstruction = void (*)(__uint32_t encoding, HartState<XLEN_t> *state, Transactor<XLEN_t> *mem);

template<typename XLEN_t>
using DisassemblyFunction = void (*)(__uint32_t encoding, std::ostream* out);

template<typename XLEN_t>
struct Instruction {
    DecodedInstruction<XLEN_t> executionFunction;
    DisassemblyFunction<XLEN_t> disassemblyFunction;
};
