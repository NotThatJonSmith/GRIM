#include <IOTargets/CoreLocalInterruptor.hpp>

namespace CASK {

__uint32_t CoreLocalInterruptor::Read32(__uint32_t startAddress, __uint32_t size, char* dst) {

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

__uint32_t CoreLocalInterruptor::Write32(__uint32_t startAddress, __uint32_t size, char* src) {

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

__uint64_t CoreLocalInterruptor::Write64(__uint64_t startAddress, __uint64_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint128_t CoreLocalInterruptor::Write128(__uint128_t startAddress, __uint128_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint64_t CoreLocalInterruptor::Read64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint128_t CoreLocalInterruptor::Read128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint32_t CoreLocalInterruptor::Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) {
    return 0;
}

__uint64_t CoreLocalInterruptor::Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return 0;
}

__uint128_t CoreLocalInterruptor::Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return 0;
}

unsigned int CoreLocalInterruptor::Tick() {
    // TODO
    return 1;
}

} // namespace CASK
