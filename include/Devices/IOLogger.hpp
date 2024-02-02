#pragma once

#include <Device.hpp>

#include <ostream>
#include <iomanip>

#include <queue>

class IOLogger : public Device {

public:

    IOLogger(Device* startTarget, std::ostream* startOStream) : target(startTarget), stream(startOStream) {}
    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override { target->Read32(startAddress, size, dst); return WriteLog<__uint32_t>("Read32", dst, startAddress, size); }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override { target->Read64(startAddress, size, dst); return WriteLog<__uint64_t>("Read64", dst, startAddress, size); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override { target->Read128(startAddress, size, dst); return WriteLog<__uint128_t>("Read128", dst, startAddress, size); }
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override { target->Write32(startAddress, size, src); return WriteLog<__uint32_t>("Write32", src, startAddress, size); }
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override { target->Write64(startAddress, size, src); return WriteLog<__uint64_t>("Write64", src, startAddress, size); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override { target->Write128(startAddress, size, src); return WriteLog<__uint128_t>("Write128", src, startAddress, size); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) override { target->Fetch32(startAddress, size, dst); return WriteLog<__uint32_t>("Fetch32", dst, startAddress, size); }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) override { target->Fetch64(startAddress, size, dst); return WriteLog<__uint64_t>("Fetch64", dst, startAddress, size); }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) override { target->Fetch128(startAddress, size, dst); return WriteLog<__uint128_t>("Fetch128", dst, startAddress, size); }
    void SetPrintContents(bool enabled) { logContents = enabled; }

private:

    Device* target;
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
