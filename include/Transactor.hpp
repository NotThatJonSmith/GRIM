#pragma once

#include <Transaction.hpp>
#include <IOTarget.hpp>

template<typename XLEN_t>
class Transactor {
public:
    virtual inline Transaction<XLEN_t> Read(XLEN_t startAddress, XLEN_t size, char* buf) = 0;
    virtual inline Transaction<XLEN_t> Write(XLEN_t startAddress, XLEN_t size, char* buf) = 0;
    virtual inline Transaction<XLEN_t> Fetch(XLEN_t startAddress, XLEN_t size, char* buf) = 0;
    template<CASK::AccessType accessType>
    inline Transaction<XLEN_t> Transact(XLEN_t startAddress, XLEN_t size, char* buf) {
        if constexpr (accessType == CASK::AccessType::R) {
            return Read(startAddress, size, buf);
        } else if constexpr (accessType == CASK::AccessType::W) {
            return Write(startAddress, size, buf);
        } else {
            return Fetch(startAddress, size, buf);
        }
    }
};
