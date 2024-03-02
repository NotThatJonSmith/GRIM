#pragma once

#include <Device.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

class Bus final : public Device {

public:

    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<__uint32_t, AccessType::R>(startAddress, size, buf); }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<__uint64_t, AccessType::R>(startAddress, size, buf); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<__uint128_t, AccessType::R>(startAddress, size, buf); }
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<__uint32_t, AccessType::W>(startAddress, size, buf); }
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<__uint64_t, AccessType::W>(startAddress, size, buf); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<__uint128_t, AccessType::W>(startAddress, size, buf); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* buf) override { return TransactInternal<__uint32_t, AccessType::X>(startAddress, size, buf); }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* buf) override { return TransactInternal<__uint64_t, AccessType::X>(startAddress, size, buf); }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* buf) override { return TransactInternal<__uint128_t, AccessType::X>(startAddress, size, buf); }

    void AddDevice32(Device *dev, __uint32_t address, __uint32_t sizeMinusOne) {
        AddDevice<__uint32_t>(dev, address, sizeMinusOne);
        AddDevice<__uint64_t>(dev, address, sizeMinusOne);
        AddDevice<__uint128_t>(dev, address, sizeMinusOne);
    }

    void AddDevice64(Device *dev, __uint64_t address, __uint64_t sizeMinusOne) {
        if (address < ((__uint64_t)1 << 32)) {
            AddDevice<__uint32_t>(dev, address, 0xffffffff-address);
        }
        AddDevice<__uint64_t>(dev, address, sizeMinusOne);
        AddDevice<__uint128_t>(dev, address, sizeMinusOne);
    }

    void AddDevice128(Device *dev, __uint128_t address, __uint128_t sizeMinusOne) {
        if (address < ((__uint64_t)1 << 32)) {
            AddDevice<__uint32_t>(dev, address, 0xffffffff-address);
        }
        if (address < ((__uint128_t)1 << 64)) {
            AddDevice<__uint64_t>(dev, address, 0xffffffffffffffff-address);
        }
        AddDevice<__uint128_t>(dev, address, sizeMinusOne);
    }

private:

    template <typename T>
    struct BusMapping {
        T first;
        T last;
        T deviceStart;
        Device *target;
    };

    std::vector<BusMapping<__uint32_t>> mappings32;
    std::vector<BusMapping<__uint64_t>> mappings64;
    std::vector<BusMapping<__uint128_t>> mappings128;

    template<typename T>
    inline std::vector<BusMapping<T>>* AddressMapForWidth() {
        if constexpr (std::is_same<T, __uint32_t>()) {
            return &mappings32;
        } else if constexpr (std::is_same<T, __uint64_t>()) {
            return &mappings64;
        } else {
            return &mappings128;
        }
    }

    // TODO handle transactions that stride multiple mappings
    template<typename T, AccessType accessType>
    inline T TransactInternal(T startAddress, T size, char* buf) {
        for (BusMapping<T> &candidate : *AddressMapForWidth<T>()) {
            if (startAddress >= candidate.first && startAddress + size - 1 <= candidate.last ) {
                T deviceAddress = startAddress - candidate.deviceStart;
                T result = candidate.target->template Transact<T, accessType>(deviceAddress, size, buf);
                hint = candidate.target->hint;
                return result;
            }
        }

        if constexpr (sizeof(T) <= 8)
            std::cerr << "Bus: Unhandled transaction at address " << startAddress << " of size " << size << std::endl;
        else
            std::cerr << "Bus: Unhandled transaction. 128-bit printing is not supported." << std::endl;
     
        return 0;
    }

    // TODO be clear about the meaning of size (it's actually size-1 right now for max-int problem)
    template<typename T>
    inline void AddDevice(Device *dev, T address, T sizeMinusOne) {
        T first = address;
        T last = address + sizeMinusOne;
        std::vector<BusMapping<T>>* map = AddressMapForWidth<T>();

        for (typename std::vector<BusMapping<T>>::iterator other = map->begin(); other != map->end(); other++) {

            if (first > other->last) { // First is after the other range
                continue;
            } else if (first > other->first) { // First is inside the other range
                if (last > other->last) { // Last is after the other range
                    // Other item has its end clipped off by the new mapping
                    other->last = first - 1;
                } else { // Last is inside the other range
                    BusMapping<T> highMapChunk = { last + 1, other->last, other->deviceStart, other->target };
                    other->last = first - 1;
                    // TODO keep the bigger half on top
                    other = map->emplace(other, highMapChunk);
                    other++;
                }
            } else { // First is before the other range
                if (last > other->last) { // Last is after the other range
                    // Other item is completely engulfed. Remove it!
                    map->erase(other);
                    // TODO log a warning
                } else if (last > other->first) { // Last is inside the other range
                    // Other item has its start clipped off by the new mapping
                    other->first = last + 1;
                } else { // Last is before the other range
                    // No overlap
                    continue;
                }
            }
        }
        map->push_back( { first, last, first, dev } );
    }

};
