#include <iostream>
#include <chrono>
#include <array>

#include <cxxopts.hpp>
#include <ElfFile.hpp>

#include <Schedule.hpp>
#include <Bus.hpp>
#include <IOLogger.hpp>
#include <MappedPhysicalMemory.hpp>
#include <CoreLocalInterruptor.hpp>
#include <PowerButton.hpp>
#include <ProxyKernelServer.hpp>
#include <ToHostInstrument.hpp>
#include <UART.hpp>
#include <SimpleHart.hpp>
#include <OptimizedHart.hpp>


__uint64_t MaskForSize(__uint64_t size) {
    if (size > (__uint64_t)1 << 63) // TODO this assumes Address is 64 bit
        return ~(__uint64_t)0;
    __uint64_t nextPo2 = 1;
    for (nextPo2 = 1; nextPo2 < size; nextPo2 <<= 1);
    return nextPo2 - 1;
}

template<typename XLEN_t, bool print_regs, bool print_disasm, bool print_details>
void PrintState(HartState<XLEN_t>* state, std::ostream* out, bool abi, unsigned int regsPerLine) {

    if constexpr (print_details) {
        (*out) << "Details:" << std::endl;
        (*out) << "| misa: rv" << RISCV::xlenModeName(state->misa.mxlen)
               << RISCV::extensionsToString(state->misa.extensions)
               << std::endl;
        (*out) << "| Privilege=" << RISCV::privilegeModeName(state->privilegeMode)
               << std::endl;
        (*out) << "| mstatus: "
               << "mprv=" << (state->mstatus.mprv ? "1 " : "0 ")
               << "sum=" << (state->mstatus.sum ? "1 " : "0 ")
               << "tvm=" << (state->mstatus.tvm ? "1 " : "0 ")
               << "tw=" << (state->mstatus.tw ? "1 " : "0 ")
               << "tsr=" << (state->mstatus.tsr ? "1 " : "0 ")
               << "sd=" << (state->mstatus.sd ? "1 " : "0 ")
               << std::endl << "| "
               << "fs=" << RISCV::floatingPointStateName(state->mstatus.fs)
               << " xs= " << RISCV::extensionStateName(state->mstatus.xs)
               << " sxl=" << RISCV::xlenModeName(state->mstatus.sxl)
               << " uxl=" << RISCV::xlenModeName(state->mstatus.uxl)
               << std::endl << "| "
               << " mie=" << (state->mstatus.mie ? "1" : "0")
               << " mpie=" << (state->mstatus.mpie ? "1" : "0")
               << " mpp=" << RISCV::privilegeModeName(state->mstatus.mpp)
               << " sie=" << (state->mstatus.sie ? "1" : "0")
               << " spie=" << (state->mstatus.spie ? "1" : "0")
               << " spp=" << RISCV::privilegeModeName(state->mstatus.spp)
               << " uie=" << (state->mstatus.uie ? "1" : "0")
               << " upie=" << (state->mstatus.upie ? "1" : "0")
               << std::endl;
        (*out) << "| satp: mode=" << RISCV::pagingModeName(state->satp.pagingMode)
               << ", ppn=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->satp.ppn << ", asid=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->satp.asid
               << std::endl;
        (*out) << "| mie=[ "
               << (state->mie.mei ? "mei " : "")
               << (state->mie.msi ? "msi " : "")
               << (state->mie.mti ? "mti " : "")
               << (state->mie.sei ? "sei " : "")
               << (state->mie.ssi ? "ssi " : "")
               << (state->mie.sti ? "sti " : "")
               << (state->mie.uei ? "uei " : "")
               << (state->mie.usi ? "usi " : "")
               << (state->mie.uti ? "uti " : "")
               << "] mip=[ "
               << (state->mip.mei ? (!state->mie.mei ? "*mei " : "mei ") : "")
               << (state->mip.msi ? (!state->mie.msi ? "*msi " : "msi ") : "")
               << (state->mip.mti ? (!state->mie.mti ? "*mti " : "mti ") : "")
               << (state->mip.sei ? (!state->mie.sei ? "*sei " : "sei ") : "")
               << (state->mip.ssi ? (!state->mie.ssi ? "*ssi " : "ssi ") : "")
               << (state->mip.sti ? (!state->mie.sti ? "*sti " : "sti ") : "")
               << (state->mip.uei ? (!state->mie.uei ? "*uei " : "uei ") : "")
               << (state->mip.usi ? (!state->mie.usi ? "*usi " : "usi ") : "")
               << (state->mip.uti ? (!state->mie.uti ? "*uti " : "uti ") : "")
               << "]" << std::endl;
        (*out) << "| mideleg=[ "
               << ((state->mideleg & RISCV::meiMask) ? "mei " : "")
               << ((state->mideleg & RISCV::msiMask) ? "msi " : "")
               << ((state->mideleg & RISCV::mtiMask) ? "mti " : "")
               << ((state->mideleg & RISCV::seiMask) ? "sei " : "")
               << ((state->mideleg & RISCV::ssiMask) ? "ssi " : "")
               << ((state->mideleg & RISCV::stiMask) ? "sti " : "")
               << ((state->mideleg & RISCV::ueiMask) ? "uei " : "")
               << ((state->mideleg & RISCV::usiMask) ? "usi " : "")
               << ((state->mideleg & RISCV::utiMask) ? "uti " : "")
               << "] sideleg=[ "
               << ((state->sideleg & RISCV::meiMask) ? "mei " : "")
               << ((state->sideleg & RISCV::msiMask) ? "msi " : "")
               << ((state->sideleg & RISCV::mtiMask) ? "mti " : "")
               << ((state->sideleg & RISCV::seiMask) ? "sei " : "")
               << ((state->sideleg & RISCV::ssiMask) ? "ssi " : "")
               << ((state->sideleg & RISCV::stiMask) ? "sti " : "")
               << ((state->sideleg & RISCV::ueiMask) ? "uei " : "")
               << ((state->sideleg & RISCV::usiMask) ? "usi " : "")
               << ((state->sideleg & RISCV::utiMask) ? "uti " : "")
               << "]" << std::endl;
        (*out) << "| medeleg=[ "
               << "TODO "
               << "] sedeleg=[ "
               << "TODO "
               << "]" << std::endl;
        (*out) << "| mtval=0x"
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->mtval;
        (*out) << " mscratch=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->mscratch;
        (*out) << " mepc=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->mepc
               << std::endl;
        (*out) << "| mtvec=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->mtvec.base
               << " (" << RISCV::tvecModeName(state->mtvec.mode) << ")"
               << " mcause=" << RISCV::trapName(state->mcause.interrupt, state->mcause.exceptionCode)
               << std::endl;
        (*out) << "| stval=0x"
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->stval;
        (*out) << " sscratch=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->sscratch;
        (*out) << " sepc=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->sepc
               << std::endl;
        (*out) << "| stvec=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->stvec.base
               << " (" << RISCV::tvecModeName(state->stvec.mode) << ")"
               << " scause=" << RISCV::trapName(state->scause.interrupt, state->scause.exceptionCode)
               << std::endl;
        (*out) << "| utval=0x"
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->utval;
        (*out) << " uscratch=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->uscratch;
        (*out) << " uepc=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->uepc
               << std::endl;
        (*out) << "| utvec=0x" 
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->utvec.base
               << " (" << RISCV::tvecModeName(state->utvec.mode) << ")"
               << " ucause=" << RISCV::trapName(state->ucause.interrupt, state->ucause.exceptionCode)
               << std::endl;

        // TODO FP state

        // TODO counters and events

        // TODO PMP

    }

    if constexpr (print_regs) {
        (*out) << "Registers:" << std::endl;
        for (unsigned int i = 0; i < 32; i++) {
            if (i % regsPerLine == 0) {
                (*out) << "| ";
            }
            if (abi) {
                (*out) << std::setfill(' ') << std::setw(4)
                    << RISCV::registerAbiNames[i] << ": "
                    << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                    << state->regs[i];
            } else {
                (*out) << std::dec << std::setfill(' ') << std::setw(2) << i << ": "
                    << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
                    << state->regs[i];
            }
            if ((i+1) % regsPerLine == 0) {
                std::cout << std::endl;
            } else {
                std::cout << "  ";
            }
        }
    }

    if constexpr (print_disasm) {
        (*out) << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->currentFetch->virtualPC << ":\t"
               << std::hex << std::setfill('0') << std::setw(sizeof(XLEN_t)*2)
               << state->currentFetch->encoding << "\t"
               << std::dec;
        state->currentFetch->instruction.disassemble(state->currentFetch->operands, out);
    }

}

