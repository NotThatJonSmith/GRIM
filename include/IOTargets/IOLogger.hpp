#pragma once

#include <IOTarget.hpp>

#include <ostream>
#include <iomanip>

#include <queue>

namespace CASK {

class IOLogger : public IOTarget {

public:

    IOLogger(IOTarget* startTarget, std::ostream* startOStream);
    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override;
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override;
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override;
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override;
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* src) override;
    void SetPrintContents(bool enabled);

private:

    IOTarget* target;
    std::ostream* stream;
    bool logContents;

    template<typename T>
    T WriteLog(std::string ioFunctionName, char* bytes, T startAddress, T size, T alignBits=4) {

        if (std::is_same<T, __uint128_t>()) {
            (*stream) << "TODO 128-bit logging is not supported" << std::endl;
            return size;
        }

        (*stream) << ioFunctionName << ":"
                  << std::hex << std::setfill('0') << std::setw(sizeof(T)*2)
                  << " start: 0x" << (__uint64_t)startAddress
                  << " size: 0x" << (__uint64_t)size
                  << std::endl;

        if (!logContents) {
            return size;
        }

        (*stream) << "| ";

        T bytesPerRow = 1 << alignBits;
        T mask = ~((1 << alignBits) - 1);
        T firstRowStartAddress = startAddress & mask;

        for (T rowStartAddress = firstRowStartAddress; rowStartAddress < startAddress + size; rowStartAddress += bytesPerRow) {
            for (T columnIndex = 0; columnIndex < bytesPerRow; columnIndex++) {

                T rowIndex = (rowStartAddress - firstRowStartAddress) / bytesPerRow;

                if (columnIndex == 0) {

                    if (rowIndex != 0) {
                        (*stream) << std::endl << "| ";
                    }

                    (*stream) << std::hex << std::setfill('0')
                              << std::setw(sizeof(T)*2)
                              << (__uint64_t)rowStartAddress << ": ";
                }

                T simAddressOfByte = rowStartAddress + columnIndex;
                if (simAddressOfByte < startAddress || simAddressOfByte >= startAddress + size) {
                    (*stream) << "** ";
                } else {
                    (*stream) << std::hex << std::setfill('0') << std::setw(2)
                              << ((unsigned int)bytes[simAddressOfByte - startAddress] & 0xff)
                              << " ";
                }
            }
        }
        (*stream) << std::endl;
        return size;
    }
};

} // namespace CASK
