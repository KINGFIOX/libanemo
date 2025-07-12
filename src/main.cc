#include <cstdint>
#include <libcpu/cache.hh>
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
    libcpu::direct_cache<uint32_t, 4, 6> icache;
    libcpu::direct_cache<uint32_t, 8, 8> dcache;
    icache.underlying_memory = &memory;
    dcache.underlying_memory = &memory;
    cpu.instr_bus = &icache;
    cpu.data_bus = &dcache;
    cpu.mmio_bus = &libvio::nemu_bus::get_instance();
    cpu.reset(0x80000000);

    while (!cpu.stopped()) {
        cpu.next_instruction();
    }

    return cpu.get_gpr(10);
}
