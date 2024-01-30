#include <IOTargets/ToHostInstrument.hpp>

namespace CASK {

ToHostInstrument::ToHostInstrument(EventQueue *eq, __uint32_t shutdownEventNumber) :
    events(eq), shutdownEvent(shutdownEventNumber) {    
}

__uint32_t ToHostInstrument::Read32(__uint32_t startAddress, __uint32_t size, char* dst) {
    // TODO fill buffer with nonsense
    return 0;
}

__uint32_t ToHostInstrument::Write32(__uint32_t startAddress, __uint32_t size, char* src) {

    if (size != 4) {
        // TODO logging and 64bit
        return 0;
    }
    __uint32_t value = *((__uint32_t*)src);

    if (startAddress != 0) {
        return 0;
    }

    if ((startAddress & 0b1) == 0) {
        return 0;
    }

    __uint32_t testNum = value >> 1;
    if (testNum == 0) {
        events->EnqueueEvent(shutdownEvent);
        // shutdownWithStatus(0x600DC0DE); TODO status codes
    } else {
        events->EnqueueEvent(shutdownEvent);
        // shutdownWithStatus(0xDEADC0DE);
    }
    return size;
}

__uint64_t ToHostInstrument::Write64(__uint64_t startAddress, __uint64_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint128_t ToHostInstrument::Write128(__uint128_t startAddress, __uint128_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint64_t ToHostInstrument::Read64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint128_t ToHostInstrument::Read128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint32_t ToHostInstrument::Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) {
    return 0;
}

__uint64_t ToHostInstrument::Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return 0;
}

__uint128_t ToHostInstrument::Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return 0;
}

} // namespace CASK
