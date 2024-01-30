#pragma once

#include <cstring>
#include <type_traits>

#include <IOTarget.hpp>

namespace CASK {

class PhysicalMemory final : public CASK::IOTarget {

public:

    PhysicalMemory();
    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override;
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override;
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override;
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override;
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* src) override;

private:

    static constexpr unsigned int tableSize = 1<<16;
    typedef void* Table[tableSize];

    void** top128;
    void** top64;
    void** top32;

    inline void** NewPointerTable() {
        void** t = new void*[1<<16];
        memset(t, 0, 1<<16);
        return t;
    }

    inline char* NewByteChunk() {
        char* t = new char[1<<16];
        memset(t, 0, 1<<16);
        return t;
    }

    template <typename T>
    inline char* GetBlockPointer(T address) {
        if constexpr (std::is_same<T, __uint128_t>()) {
            return GetBlockPointerRecursive<T, 7>(top128, address);
        } else if constexpr (std::is_same<T, __uint64_t>()) {
            return GetBlockPointerRecursive<T, 3>(top64, address);
        } else {
            return GetBlockPointerRecursive<T, 1>(top32, address);
        }
    }

    template <typename T, int level>
    inline char* GetBlockPointerRecursive(void** table, T address) {
        __uint16_t tableIndex = (address >> (16*level)) & (T)0x0000ffff;
        if (table[tableIndex] == nullptr) {
            if constexpr (level == 1) {
                table[tableIndex] = NewByteChunk();
            } else {
                table[tableIndex] = NewPointerTable();
            }
        }

        if constexpr (level == 1) {
            return (char*)table[tableIndex];
        } else {
            return GetBlockPointerRecursive<T, level-1>((void**)table[tableIndex], address);
        }
    }

    template <typename T, CASK::AccessType accessType>
    inline T TransactInternal(T startAddress, T size, char* buf) {

        T endAddress = startAddress + size - 1;
        if (endAddress < startAddress) {
            return 0;
        }

        T chunkSize;
        for (T chunkStartAddress = startAddress; chunkStartAddress <= endAddress; chunkStartAddress += chunkSize) {

            T chunkEndAddress = (chunkStartAddress & ~0x0000ffff) + 0x10000 - 1;
            if (chunkEndAddress > endAddress) {
                chunkEndAddress = endAddress;
            }

            chunkSize = chunkEndAddress - chunkStartAddress + 1;
            T chunkOffset = chunkStartAddress - startAddress;
            char* chunkInCallerBuf = buf + chunkOffset;
            char* chunkInRadixTree = GetBlockPointer<T>(chunkStartAddress) + (chunkStartAddress & 0x0000ffff);

            if constexpr (accessType != AccessType::W) {
                memcpy(chunkInCallerBuf, chunkInRadixTree, chunkSize);
            } else {
                memcpy(chunkInRadixTree, chunkInCallerBuf, chunkSize);
            }

        }

        return size;
    }
};

} // namespace CASK
