#pragma once

#include <Hart.hpp>
#include <Devices/Bus.hpp>
#include <Devices/MappedPhysicalMemory.hpp>

template<typename MXLEN_t>
class HartTest : public ::testing::Test {
protected:

    static constexpr __uint64_t mem_size = (sizeof(MXLEN_t) < 8) ? 0x100000000 : 0x200000000;

    Hart<MXLEN_t> hart;
    MappedPhysicalMemory mem;
    Bus bus;

    HartTest() : hart(&bus, RISCV::stringToExtensions("imacsu")), mem(mem_size) {
        bus.AddDevice64((Device*)&mem, 0, mem_size-1);
    }

    void RunAtLeast(unsigned int cycles) {
        unsigned int counter = 0;
        while (counter <= cycles)
            counter += hart.Tick();
    }
};

using HartTest64 = HartTest<__uint64_t>;
using HartTest32 = HartTest<__uint32_t>;
