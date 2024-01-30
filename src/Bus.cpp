#include <IOTargets/Bus.hpp>

namespace CASK {

__uint32_t Bus::Read32(__uint32_t startAddress, __uint32_t size, char* buf) {
    return TransactInternal<__uint32_t, AccessType::R>(startAddress, size, buf);
}

__uint64_t Bus::Read64(__uint64_t startAddress, __uint64_t size, char* buf) {
    return TransactInternal<__uint64_t, AccessType::R>(startAddress, size, buf);
}

__uint128_t Bus::Read128(__uint128_t startAddress, __uint128_t size, char* buf) {
    return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf);
}

__uint32_t Bus::Write32(__uint32_t startAddress, __uint32_t size, char* buf) {
    return TransactInternal<__uint32_t, AccessType::W>(startAddress, size, buf);
}

__uint64_t Bus::Write64(__uint64_t startAddress, __uint64_t size, char* buf) {
    return TransactInternal<__uint64_t, AccessType::W>(startAddress, size, buf);
}

__uint128_t Bus::Write128(__uint128_t startAddress, __uint128_t size, char* buf) {
    return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf);
}

__uint32_t Bus::Fetch32(__uint32_t startAddress, __uint32_t size, char* buf) {
    return TransactInternal<__uint32_t, AccessType::X>(startAddress, size, buf);
}

__uint64_t Bus::Fetch64(__uint64_t startAddress, __uint64_t size, char* buf) {
    return TransactInternal<__uint64_t, AccessType::X>(startAddress, size, buf);
}

__uint128_t Bus::Fetch128(__uint128_t startAddress, __uint128_t size, char* buf) {
    return TransactInternal<__uint128_t, AccessType::X>(startAddress, size, buf);
}

void Bus::AddIOTarget32(IOTarget *dev, __uint32_t address, __uint32_t sizeMinusOne) {
    AddIOTarget<__uint32_t>(dev, address, sizeMinusOne);
    AddIOTarget<__uint64_t>(dev, address, sizeMinusOne);
    AddIOTarget<__uint128_t>(dev, address, sizeMinusOne);
}

void Bus::AddIOTarget64(IOTarget *dev, __uint64_t address, __uint64_t sizeMinusOne) {
    if (address < ((__uint64_t)1 << 32)) {
        AddIOTarget<__uint32_t>(dev, address, 0xffffffff-address);
    }
    AddIOTarget<__uint64_t>(dev, address, sizeMinusOne);
    AddIOTarget<__uint128_t>(dev, address, sizeMinusOne);
}

void Bus::AddIOTarget128(IOTarget *dev, __uint128_t address, __uint128_t sizeMinusOne) {
    if (address < ((__uint64_t)1 << 32)) {
        AddIOTarget<__uint32_t>(dev, address, 0xffffffff-address);
    }
    if (address < ((__uint128_t)1 << 64)) {
        AddIOTarget<__uint64_t>(dev, address, 0xffffffffffffffff-address);
    }
    AddIOTarget<__uint128_t>(dev, address, sizeMinusOne);
}

} // namespace CASK
