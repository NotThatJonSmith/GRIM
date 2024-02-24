#include <gtest/gtest.h>

#include <Hart.hpp>
#include <Devices/Bus.hpp>
#include <Devices/MappedPhysicalMemory.hpp>

template<typename MXLEN_t>
class HartTest : public ::testing::Test {
protected:
    Hart<MXLEN_t> hart;
    MappedPhysicalMemory mem;
    Bus bus;

    HartTest() : hart(&bus, RISCV::stringToExtensions("imacsu")) {
        bus.AddDevice32((Device*)&mem, 0, 0xffffffff);
    }

};

using HartTest64 = HartTest<__uint64_t>;

TEST_F(HartTest64, Test1) {
    ASSERT_EQ(1, 1);
}

#include <rv64gc/addiw.h>

TEST_F(HartTest64, Test2) {
    ASSERT_EQ(1, 1);

    // Write generated assembly to memory
    bus.Write32(0x80000000, sizeof(ADDIW_BYTES), (char*)ADDIW_BYTES);

    // Set the PC to the start of the generated assembly
    hart.state.resetVector = 0x80000000;

    ASSERT_EQ(hart.state.regs[10], (__uint64_t)0);

    // Run the hart
    hart.Tick();

    ASSERT_EQ(hart.state.regs[10], 0xffff'ffff'ffff'ffff);

    hart.Reset();
    ASSERT_EQ(hart.state.regs[10], (__uint64_t)0);

}
