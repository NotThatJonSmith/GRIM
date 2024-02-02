#pragma once

#include <Device.hpp>
#include <queue>

class PowerButton : public Device {

private:

    std::queue<unsigned int> *events;
    __uint32_t shutdownEvent;

public:
    
    PowerButton(std::queue<unsigned int> *eq, __uint32_t shutdownEventNumber) : events(eq), shutdownEvent(shutdownEventNumber) { }
    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override { return 0; }
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override {
        if (size != 4) {
            // TODO logging and 64bit
            return 0;
        }
        // __uint32_t value = *((__uint32_t*)src);
        events->push(shutdownEvent);
        return size;
    }
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) override { return 0; }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) override { return 0; }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) override { return 0; }
};
