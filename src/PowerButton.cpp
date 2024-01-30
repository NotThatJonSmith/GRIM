#include <IOTargets/PowerButton.hpp>

namespace CASK {

PowerButton::PowerButton(EventQueue *eq, __uint32_t shutdownEventNumber) :
    events(eq), shutdownEvent(shutdownEventNumber) { }

__uint32_t PowerButton::Read32(__uint32_t startAddress, __uint32_t size, char* dst) {
    // TODO fill buffer with nonsense
    return 0;
}

__uint32_t PowerButton::Write32(__uint32_t startAddress, __uint32_t size, char* src) {
    if (size != 4) {
        // TODO logging and 64bit
        return 0;
    }
    // __uint32_t value = *((__uint32_t*)src);
    events->EnqueueEvent(shutdownEvent);
    return size;
}

__uint64_t PowerButton::Write64(__uint64_t startAddress, __uint64_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint128_t PowerButton::Write128(__uint128_t startAddress, __uint128_t size, char* src) {
    return Write32(startAddress & 0xffffffff, size, src);
}

__uint64_t PowerButton::Read64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint128_t PowerButton::Read128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return Read32(startAddress & 0xffffffff, size, dst);
}

__uint32_t PowerButton::Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) {
    return 0;
}

__uint64_t PowerButton::Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) {
    return 0;
}

__uint128_t PowerButton::Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) {
    return 0;
}

} // namespace CASK
