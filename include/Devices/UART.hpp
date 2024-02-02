#pragma once

#include <Device.hpp>

#include <iostream>

#define UART_REG_TXFIFO		0
#define UART_REG_RXFIFO		1
#define UART_REG_TXCTRL		2
#define UART_REG_RXCTRL		3
#define UART_REG_IE			4
#define UART_REG_IP			5
#define UART_REG_DIV		6

class UART : public Device {

private:

    char state[4];
    std::ostream *out;
    bool txen = false;
    bool rxen = false;

public:

    UART(std::ostream *out = &std::cout) : out(out) { }

    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override {

        if (size != 4) {
            *out << "WARNING: UART only accepts IO in four-byte words" << std::endl;
            return 0;
        }

        if (startAddress % 4 != 0) {
            *out << "WARNING: UART only accepts four-byte aligned start addresses" << std::endl;
            return 0;
        }

        dst[0] = dst[1] = dst[2] = dst[3] = 0;

        switch (startAddress) {
        case UART_REG_TXFIFO * 4:
            // Reading back from the TXFIFO queue is nonsense but not illegal
            break;
        case UART_REG_RXFIFO * 4:
            // Reading 4B from the RX FIFO
            dst[0] = state[startAddress+0];
            dst[1] = state[startAddress+1];
            dst[2] = state[startAddress+2];
            dst[3] = state[startAddress+3];
            break;
        case UART_REG_TXCTRL * 4:
            dst[0] = txen ? 1 : 0;
            break;
        case UART_REG_RXCTRL * 4:
            dst[0] = rxen ? 1 : 0;
            break;
        case UART_REG_IE * 4:
            *out << "WARNING: Unimplemented UART register" << std::endl;
            break;
        case UART_REG_IP * 4:
            *out << "WARNING: Unimplemented UART register" << std::endl;
            break;
        case UART_REG_DIV * 4:
            *out << "WARNING: Unimplemented UART register" << std::endl;
            break;
        default:
            *out << "WARNING: Out-of-bounds read from UART regs" << std::endl;
            break;
        }

        return size;
    }
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override { return Read32(startAddress & 0xffffffff, size, dst); }

    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override {

        if (size != 4) {
            *out << "WARNING: UART only accepts IO in four-byte words" << std::endl;
            return 0;
        }

        if (startAddress % 4 != 0) {
            *out << "WARNING: UART only accepts four-byte aligned start addresses" << std::endl;
            return 0;
        }

        // TODO def'd values are not mul4 and they should be
        switch (startAddress) {
        case UART_REG_TXFIFO * 4:
            for (unsigned int i = 0; i < size; i++) {
                *out << src[i];
            }  
            break;
        case UART_REG_RXFIFO * 4:
            // Writing to the RX buffer is nonsense
            break;
        case UART_REG_TXCTRL * 4:
            txen = src[0] != 0;
            break;
        case UART_REG_RXCTRL * 4:
            rxen = src[0] != 0;
            break;
        case UART_REG_IE * 4:
            *out << "WARNING: Write to unimplemented UART register" << std::endl;
            break;
        case UART_REG_IP * 4:
            *out << "WARNING: Write to unimplemented UART register" << std::endl;
            break;
        case UART_REG_DIV * 4:
            *out << "WARNING: Write to unimplemented UART register" << std::endl;
            break;
        default:
            *out << "WARNING: Out-of-bounds write to UART regs" << std::endl;
            break;
        }

        return size;
    }

    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override { return Write32(startAddress & 0xffffffff, size, src); }
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* dst) override { return 0; }
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* dst) override { return 0; }
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* dst) override { return 0; }
};
