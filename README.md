# `libanemo` - A Universal CPU Simulator and Differential Testing Framework

> Still, the winds change direction.
>
> Someday, they will blow towards a brighter future...
>
> Take my blessings and live leisurely from this day onward.

## Overview

`libanemo` is a library made to provide a unified, portable and graceful collection of tools for designing processors. It is designed with extendable API and differential testing support in mind. This library consists of these parts:

- `libvio`: Simulated peripherals with a unified interface and differential testing support.
- `libcpu`: Simulated CPU cores with a unified interface.
- `libsdb`: A simple GDB-like command-line interface.

## Getting Started

### Building

The quickest way to use this library is including it as a CMake subdirectory in your project and link your code against the static library produced. If you are not using CMake, simply compile all the sources and add `include` into the header path with your build system. This project does not require any external libraries currently. You can copy `elf.h` from a Linux system to `include` if your system does not have it.

### Using Virtual MMIO

`libvio` consists of 3 major parts:

- IO frontends: resolve the MMIO requests.
- IO backends: do the actual input and output.
- MMIO bus: dispatch MMIO request to a set of managed frontends.

Different IO backends can be implemented for the same type of virtual device. For example, a simple iostream based console IO backend is implemented in this library. And you can also implement another console IO backend that connects the virtual console of the simulated processor to a physical serial port of your computer. Both backends work with the same frontend.

To use `libvio` for simulated MMIO, simply initialize a `libvio::bus` with specified IO frontend, IO backend, starting address and address range size of each virtual device.

```c++
libvio::bus bus{{
    {new libvio::console_frontend{}, new libvio::console_backend_iostream{std::cin, std::cout}, 0xa00003f8, 8},
    {new libvio::mtime_frontend{}, new libvio::mtime_backend_chrono{}, 0xa0000048, 16}
}};
```

Then you can use `bus.read()` and `bus.write()` to simulate MMIO operations. You need to call `bus.next_cycle()` after each cycle of the simulated system.

### Simulating a CPU

`libcpu` provides the `abstract_cpu` base class for a simulated processor core, and the `abstract_memory` base class for simulated memory. To simulate a processor, instantiate a processor core, a memory, initialize the memory with proper content, and connect the memory to the memory ports of the CPU core.

```c++
libcpu::rv32i_cpu_system cpu;

libcpu::contiguous_memory<uint32_t> memory{0x80000000, 128*1024*1024};
memory.load_elf_from_file(argv[1]);

cpu.instr_bus = &memory;
cpu.data_bus = &memory;
```

Optionally connect an MMIO bus to the processor. MMIO requests are ignored if no MMIO bus is connected.

```c++
cpu.mmio_bus = &bus;
```

You can connect the `instr_bus` and `data_bus` to the same memory, or different caches with the same underlaying memory, or even different memories if the processor uses different address space to access instruction and data.

Then reset the CPU with specified initial program counter with `reset()`. Then you can step the CPU forward with `next_instruction()` or `next_cycle()`, and check whether it has stopped with `stopped`.

```c++
cpu.reset(0x80000000);
while (!cpu.stopped()) {
    cpu.next_cycle();
    bus.next_cycle();
}
```

`libcpu` provides `event_t<WORD_T>` in `libcpu/event.hh` describing an architectural event, for example, writing to a register and a memory operation. To enable event tracing, attach a `libvio::ringbuffer<event_t<WORT_T>>` to the CPU core. The events will be automatically put into the ring-buffer if supported.

## Accessing a Simple Debugging Command-line

`libsdb` provides a template class `sdb<WORD_T>` that provides a simple command-line interface for debugging.

```c++
libsdb::sdb<uint32_t> sdb {};
sdb.cpu = &cpu;

while (!sdb.stopped()) {
    std::cout << "sdb> ";
    std::string cmd;
    std::getline(std::cin, cmd);
    sdb.execute_command(cmd);
}
```

The command format supports:

- Space-separated tokens (quoted sections preserve spaces)
- Escape sequences (`\` for literal characters)
- Double-quoted strings (`"..."` for multi-word arguments)
- Piped output (`sdb_command | shell_command`)

For example:

```
break rm 0
eval "sp + a0"
help | less
trace instr | "tee npc.log"
```

Use the `help` command or refer to `include/libsdb/sdb.hh` for a list of available commands.

## Behavior Based Differential Testing

Instead of comparing the state of CPUs, this library uses a different approach for differential testing, where it is the behavior of each instruction that is compared. Or to say in another way, instead of comparing the values of the registers, CSRs, etc., it compares how an instruction modifies the value of the registers, what memory operation it has done, etc. The reasons why comparing behavior are that:

- For pipeline and superscalar processors, there may not be a concrete "architectural state".
- Instructions can be executed out of order, but they are committed in order. The intermediate state of the processor might be utter chaos, but the behaviors of the instructions on being committed are strictly in order and well-defined.
- In this way of testing, the reference design does not need to align with the DUT in a cycle-precise way. You can even test an out-of-order design with a single-cycle reference, as long as the behavior of the reference is correct.
- Comparing the entire state of the processor can be expensive in terms of performance.
- In this way, comparing can be done semi-offline, or even in another thread.

`libanemo` is designed with supporting this way of differential testing in mind. `libvio` is designed 

## TODO List

- Interrupt support for `libvio`.
- RV64I simulator.
- Transforming the instruction stream into a static single assignment form for performance analysis.
- C API.
- More types of virtual devices for `libvio`.
- SDL based backends for `libvio` as in `nvboard`.
- Supporting differential testing between a processor with peripherals simulated by software and peripherals implemented in RTL.

## Notes on Academic Integrity

The author of this library has participated in the 一生一芯 program, and some of the code is refactored from the author's assignments. It is okay to make this library public because it is mostly implemented in a very different way from the official assignments of that program, as it is intended to be more portable than the code in the assignments. You may use this library as a tool assisting your assignments, for example:

- Using the CPU emulator as a differential testing reference.
- Using `libvio` as an alternative MMIO framework for NEMU.
- Using part of this project in your workbench as long as its functionality is not required to be implemented by yourself.

If some assignment requires to be completed on yourself, you may not:

- Copy any code from this project, other than trivial constants and definitions, into your assignments.
- Completing your assignments by integrating this library into your code.
- Reimplement anything in your assignment in the exact same way of this project. 
