#include <cstdint>
#include <iostream>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/event.hh>
#include <libcpu/riscv_cpu.hh>
#include <libcpu/rv32i_cpu_system.hh>
#include <libcpu/difftest.hh>
#include <libsdb/sdb.hh>
#include <libvio/bus.hh>
#include <libvio/console.hh>
#include <libvio/mtime.hh>
#include <libvio/ringbuffer.hh>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: nemu-minimal <binary_file>\n";
        return 1;
    }

    libvio::io_dispatcher bus{{
        {new libvio::console_frontend{}, new libvio::console_backend_iostream{std::cin, std::cout}, 0xa00003f8, 8},
        {new libvio::mtime_frontend{}, new libvio::mtime_backend_chrono{}, 0xa0000048, 16}
    }};

    libcpu::riscv_cpu<uint32_t> dut;
    libcpu::contiguous_memory<uint32_t> memory_dut{0x80000000, 128*1024*1024};
    memory_dut.load_elf_from_file(argv[1]);
    dut.instr_bus = &memory_dut;
    dut.data_bus = &memory_dut;
    dut.mmio_bus = bus.new_agent();
    libvio::ringbuffer<libcpu::event_t<uint32_t>> events_dut{4096};
    dut.event_buffer = &events_dut;

    libcpu::rv32i_cpu_system ref;
    libcpu::contiguous_memory<uint32_t> memory_ref{0x80000000, 128*1024*1024};
    memory_ref.load_elf_from_file(argv[1]);
    ref.instr_bus = &memory_ref;
    ref.data_bus = &memory_ref;
    ref.mmio_bus = bus.new_agent();
    libvio::ringbuffer<libcpu::event_t<uint32_t>> events_ref{4096};
    ref.event_buffer = &events_ref;

    libcpu::simple_difftest<uint32_t> difftest{};
    libvio::ringbuffer<libcpu::event_t<uint32_t>> events_difftest{4096};
    difftest.dut = &dut;
    difftest.ref = &ref;
    difftest.event_buffer = &events_difftest;
    difftest.reset(0x80000000);

    libsdb::sdb<uint32_t> sdb {};
    sdb.cpu = &difftest;

    while (!sdb.stopped()) {
        std::cout << "sdb> ";
        std::string cmd;
        std::getline(std::cin, cmd);
        sdb.execute_command(cmd);
    }

    sdb.execute_command("status");
    return difftest.get_gpr(10);
}
