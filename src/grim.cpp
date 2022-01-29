#include <iostream>
#include <chrono>
#include <array>

#include <cxxopts.hpp>
#include <ElfFile.hpp>

#include <Schedule.hpp>
#include <Bus.hpp>
#include <IOLogger.hpp>
#include <MappedPhysicalMemory.hpp>
#include <PhysicalMemory.hpp> // TODO remove - in for a test
#include <CoreLocalInterruptor.hpp>
#include <PowerButton.hpp>
#include <ProxyKernelServer.hpp>
#include <ToHostInstrument.hpp>
#include <UART.hpp>
#include <SimpleHart.hpp>
#include <OptimizedHart.hpp>

#include <CacheWrappedTranslator.hpp>


__uint64_t MaskForSize(__uint64_t size) {
    if (size > (__uint64_t)1 << 63) // TODO this assumes Address is 64 bit
        return ~(__uint64_t)0;
    __uint64_t nextPo2 = 1;
    for (nextPo2 = 1; nextPo2 < size; nextPo2 <<= 1);
    return nextPo2 - 1;
}

template<typename XLEN_t>
std::string DisassembleNext(HartState::Fetch* fetch) {
    std::stringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
            << fetch->virtualPC.Read<XLEN_t>() << ":\t"
            << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
            << fetch->encoding << "\t"
            << std::dec;
    fetch->instruction.disassemble(fetch->operands, &stream);
    return stream.str();
}

template<typename XLEN_t>
void PrintRegs(HartState* state, std::ostream* out, bool abi=false, unsigned int regsPerLine=8) {

    (*out) << "Registers:" << std::endl;

    for (unsigned int i = 0; i < 32; i++) {

        if (i % regsPerLine == 0) {
            (*out) << "| ";
        }

        if (abi) {
            (*out) << RISCV::registerAbiNames[i] << ": "
                    << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                    << state->regs[i].Read<XLEN_t>();
        } else {
            (*out) << std::dec << std::setfill(' ') << std::setw(2) << i << ": "
                    << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                    << state->regs[i].Read<XLEN_t>();
        }

        if ((i+1) % regsPerLine == 0) {
            std::cout << std::endl;
        } else {
            std::cout << "  ";
        }
    }

}

// Note: These parameters could be constexpr-if'd but that breaks pedantry, and the compiler does the right thing in O3.
template <bool limit_cycles, bool check_events, bool print_regs, bool print_disasm>
__uint32_t tick_until(
        CASK::Tickable *sched,
        Hart *disasm_hart,
        CASK::EventQueue *eq,
        std::ostream *out,
        unsigned int *ticks,
        unsigned int cycle_limit,
        unsigned int event_check_freq,
        bool useRegAbiNames) {

    while (true) {

        if (print_disasm) {
            (*out) << DisassembleNext<__uint32_t>(disasm_hart->state.currentFetch);
        }

        sched->Tick();

        if (print_regs) {
            PrintRegs<__uint32_t>(&disasm_hart->state, out, useRegAbiNames);
        }

        if (limit_cycles || check_events) {
            (*ticks)++;
        }

        if (check_events) {
            if ((*ticks) % event_check_freq == 0) {
                if (!eq->IsEmpty()) {
                    return eq->DequeueEvent();
                }
            }
        }

        if (limit_cycles) {
            if ((*ticks) > cycle_limit) {
                return 0;
            }
        }
    }
}

