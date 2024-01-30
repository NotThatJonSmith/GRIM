#pragma once

#include <RiscV.hpp>

template<typename XLEN_t>
struct Translation {
    XLEN_t untranslated;
    XLEN_t translated;
    XLEN_t virtPageStart;
    XLEN_t validThrough;
    RISCV::TrapCause generatedTrap;
};
