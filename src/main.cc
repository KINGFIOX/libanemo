#include <cstdint>
#include <libvio/nemu_interface.hh>
#include <libcpu/rv32i_cpu_system.hh>
#include <libcpu/memory.hh>
#include <iostream>

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
    cpu.mmio_bus = &libvio::nemu_bus::get_instance();
    cpu.reset(0x80000000);

    while (!cpu.stopped()) {
        cpu.next_instruction();
    }

    return cpu.get_gpr(10);
}
