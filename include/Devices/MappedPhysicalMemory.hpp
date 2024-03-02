#pragma once

#include <cstring>
#include <type_traits>

#include <Device.hpp>

#include <sys/mman.h>
#include <cassert>

class MappedPhysicalMemory final : public Device {

public:
    MappedPhysicalMemory(__uint64_t size) {
        memStartAddress = (char*)mmap(
            NULL,
            size,
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE,
            -1,
            0);
        assert(memStartAddress != MAP_FAILED);
    }

    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* buf) override {
        return TransactInternal<__uint32_t, AccessType::R>(startAddress, size, buf);
    }

    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* buf) override {
        return TransactInternal<__uint64_t, AccessType::R>(startAddress, size, buf);
    }

    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* buf) override {
        return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf);
    }

    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* buf) override {
        return TransactInternal<__uint32_t, AccessType::W>(startAddress, size, buf);
    }

    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* buf) override {
        return TransactInternal<__uint64_t, AccessType::W>(startAddress, size, buf);
    }

    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* buf) override {
        return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf);
    }

    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* buf) override {
        return TransactInternal<__uint32_t, AccessType::X>(startAddress, size, buf);
    }

    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* buf) override {
        return TransactInternal<__uint64_t, AccessType::X>(startAddress, size, buf);
    }

    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* buf) override {
        return TransactInternal<__uint128_t, AccessType::X>(startAddress, size, buf);
    }

private:

    char* memStartAddress;

    template <typename T, AccessType accessType>
    inline T TransactInternal(T startAddress, T size, char* buf) {

        if constexpr (accessType != AccessType::W) {
            memcpy(buf, memStartAddress+startAddress, size);
        } else {
            memcpy(memStartAddress+startAddress, buf, size);
        }
        hint = memStartAddress+startAddress;
        return size;
    }
};
