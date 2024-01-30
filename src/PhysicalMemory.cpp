#include <IOTargets/PhysicalMemory.hpp>

namespace CASK {

PhysicalMemory::PhysicalMemory() {
    void ** cur;

    cur = NewPointerTable();
    top128 = cur;

    cur[0] = (void*)NewPointerTable();
    cur = (void**)cur[0];

    cur[0] = (void*)NewPointerTable();
    cur = (void**)cur[0];

    cur[0] = (void*)NewPointerTable();
    cur = (void**)cur[0];

    cur[0] = (void*)NewPointerTable();
    cur = (void**)cur[0];
    top64 = cur;

    cur[0] = (void*)NewPointerTable();
    cur = (void**)cur[0];

    cur[0] = (void*)NewPointerTable();
    cur = (void**)cur[0];
    top32 = cur;

}

__uint32_t PhysicalMemory::Read32(__uint32_t startAddress, __uint32_t size, char* buf) {
    return TransactInternal<__uint32_t, AccessType::R>(startAddress, size, buf);
}

__uint64_t PhysicalMemory::Read64(__uint64_t startAddress, __uint64_t size, char* buf) {
    return TransactInternal<__uint64_t, AccessType::R>(startAddress, size, buf);
}

__uint128_t PhysicalMemory::Read128(__uint128_t startAddress, __uint128_t size, char* buf) {
    return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf);
}

__uint32_t PhysicalMemory::Write32(__uint32_t startAddress, __uint32_t size, char* buf) {
    return TransactInternal<__uint32_t, AccessType::W>(startAddress, size, buf);
}

__uint64_t PhysicalMemory::Write64(__uint64_t startAddress, __uint64_t size, char* buf) {
    return TransactInternal<__uint64_t, AccessType::W>(startAddress, size, buf);
}

__uint128_t PhysicalMemory::Write128(__uint128_t startAddress, __uint128_t size, char* buf) {
    return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf);
}

__uint32_t PhysicalMemory::Fetch32(__uint32_t startAddress, __uint32_t size, char* buf) {
    return TransactInternal<__uint32_t, AccessType::X>(startAddress, size, buf);
}

__uint64_t PhysicalMemory::Fetch64(__uint64_t startAddress, __uint64_t size, char* buf) {
    return TransactInternal<__uint64_t, AccessType::X>(startAddress, size, buf);
}

__uint128_t PhysicalMemory::Fetch128(__uint128_t startAddress, __uint128_t size, char* buf) {
    return TransactInternal<__uint128_t, AccessType::X>(startAddress, size, buf);
}

} // namespace CASK