// Note: These parameters could be constexpr-if'd but that breaks pedantry, and
//       the compiler does the right thing in O3.
template <bool limit_cycles, bool check_events, bool print_regs, bool print_disasm, bool print_details>
__uint32_t tick_until(
        CASK::Tickable *sched,
        Hart<__uint32_t> *disasm_hart,
        CASK::EventQueue *eq,
        std::ostream *out,
        unsigned int *ticks,
        unsigned int cycle_limit,
        unsigned int event_check_freq,
        bool useRegAbiNames) {

    while (true) {

        PrintState<__uint32_t, print_regs, print_disasm, print_details>(&disasm_hart->state, out, useRegAbiNames, 4);

        sched->Tick();

        if (limit_cycles || check_events) {
            (*ticks)++;
        }

        if (check_events) {
            // TODO ServiceInterrupts about here?
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

typedef unsigned int (*tick_func)(CASK::Tickable*, Hart<__uint32_t>*, CASK::EventQueue*, std::ostream*, unsigned int*, unsigned int, unsigned int, bool);

template<unsigned int TickerHash>
constexpr std::array<tick_func, 32> add_tickers(std::array<tick_func, 32> arr) {
    if constexpr (TickerHash < 32) {
        constexpr bool limit_cycles  = TickerHash & 0b10000;
        constexpr bool check_events  = TickerHash & 0b01000;
        constexpr bool print_regs    = TickerHash & 0b00100;
        constexpr bool print_disasm  = TickerHash & 0b00010;
        constexpr bool print_details = TickerHash & 0b00001;
        arr[TickerHash] = tick_until<limit_cycles, check_events, print_regs, print_disasm, print_details>;
        return add_tickers<TickerHash + 1>(arr);
    }
    return arr;
}

constexpr std::array<tick_func, 32> gen_tickers() {
    std::array<tick_func, 32> result = {0};
    result = add_tickers<0>(result);
    return result;
}

constexpr std::array<tick_func, 32> tickers = gen_tickers();

constexpr unsigned int hash_tick_params(bool limit_cycles, bool check_events, bool print_regs, bool print_disasm, bool print_details) {
    return (limit_cycles  ? 0b10000 : 0b00000) |
           (check_events  ? 0b01000 : 0b00000) |
           (print_regs    ? 0b00100 : 0b00000) |
           (print_disasm  ? 0b00010 : 0b00000) |
           (print_details ? 0b00001 : 0b00000);
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
    bool print_details = print_flags.find('+') != std::string::npos;
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

    Hart<__uint32_t>* hart = nullptr;
    if (use_fast_model) {
        hart = new OptimizedHart<__uint32_t, 8, true>(hartIOTarget, (CASK::IOTarget*)&mem, RISCV::stringToExtensions("imacsu"));
    } else {
        hart = new SimpleHart<__uint32_t>(hartIOTarget, RISCV::stringToExtensions("imacsu"));
    }

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

        hart->state.regs[11] = 0xf0000000;
    }

    // -- Run the Simulation --

    unsigned int ticks = 0;
    unsigned int event = 0;

    hart->resetVector = elf.elfHeader.e_entry;

    sched.BeforeFirstTick();

    unsigned int tick_hash = hash_tick_params(cycle_limit > 0, check_events_every > 0, print_regs, print_disasm, print_details);
    tick_func tick = tickers[tick_hash];

    auto begin = std::chrono::high_resolution_clock::now();
    event = tick(&sched, hart, &eq, &std::cout, &ticks, cycle_limit, check_events_every, useRegAbiNames);
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
        PrintState<__uint32_t, true, true, true>(&hart->state, &std::cout, useRegAbiNames, 4);
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
