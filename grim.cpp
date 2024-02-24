#include <iostream>
#include <chrono>
#include <array>

#include <cxxopts.hpp>

#include <ElfFile.hpp>
#include <Devices/Bus.hpp>
#include <Devices/IOLogger.hpp>
#include <Devices/MappedPhysicalMemory.hpp>
#include <Devices/CoreLocalInterruptor.hpp>
#include <Devices/PowerButton.hpp>
#include <Devices/ProxyKernelServer.hpp>
#include <Devices/UART.hpp>
#include <Hart.hpp>
#include <PrintStates.hpp>

__uint64_t MaskForSize(__uint64_t size) {
    if (size > (__uint64_t)1 << 63) // TODO this assumes Address is 64 bit
        return ~(__uint64_t)0;
    __uint64_t nextPo2 = 1;
    for (nextPo2 = 1; nextPo2 < size; nextPo2 <<= 1);
    return nextPo2 - 1;
}

// Note: These parameters could be constexpr-if'd but that breaks pedantry, and
//       the compiler does the right thing in O3.
template <typename MXLEN_t, bool limit_cycles, bool check_events, bool print_regs, bool print_disasm, bool print_details>
unsigned int tick_until(
        Hart<MXLEN_t> *hart,
        CoreLocalInterruptor* clint,
        std::queue<unsigned int> *eq,
        std::ostream *out,
        unsigned int *ticks,
        unsigned int cycle_limit,
        unsigned int event_check_freq,
        bool useRegAbiNames) {

    while (true) {

        if constexpr (print_details) {
            PrintArchDetails<MXLEN_t>(&hart->state, out);
        }

        if constexpr (print_regs) {
            PrintRegisters<MXLEN_t>(&hart->state, out, useRegAbiNames, 4);
        }

        if constexpr (print_disasm) {
            if constexpr (limit_cycles || check_events) {
                (*ticks) += hart->TickOnceAndPrintDisasm(&std::cout);
            } else {
                hart->TickOnceAndPrintDisasm(&std::cout);
            }
        } else {
            if constexpr (limit_cycles || check_events) {
                (*ticks) += hart->Tick();
            } else {
                hart->Tick();
            }
        }

        clint->Tick();

        if constexpr (check_events) {
            if ((*ticks) % event_check_freq == 0) {
                if (!eq->empty()) {
                    unsigned int event = eq->front();
                    eq->pop();
                    return event;
                }
            }
        }

        if constexpr (limit_cycles) {
            if ((*ticks) > cycle_limit) {
                return 0;
            }
        }
    }
}

template <typename MXLEN_t>
using tick_func = unsigned int (*)(Hart<MXLEN_t>*, CoreLocalInterruptor*, std::queue<unsigned int>*, std::ostream*, unsigned int*, unsigned int, unsigned int, bool);

template<typename MXLEN_t, unsigned int TickerHash>
constexpr std::array<tick_func<MXLEN_t>, 32> add_tickers(std::array<tick_func<MXLEN_t>, 32> arr) {
    if constexpr (TickerHash < 32) {
        constexpr bool limit_cycles  = TickerHash & 0b10000;
        constexpr bool check_events  = TickerHash & 0b01000;
        constexpr bool print_regs    = TickerHash & 0b00100;
        constexpr bool print_disasm  = TickerHash & 0b00010;
        constexpr bool print_details = TickerHash & 0b00001;
        arr[TickerHash] = tick_until<MXLEN_t, limit_cycles, check_events, print_regs, print_disasm, print_details>;
        return add_tickers<MXLEN_t, TickerHash + 1>(arr);
    }
    return arr;
}

template <typename MXLEN_t>
constexpr std::array<tick_func<MXLEN_t>, 32> gen_tickers() {
    std::array<tick_func<MXLEN_t>, 32> result = {0};
    result = add_tickers<MXLEN_t, 0>(result);
    return result;
}

constexpr unsigned int hash_tick_params(bool limit_cycles, bool check_events, bool print_regs, bool print_disasm, bool print_details) {
    return (limit_cycles  ? 0b10000 : 0b00000) |
           (check_events  ? 0b01000 : 0b00000) |
           (print_regs    ? 0b00100 : 0b00000) |
           (print_disasm  ? 0b00010 : 0b00000) |
           (print_details ? 0b00001 : 0b00000);
}

