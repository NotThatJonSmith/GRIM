#pragma once

#include <IOTarget.hpp>

#include <iostream>

namespace CASK {

#define UART_REG_TXFIFO		0
#define UART_REG_RXFIFO		1
#define UART_REG_TXCTRL		2
#define UART_REG_RXCTRL		3
#define UART_REG_IE			4
#define UART_REG_IP			5
#define UART_REG_DIV		6

class UART : public CASK::IOTarget {

private:

    char state[4];
    std::ostream *out;
    bool txen = false;
    bool rxen = false;

public:

    UART(std::ostream *out = &std::cout);

    virtual __uint32_t Read32(__uint32_t startAddress, __uint32_t size, char* dst) override;
    virtual __uint64_t Read64(__uint64_t startAddress, __uint64_t size, char* dst) override;
    virtual __uint128_t Read128(__uint128_t startAddress, __uint128_t size, char* dst) override;
    virtual __uint32_t Write32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Write64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Write128(__uint128_t startAddress, __uint128_t size, char* src) override;
    virtual __uint32_t Fetch32(__uint32_t startAddress, __uint32_t size, char* src) override;
    virtual __uint64_t Fetch64(__uint64_t startAddress, __uint64_t size, char* src) override;
    virtual __uint128_t Fetch128(__uint128_t startAddress, __uint128_t size, char* src) override;

};

} // namespace CASK
