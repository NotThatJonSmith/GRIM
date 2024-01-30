#pragma once

#include <RiscV.hpp>

template<typename XLEN_t>
struct Transaction {
    RISCV::TrapCause trapCause;
    XLEN_t transferredSize;
};
