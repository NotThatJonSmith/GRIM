#pragma once

#include <Device.hpp>

class CoreLocalInterruptor : public Device {

private:

    __uint32_t msip[5];
    __uint64_t mtimecmp[5];
    __uint64_t mtime;

public:
    
    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override {

        if (size != 4) {
            // TODO logging and 64bit
            return 0;
        }
        
        __uint32_t value = 0;

        if (startAddress < 0x4000) {
            unsigned int msip_idx = startAddress/4;
            if (msip_idx <= 4)
                value = msip[msip_idx];
        } else if (startAddress < 0xb000) {
            unsigned int mtimecmp_idx = (startAddress-0x4000)/8;
            if (mtimecmp_idx <= 4)
                value = mtimecmp[mtimecmp_idx];
        } else if (startAddress == 0xbff8) {
            value = mtime;
        }

        *((__uint32_t*)dst) = value;
        return size;
        
    }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }

    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override {

        if (size != 4) {
            // TODO logging and 64bit
            return 0;
        }

        __uint32_t value = *((__uint32_t*)src);

        if (startAddress < 0x4000) {
            unsigned int msip_idx = startAddress/4;
            if (msip_idx <= 4)
                msip[msip_idx] = value;
        } else if (startAddress < 0xb000) {
            unsigned int mtimecmp_idx = (startAddress-0x4000)/8;
            if (mtimecmp_idx <= 4)
                mtimecmp[mtimecmp_idx] = value;
        } else if (startAddress == 0xbff8) {
            mtime = value;
        }

        return size;
        
    }

    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) override { return 0; }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) override { return 0; }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) override { return 0; }
    virtual unsigned int Tick() override { return 1; }


};