int main(int argc, char **argv) {
    
    // -- Parse Arguments --

    cxxopts::Options options("grim", "Generic RISC-V Interpretive Machine");
    options.add_options()
    ("d,dtb", "Name of the Device Tree Blob file describing the platform", cxxopts::value<std::string>())
    ("f,fast", "Use the fast, risky model instead of the slow, sure model")
    ("k,kernel", "Name of the ELF executable to load into the simulation", cxxopts::value<std::string>())
    ("a,args", "Argument string returned by getmainvars system call", cxxopts::value<std::string>())
    ("r,root", "Host directory to serve as the simulated file system's root", cxxopts::value<std::string>())
    ("c,cycles", "Number of cycles to run, 0 for unlimited", cxxopts::value<unsigned int>())
    ("e,check-events-every", "Number of cycles between checking the event queue", cxxopts::value<unsigned int>())
    ("p,print", "String matching /[drtcms]*/ for [d]isassembly, [r]egisters, [t]iming, system [c]alls, [m]emory transcations and a final [s]ummary", cxxopts::value<std::string>())
    ("h,help", "Print help message");

    auto parsed_arguments = options.parse(argc, argv);

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
    bool print_summary = print_flags.find('s') != std::string::npos;
    bool print_timing = print_flags.find('t') != std::string::npos;
    bool print_syscalls = print_flags.find('c') != std::string::npos;
    bool print_mem = print_flags.find('m') != std::string::npos;

    if (parsed_arguments.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (!parsed_arguments.count("kernel")) { 
        std::cerr << "Warning: No kernel image provided." << std::endl;
        return 0;
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

    bool use_fast_model = false;
    if (parsed_arguments.count("fast")) {
        use_fast_model = true;
    }

    // -- System Construction --

    CASK::Bus bus;
    CASK::EventQueue eq;

    CASK::IOLogger iologger(&bus, &std::cout); // TODO a locking stream
    iologger.SetPrintContents(true);

    CASK::IOTarget *hartIOTarget = print_mem ? (CASK::IOTarget*)&iologger : (CASK::IOTarget*)&bus;

    CASK::MappedPhysicalMemory mem;
    bus.AddIOTarget32(&mem, 0, 0xffffffff);

    Hart* hart = nullptr;
    if (use_fast_model) {
        hart = new OptimizedHart<true, false, true, true, 8, 0, 1>(hartIOTarget, &mem);
    } else {
        hart = new SimpleHart(hartIOTarget);
    }

    hart->spec.SetExtensionSupport("imacsu");
    hart->spec.SetWidthSupport(RISCV::XlenMode::XL32, true);
    hart->spec.SetWidthSupport(RISCV::XlenMode::XL64, false);
    hart->spec.SetWidthSupport(RISCV::XlenMode::XL128, false);

    CASK::UART uart;
    bus.AddIOTarget32(&uart, 0x01000000, 0xf);

    CASK::CoreLocalInterruptor clint;
    bus.AddIOTarget32(&clint, 0x02000000, 0xfffff);

    CASK::Schedule sched;
    sched.AddTickable(hart);
    sched.AddTickable(&clint);

    const __uint32_t shutdownEvent = 0x0D15EA5E;

    CASK::PowerButton powerButton(&eq, shutdownEvent);
    bus.AddIOTarget32(&powerButton, 0x01000010, 0xf);

    CASK::ProxyKernelServer pkServer(&bus, &eq, shutdownEvent);
    pkServer.SetCommandLine(arg_string);
    pkServer.SetFSRoot(fs_root);
    if (print_syscalls) {
        pkServer.AttachLog(&std::cout);
    }

    CASK::ToHostInstrument toHost(&eq, shutdownEvent);

    HighELF::ElfFile elf;
    elf.Load(elfFileName);
    if (elf.status != HighELF::ElfFile::Status::Loaded) {
        std::cout << "Failed to load ELF file into memory!" << std::endl;
    }

    for (unsigned int sid = 0; sid < elf.elfHeader.e_shnum; sid++) {
        
        if (elf.sections[sid].name.compare(".htif") == 0) {
            __uint64_t mask = MaskForSize(elf.sectionHeaders[sid].sh_size);
            bus.AddIOTarget32(&pkServer, elf.sectionHeaders[sid].sh_addr, mask);
            continue;
        }

        if (elf.sections[sid].name.compare(".tohost") == 0) {
            __uint64_t mask = MaskForSize(elf.sectionHeaders[sid].sh_size);
            bus.AddIOTarget32(&toHost, elf.sectionHeaders[sid].sh_addr, mask);
            continue;
        }
    }

    // -- Load the Kernel Image and Device Tree --

    for (unsigned int sid = 0; sid < elf.elfHeader.e_shnum; sid++) {

        if (elf.sectionHeaders[sid].sh_type == 8) {
            continue;
        }

        if (elf.sectionHeaders[sid].sh_addr == 0) {
            continue;
        }
        
        bus.Write32(elf.sectionHeaders[sid].sh_addr,
                    elf.sectionHeaders[sid].sh_size,
                    elf.sections[sid].bytes.data());
    }

    // TODO make a bytes file loader class - also this is naive and non-optimal
    if (!dtb_filename.empty()) {

        std::ifstream dtbIfStream;
        dtbIfStream.open(dtb_filename, std::ios::in | std::ios::binary);
        if (!dtbIfStream.is_open()) {
            std::cerr << "Can't open DTB file "+dtb_filename;
            return 0;
        }

        dtbIfStream.seekg(0, std::ios::end);
        unsigned int size = dtbIfStream.tellg();
        dtbIfStream.seekg(0, std::ios::beg);
        char *bytes = new char[size];

        for (unsigned int i = 0; i < size; i++) {
            dtbIfStream.read(bytes, size);
        }

        dtbIfStream.close();
        
        bus.Write32(0xf0000000, size, bytes);
        delete[] bytes;

        hart->state.regs[11].Write<__uint32_t>(0xf0000000);
    }

    // -- Run the Simulation --

    unsigned int ticks = 0;
    unsigned int event = 0;

    hart->spec.resetVector.Write<__uint32_t>(elf.elfHeader.e_entry);

    sched.BeforeFirstTick();

    auto begin = std::chrono::high_resolution_clock::now();

    // TODO clean this mess up
    if (cycle_limit != 0) {
        if (ignore_events) {
            if (print_disasm) {
                if (print_regs) {
                    event = tick_until<true, false, true, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<true, false, false, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            } else {
                if (print_regs) {
                    event = tick_until<true, false, true, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<true, false, false, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            }
        } else {
            if (print_disasm) {
                if (print_regs) {
                    event = tick_until<true, true, true, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<true, true, false, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            } else {
                if (print_regs) {
                    event = tick_until<true, true, true, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<true, true, false, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            }
        } 
    } else {
        if (ignore_events) {
            if (print_disasm) {
                if (print_regs) {
                    event = tick_until<false, false, true, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<false, false, false, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            } else {
                if (print_regs) {
                    event = tick_until<false, false, true, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<false, false, false, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            }
        } else {
            if (print_disasm) {
                if (print_regs) {
                    event = tick_until<false, true, true, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<false, true, false, true>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            } else {
                if (print_regs) {
                    event = tick_until<false, true, true, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                } else {
                    event = tick_until<false, true, false, false>(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
                }
            }
        }
    }

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

        std::cout << "Final register state:" << std::endl;
        PrintRegs<__uint32_t>(&hart->state, &std::cout, useRegAbiNames);
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

    return 0;
}
