/**
 * @file A minimal RISC-V emulator based on the emulator backend.
 *
 * The purpose of this file is showing the usage and internals of the emulator
 * backend. The backend itself is not compliant with the `abstract_cpu` API. The
 * backend may have breaking changes within a single major version. This file
 * assumes the same memory mapping with NEMU.
 *
 */
#include <cassert>
#include <iostream>
#include <libcpu/memory.hh>
#include <libcpu/riscv/decode_cache.hh>
#include <libcpu/riscv/privilege_module.hh>
#include <libcpu/riscv/riscv.hh>
#include <libcpu/riscv/user_core.hh>
#include <libvio/bus.hh>
#include <libvio/console.hh>
#include <libvio/mtime.hh>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <elf_file>\n";
    return 1;
  }

  using word_t = uint32_t;

  libcpu::riscv::user_core<word_t> user_core;
  libcpu::riscv::privilege_module<word_t> privilege_module;

  libcpu::memory memory{0x80000000, 128 * 1024 * 1024};
  memory.load_elf_from_file(argv[1]);
  privilege_module.mem_bus = &memory;

  libvio::io_dispatcher bus{
      {{new libvio::console_frontend{},
        new libvio::console_backend_iostream{std::cin, std::cout}, 0xa00003f8,
        8},
       {new libvio::mtime_frontend{}, new libvio::mtime_backend_chrono{},
        0xa0000048, 16}}};
  privilege_module.mmio_bus = bus.new_agent();

  libcpu::riscv::decode_cache<word_t, 24, 2> decode_cache;

  libcpu::riscv::exec_result_t<word_t> exec_result;
  exec_result.pc = 0x80000000;
  user_core.reset();
  privilege_module.reset();
  while (true) {
    privilege_module.paddr_fetch_instruction(exec_result);

    if (exec_result.type == libcpu::riscv::exec_result_type_t::fetch) {
      // libcpu::riscv::user_core<word_t>::decode(exec_result);
      decode_cache.decode(exec_result);
    }

    if (exec_result.type == libcpu::riscv::exec_result_type_t::decode) {
      user_core.execute(exec_result);
    }

    // Do privileged operations
    if (exec_result.type == libcpu::riscv::exec_result_type_t::load) {
      privilege_module.paddr_load(exec_result);
    } else if (exec_result.type == libcpu::riscv::exec_result_type_t::store) {
      privilege_module.paddr_store(exec_result);
    } else if (exec_result.type == libcpu::riscv::exec_result_type_t::csr_op) {
      privilege_module.csr_op(exec_result);
    } else if (exec_result.type == libcpu::riscv::exec_result_type_t::sys_op) {
      privilege_module.sys_op(exec_result);
    }

    if (exec_result.type == libcpu::riscv::exec_result_type_t::trap) {
      if (exec_result.trap.cause ==
          libcpu::riscv::mcause<word_t>::except_breakpoint) {
        break;
      } else {
        privilege_module.handle_exception(exec_result);
      }
    }

    assert(exec_result.type == libcpu::riscv::exec_result_type_t::retire);

    if (exec_result.retire.rd != 0) {
      user_core.gpr[exec_result.retire.rd] = exec_result.retire.value;
    }
    exec_result.pc = exec_result.next_pc;
  }

  return 0;
}