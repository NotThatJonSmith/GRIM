#pragma once

#include <cstdint>
#include <type_traits>

#include <iostream>


namespace CASK {

enum AccessType { R, W, X };

class IOTarget {

public:

    void *hint = nullptr;

    /*
     * Generic transaction helper function template. Instantiates as one of the
     * nine interface functions depending on the chosen address width and access
     * type, { 32, 64, 128 } X { Read, Write, Fetch }.
     */
    template<typename T, AccessType accessType>
    inline T Transact(T startAddress, T size, char* buf) {
        if constexpr (accessType == AccessType::R) {
            return Read<T>(startAddress, size, buf);
        } else if constexpr (accessType == AccessType::W) {
            return Write<T>(startAddress, size, buf);
        } else {
            return Fetch<T>(startAddress, size, buf);
        }
    }

    virtual inline __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) = 0;
    virtual inline __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) = 0;
    virtual inline __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) = 0;
    template<typename T>
    inline T Read(T startAddress, T size, char* dst) {
        if constexpr (std::is_same<T, __uint32_t>()) {
            return Read32(startAddress, size, dst);
        } else if constexpr (std::is_same<T, __uint64_t>()) {
            return Read64(startAddress, size, dst);
        } else if constexpr (std::is_same<T, __uint128_t>()) {
            return Read128(startAddress, size, dst);
        } else {
            // TODO fatal, bail out
        }
    }

    virtual inline __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) = 0;
    virtual inline __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) = 0;
    virtual inline __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) = 0;
    template<typename T>
    inline T Write(T startAddress, T size, char* src) {
        if constexpr (std::is_same<T, __uint32_t>()) {
            return Write32(startAddress, size, src);
        } else if constexpr (std::is_same<T, __uint64_t>()) {
            return Write64(startAddress, size, src);
        } else if constexpr (std::is_same<T, __uint128_t>()) {
           return  Write128(startAddress, size, src);
        } else {
            // TODO fatal, bail out
        }
    }

    virtual inline __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) = 0;
    virtual inline __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) = 0;
    virtual inline __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) = 0;
    template<typename T>
    inline T Fetch(T startAddress, T size, char* dst) {
        if constexpr (std::is_same<T, __uint32_t>()) {
            return Fetch32(startAddress, size, dst);
        } else if constexpr (std::is_same<T, __uint64_t>()) {
            return Fetch64(startAddress, size, dst);
        } else if constexpr (std::is_same<T, __uint128_t>()) {
            return Fetch128(startAddress, size, dst);
        } else {
            // TODO fatal, bail out
        }
    }

};

} // namespace CASK
