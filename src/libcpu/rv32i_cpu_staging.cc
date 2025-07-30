#include <cstdint>
#include <cassert>
#include <libcpu/event.hh>
#include <libcpu/riscv_user_core.hh>
#include <libcpu/rv32i_cpu_staging.hh>
#include <libcpu/riscv.hh>
#include <libvio/width.hh>

namespace libcpu {

uint8_t rv32i_cpu_staging::n_gpr(void) const {
    return 32;
}

const char *rv32i_cpu_staging::gpr_name(uint8_t addr) const {
    return riscv::gpr_name(addr);
}

uint8_t rv32i_cpu_staging::gpr_addr(const char *name) const {
    return riscv::gpr_addr(name);
}

void rv32i_cpu_staging::reset(uint32_t init_pc) {
    user_core.pc = init_pc;
    user_core.data_bus = data_bus;
    user_core.mmio_bus = mmio_bus;
    user_core.event_buffer = event_buffer;
    for (auto &i: user_core.gpr) {
        i = 0;
    }
    is_stopped = false;
}

uint32_t rv32i_cpu_staging::get_pc(void) const {
    return user_core.pc;
}

const uint32_t *rv32i_cpu_staging::get_gpr(void) const {
    return user_core.gpr;
}

uint32_t rv32i_cpu_staging::get_gpr(uint8_t addr) const {
    return user_core.gpr[addr];
}

void rv32i_cpu_staging::next_cycle(void) {
    next_instruction();
}

void rv32i_cpu_staging::next_instruction(void) {
    uint32_t pc = user_core.pc;
    exec_result_t exec_result;

    // Do unprivileged operations
    std::optional<uint32_t> instr_opt = instr_bus->read(pc, libvio::width_t::word);
    if (instr_opt.has_value()) {
        decode_t decode = riscv_user_core<uint32_t>::decode_instr(instr_opt.value());
        exec_result = user_core.decode_exec(decode);
    } else {
        exec_result = {
            .type = exec_result_type_t::trap,
            .trap = {
                .mcause = riscv::mcause<uint32_t>::except_instr_fault,
                .mtval = pc,
            }
        };
    }

    // Do privileged operations
    if (exec_result.type==exec_result_type_t::privileged || exec_result.type==exec_result_type_t::trap) {
        is_stopped = true;
    }

//    assert(exec_result.type == exec_result_type_t::retire);

    if (this->mmio_bus != nullptr) {
        this->mmio_bus->next_cycle();
    }
}

bool rv32i_cpu_staging::stopped(void) const {
    return is_stopped;
}

std::optional<uint32_t> rv32i_cpu_staging::get_trap(void) const {
    return std::nullopt;
}

std::optional<uint32_t> rv32i_cpu_staging::pmem_peek(uint32_t addr, libvio::width_t width) const {
    return data_bus->peek(addr, width);
}

} // namespace libcpu
