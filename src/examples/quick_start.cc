#include <cstddef>
#include <cstdint>
#include <iostream>
#include <libcpu/memory.hh>
#include <libcpu/riscv_cpu_system.hh>
#include <libsdb/sdb.hh>
#include <libvio/bus.hh>
#include <libvio/console.hh>
#include <libvio/mtime.hh>
#include <libvio/ringbuffer.hh>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: nemu-minimal <binary_file>\n";
        return 1;
    }

    libvio::io_dispatcher bus{{
        {new libvio::console_frontend{}, new libvio::console_backend_iostream{std::cin, std::cout}, 0xa00003f8, 8},
        {new libvio::mtime_frontend{}, new libvio::mtime_backend_chrono{}, 0xa0000048, 16}
    }};

    using word_t = uint32_t;

    libcpu::riscv_cpu_system<word_t> cpu;
    libcpu::memory memory{0x80000000, 128*1024*1024};
    memory.load_elf_from_file(argv[1]);
    cpu.instr_bus = &memory;
    cpu.data_bus = &memory;
    cpu.mmio_bus = bus.new_agent();
    libvio::ringbuffer<libcpu::event_t<word_t>> events{4096};
    cpu.event_buffer = &events;
    cpu.reset(0x80000000);

    libsdb::sdb<word_t> sdb {};
    sdb.cpu = &cpu;

    for (size_t i=2; i<argc; ++i) {
        sdb.execute_command(argv[i]);
    }

    while (!sdb.stopped()) {
        std::cout << sdb.get_prompt();
        std::string cmd;
        std::getline(std::cin, cmd);
        sdb.execute_command(cmd);
    }

    sdb.execute_command("status");
    return 0;
}
