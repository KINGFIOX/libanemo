#include <iostream>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/rv32i_cpu_system.hh>
#include <libvio/bus.hh>
#include <libvio/console.hh>
#include <libvio/mtime.hh>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: nemu-minimal <binary_file>\n";
        return 1;
    }

    libcpu::rv32i_cpu_system cpu;
    libcpu::contiguous_memory<uint32_t> memory{0x80000000, 128*1024*1024};
    memory.load_elf_from_file(argv[1]);
    cpu.instr_bus = &memory;
    cpu.data_bus = &memory;
    libvio::bus bus{{
        {new libvio::console_frontend{}, new libvio::console_backend_iostream{std::cin, std::cout}, 0xa00003f8, 8},
        {new libvio::mtime_frontend{}, new libvio::mtime_backend_chrono{}, 0xa0000048, 16}
    }};
    cpu.mmio_bus = &bus;
    cpu.reset(0x80000000);

    while (!cpu.stopped()) {
        cpu.next_instruction();
        bus.next_cycle();
    }

    return cpu.get_gpr(10);
}
