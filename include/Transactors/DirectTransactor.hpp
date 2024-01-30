#pragma once

#include <Transactor.hpp>

template<typename XLEN_t>
class DirectTransactor final : public Transactor<XLEN_t> {

private:

    CASK::IOTarget* target;

public:

    DirectTransactor(CASK::IOTarget* ioTarget) : target(ioTarget) {}

    virtual Transaction<XLEN_t> Read(XLEN_t startAddress, XLEN_t size, char* buf) override {
        return { RISCV::TrapCause::NONE, target->Read<XLEN_t>(startAddress, size, buf) };
    }

    virtual Transaction<XLEN_t> Write(XLEN_t startAddress, XLEN_t size, char* buf) override {
        return { RISCV::TrapCause::NONE, target->Write<XLEN_t>(startAddress, size, buf) };
    }

    virtual Transaction<XLEN_t> Fetch(XLEN_t startAddress, XLEN_t size, char* buf) override {
        return { RISCV::TrapCause::NONE, target->Fetch<XLEN_t>(startAddress, size, buf) };
    }
};