template <typename MXLEN_t>
void run_simulation(cxxopts::ParseResult parsed_arguments) {
    bool useRegAbiNames = true;

    std::string arg_string = "";
    if (parsed_arguments.count("args")) {
        arg_string = parsed_arguments["args"].as<std::string>();
    }

    std::string fs_root = ".";
    if (parsed_arguments.count("root")) {
        fs_root = parsed_arguments["root"].as<std::string>();
    }

    unsigned int cycle_limit = 0;
    if (parsed_arguments.count("cycles")) {
        cycle_limit = parsed_arguments["cycles"].as<unsigned int>();
    }

    std::string print_flags = "";
    if (parsed_arguments.count("print")) {
        print_flags = parsed_arguments["print"].as<std::string>();
    }
    bool print_regs = print_flags.find('r') != std::string::npos;
    bool print_disasm = print_flags.find('d') != std::string::npos;
    bool print_details = print_flags.find('+') != std::string::npos;
    bool print_summary = print_flags.find('s') != std::string::npos;
    bool print_timing = print_flags.find('t') != std::string::npos;
    bool print_syscalls = print_flags.find('c') != std::string::npos;
    bool print_mem = print_flags.find('m') != std::string::npos;

    if (!parsed_arguments.count("kernel")) {
        std::cerr << "Fatal: No kernel image provided. Nonsense run - nothing would be loaded into memory!" << std::endl;
        return;
    }

    std::string elfFileName = parsed_arguments["kernel"].as<std::string>();

    std::string dtb_filename = "";
    if (parsed_arguments.count("dtb")) {
        dtb_filename = parsed_arguments["dtb"].as<std::string>();
    }

    bool ignore_events = parsed_arguments.count("ignore-events");
    unsigned int check_events_every = 1000;
    if (parsed_arguments.count("check-events-every")) {
        check_events_every = parsed_arguments["check-events-every"].as<unsigned int>();
        if (ignore_events) {
            std::cerr << "Warning: --check-events-every is being overridden by --ignore-events, is this what you want?" << std::endl;
        }
    }

    // -- System Construction --

    Bus bus;
    std::queue<unsigned int> eq;

    IOLogger iologger(&bus, &std::cout); // TODO a locking stream
    iologger.SetPrintContents(true);

    Device *hartDevice = print_mem ? (Device*)&iologger : (Device*)&bus;

    MappedPhysicalMemory mem;
    bus.AddDevice32((Device*)&mem, 0, 0xffffffff);

    Hart<MXLEN_t> *hart;
    hart = new Hart<MXLEN_t>(hartDevice, RISCV::stringToExtensions("imacsu"));

    UART uart;
    bus.AddDevice32(&uart, 0x01000000, 0xf);
    CoreLocalInterruptor clint;
    bus.AddDevice32(&clint, 0x02000000, 0xfffff);
    const __uint32_t shutdownEvent = 0x0D15EA5E;
    PowerButton powerButton(&eq, shutdownEvent);
    bus.AddDevice32(&powerButton, 0x01000010, 0xf);

    ProxyKernelServer pkServer(hartDevice, &eq, shutdownEvent);
    pkServer.SetCommandLine(arg_string);
    pkServer.SetFSRoot(fs_root);
    if (print_syscalls) {
        pkServer.AttachLog(&std::cout);
    }

    HighELF::ElfFile elf;
    elf.Load(elfFileName);
    if (elf.status != HighELF::ElfFile::Status::Loaded) {
        std::cout << "Failed to load ELF file into memory!" << std::endl;
    }

    // -- Load the Kernel Image and Device Tree --

    for (unsigned int sid = 0; sid < elf.elfHeader.e_shnum; sid++) {

        std::cout << "ELF section #" << sid << " named \"" << elf.sections[sid].name << "\"" << std::endl;

        if (elf.sections[sid].name.compare(".htif") == 0) {
            std::cout << "    Setting up simulated device on behalf of proxy kernel" << std::endl;
            __uint64_t mask = MaskForSize(elf.sectionHeaders[sid].sh_size);
            bus.AddDevice32(&pkServer, elf.sectionHeaders[sid].sh_addr, mask);
        }

        if (elf.sections[sid].name.compare(".tohost") == 0) {
            std::cout << "    Skipping setup of simulated device requested by the proxy kernel which we have no model for" << std::endl;
        }

        if (elf.sectionHeaders[sid].sh_type == 8) {
            std::cout << "    Not loaded, type is SH_NOBITS" << std::endl;
            continue;
        }

        if (elf.sectionHeaders[sid].sh_addr == 0) {
            std::cout << "    Not loaded, section address is 0x0" << std::endl;
            continue;
        }

        std::cout << "    Loading; size 0x"
                  << std::setw(16) << std::setfill('0') << std::hex
                  << elf.sectionHeaders[sid].sh_size << " at address 0x"
                  << std::setw(16) << std::setfill('0') << std::hex
                  << elf.sectionHeaders[sid].sh_addr << std::endl;
        hartDevice->Write32(elf.sectionHeaders[sid].sh_addr,
                            elf.sectionHeaders[sid].sh_size,
                            elf.sections[sid].bytes.data());
    }

    hart->state.resetVector = elf.elfHeader.e_entry;
    hart->Reset();

    // TODO make a bytes file loader class - also this is naive and non-optimal
    if (!dtb_filename.empty()) {

        std::ifstream dtbIfStream;
        dtbIfStream.open(dtb_filename, std::ios::in | std::ios::binary);
        if (!dtbIfStream.is_open()) {
            std::cerr << "Fatal: Given a DTB file I can't open: "+dtb_filename;
            return;
        }

        dtbIfStream.seekg(0, std::ios::end);
        unsigned int size = dtbIfStream.tellg();
        dtbIfStream.seekg(0, std::ios::beg);
        char *bytes = new char[size];

        for (unsigned int i = 0; i < size; i++) {
            dtbIfStream.read(bytes, size);
        }

        dtbIfStream.close();

        std::cout << "Writing device tree into memory"
                  << " size 0x" << std::setw(16) << std::setfill('0') << std::hex << size
                  << " at address 0x" << std::setw(16) << std::setfill('0') << std::hex << 0xf0000000
                  << std::endl;
        hartDevice->Write32(0xf0000000, size, bytes);
        delete[] bytes;

        hart->state.regs[11] = 0xf0000000;
    }

    // -- Run the Simulation --

    unsigned int ticks = 0;
    unsigned int event = 0;

    constexpr std::array<tick_func<MXLEN_t>, 32> tickers = gen_tickers<MXLEN_t>();
    unsigned int tick_hash = hash_tick_params(cycle_limit > 0, check_events_every > 0, print_regs, print_disasm, print_details);
    tick_func<MXLEN_t> tick = tickers[tick_hash];

    std::cout << "Begin Simulation" << std::endl;

    auto begin = std::chrono::high_resolution_clock::now();
    event = tick(hart, &clint, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
    auto end = std::chrono::high_resolution_clock::now();

    if (print_summary) {

        if (event != 0) {
            std::cout << "Normal shutdown called with code: "
                      << "0x" << std::hex << std::setfill('0') << std::setw(8)
                      << event << std::endl;
        } else {
            std::cout << "Ran requested number of cycles and stopped."
                      << std::endl;
        }

        std::cout << "Final hart state:" << std::endl;
        PrintArchDetails<MXLEN_t>(&hart->state, &std::cout);
        PrintRegisters<MXLEN_t>(&hart->state, &std::cout, useRegAbiNames, 4);
        std::cout << std::endl;

    }

    if (print_timing) {
        unsigned long int nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        double seconds = (double)nanoseconds/1000000000.0;
        double ips = (double)ticks / seconds;
        double mips = ips / 1000000.0;

        std::cout << std::endl;
        std::cout << "Timing Summary" << std::endl;
        std::cout << "Instructions retired: " << std::dec << ticks << std::endl;
        std::cout << "Run time: " << std::dec << seconds << "s" << std::endl;
        std::cout << "MIPS: " << mips << std::endl;
    }
}

int main(int argc, char **argv) {

    // -- Parse Arguments --

    cxxopts::Options options("grim", "Generic RISC-V Interpretive Machine");
    options.add_options()
    ("x,mxlen", "Machine-mode system width, MXLEN, one of (32, 64, 128)", cxxopts::value<std::string>())
    ("m,model", "Name of the hart model to use, one of (simple, fast, threaded)", cxxopts::value<std::string>())
    ("d,dtb", "Name of the Device Tree Blob file describing the platform", cxxopts::value<std::string>())
    ("k,kernel", "Name of the ELF executable to load into the simulation", cxxopts::value<std::string>())
    ("a,args", "Argument string returned by getmainvars system call", cxxopts::value<std::string>())
    ("r,root", "Host directory to serve as the simulated file system's root", cxxopts::value<std::string>())
    ("c,cycles", "Number of cycles to run, 0 for unlimited", cxxopts::value<unsigned int>())
    ("e,check-events-every", "Number of cycles between checking the event queue", cxxopts::value<unsigned int>())
    ("p,print", "String matching /[drtcms]*/ for [d]isassembly, [r]egisters, [t]iming, system [c]alls, [m]emory transcations and a final [s]ummary", cxxopts::value<std::string>())
    ("h,help", "Print help message");

    cxxopts::ParseResult parsed_arguments = options.parse(argc, argv);

    if (parsed_arguments.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (parsed_arguments.count("mxlen")) {
        std::string xlenArgString = parsed_arguments["mxlen"].as<std::string>();
        RISCV::XlenMode mxlen = RISCV::xlenNameToMode(xlenArgString);
        switch (mxlen) {
        case RISCV::XlenMode::XL32: run_simulation<__uint32_t>(parsed_arguments); break;
        case RISCV::XlenMode::XL64: run_simulation<__uint64_t>(parsed_arguments); break;
        case RISCV::XlenMode::XL128: run_simulation<__uint128_t>(parsed_arguments); break;
        case RISCV::XlenMode::None:
        default:
            std::cerr << "Fatal: nonsense --mxlen argument "
                      << "\"" << xlenArgString << "\". Valid choices are "
                      << "(32, 64, 128)."
                      << std::endl;
            break;
        }
    } else {
        std::cerr << "Warning: no --mxlen argument, assuming 32." << std::endl;
        run_simulation<__uint32_t>(parsed_arguments);
    }
}
