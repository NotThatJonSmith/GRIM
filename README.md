# GRIM
GRIM is the Generic RISC-V Interpretive Machine

It's not quite met its goals, but:

The goal is to create the "best" RISC-V system emulator, where "best" means:

* The software supports the emulation of...
  * any conceivable legal RISC-V system according to spec, meaning:
    * All standard ISA extensions
    * All processor widths / base ISAs
    * All privilege modes
    * Custom ISA extensions (easily)
    * All interrupt and exception mechanisms described in the specs, including delegation and the emulation of the SEIP and UEIP pins, as well has hard-wiring options for these
    * All mechanisms for machine mode software to change CPU features at runtime are supported
  * any conceivable legal subset of RISC-V features, including whether the platform supports such changes at runtime
  * arbitrary platforms and peripheral models
  * systems with multiple harts
* The software can be hosted on any platform, meaning:
  * any OS where the C++ standard ways of working with files and threads are supported
  * any CPU architecture where the C++ standards needed by the project are supported, including big and little endian systems
* The software supports aggressive determinism, meaning it's possible to:
  * Run execution backward
  * Capture and dump whole-system state
  * Resume execution from captured states
* The software supports modern, powerful debugging tools through:
  * In-app hosting of a GDB server
  * RISC-V's debugging facilities
* The software can emulate platforms that can boot various flavors of Linux
* The software can self-host, so you can run the simulator inside itself
* The software run at the fastest rate it possibly can, given the above constraints. Performance target = 100 MIPS on a system like:
  * 2019 16-inch MacBook Pro
  * 2.3 GHz 8-Core Intel Core i9
  * 16 GB 2667 MHz DDR4

## TO-DO

* Get a third middle-ground hart model going, optimized but still single-thread
* Clean up an JSON-ify the trace format and create a trace comparator tool
* Establish state dumping and loading, compatible with the JSON traces
* Set up an ISA gtests project (Cardiogram)
* Actually finish implementing the whole ISA instead of just the instructions
  we've needed so far
* Finish out the core-local interruptor and establish stdin stream from UART
* Set up a gdb server inside the simulator app
* Branch-Free MMU optimization is broken
