#include <libvio/nemu_interface.hh>
#include <libcpu/rv32i_cpu_nemu.hh>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: nemu-minimal <binary_file>\n";
        return 1;
    }

    libcpu::rv32i_cpu_nemu cpu{4096, 128*1024*1024, &libvio::nemu_bus::get_instance()};

    std::ifstream file(argv[1], std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(cpu.memory.data()), file_size);

    while (!cpu.get_ebreak()) {
        cpu.next_instruction();
    }

    return cpu.gpr_read(10);
}
