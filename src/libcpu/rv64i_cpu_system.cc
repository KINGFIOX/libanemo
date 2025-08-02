#include <cstdint>
#include <cassert>
#include <libcpu/event.hh>
#include <libcpu/rv64i_cpu_system.hh>
#include <libcpu/riscv_user_core.hh>
#include <libcpu/riscv_privilege_module.hh>
#include <libcpu/riscv.hh>
#include <libvio/width.hh>

namespace libcpu {

uint8_t rv64i_cpu_system::n_gpr(void) const {
    return 32;
}

const char *rv64i_cpu_system::gpr_name(uint8_t addr) const {
    return riscv::gpr_name(addr);
}

uint8_t rv64i_cpu_system::gpr_addr(const char *name) const {
    return riscv::gpr_addr(name);
}

void rv64i_cpu_system::reset(word_t init_pc) {
    privilege_module.instr_bus = instr_bus;
    privilege_module.data_bus = data_bus;
    privilege_module.mmio_bus = mmio_bus;
    user_core.reset();
    privilege_module.reset();
    exec_result.pc = init_pc;
    last_trap = std::nullopt;
    is_stopped = false;
}

uint64_t rv64i_cpu_system::get_pc(void) const {
    return exec_result.pc;
}

const uint64_t *rv64i_cpu_system::get_gpr(void) const {
    return user_core.gpr;
}

uint64_t rv64i_cpu_system::get_gpr(uint8_t addr) const {
    return user_core.gpr[addr];
}

void rv64i_cpu_system::next_cycle(void) {
    next_instruction();
}

void rv64i_cpu_system::next_instruction(void) {
    privilege_module.paddr_fetch_instruction(exec_result);

    if (exec_result.type == exec_result_type_t::fetch) {
        if (event_buffer!=nullptr) {
            event_buffer->push_back({.type=event_type_t::issue, .pc=exec_result.pc, .val1=exec_result.instr, .val2=0});
        }
        user_core.decode(exec_result);
    }

    if (exec_result.type == exec_result_type_t::decode) {
        user_core.execute(exec_result);
    }

    // Do privileged operations
    if (exec_result.type == exec_result_type_t::load) {
        if (event_buffer!=nullptr) {
            auto [addr, width, sign_extend, rd] = exec_result.load;
            privilege_module.paddr_load(exec_result);
            if (exec_result.type == exec_result_type_t::retire) {
                event_buffer->push_back({.type=event_type_t::load, .pc=exec_result.pc, .val1=addr, .val2=libvio::zero_truncate(exec_result.retire.value, width)});
            }
        } else {
            privilege_module.paddr_load(exec_result);
        }
    } else if (exec_result.type == exec_result_type_t::store) {
        if (event_buffer!=nullptr) {
            auto [addr, width, data] = exec_result.store;
            privilege_module.paddr_store(exec_result);
            if (exec_result.type == exec_result_type_t::retire) {
                event_buffer->push_back({.type=event_type_t::store, .pc=exec_result.pc, .val1=addr, .val2=libvio::zero_truncate(data, width)});
            }
        } else {
            privilege_module.paddr_store(exec_result);
        }
    } else if (exec_result.type == exec_result_type_t::csr_op) {
        privilege_module.csr_op(exec_result);
    } else if (exec_result.type == exec_result_type_t::sys_op) {
        privilege_module.sys_op(exec_result);
        if (event_buffer!=nullptr && exec_result.type==exec_result_type_t::retire) {
            if (exec_result.sys_op.mret) {
                event_buffer->push_back({.type=event_type_t::trap_ret, .pc=exec_result.pc, .val1=privilege_module.mepc, .val2=0});
            } else if (exec_result.sys_op.sret) {
                event_buffer->push_back({.type=event_type_t::trap_ret, .pc=exec_result.pc, .val1=privilege_module.sepc, .val2=0});
            }
        }
    }

    if (exec_result.type == exec_result_type_t::trap) {
        if (exec_result.trap.cause == riscv::mcause<uint32_t>::except_breakpoint) {
            is_stopped = true;
            return;
        }
        if (event_buffer != nullptr) {
            event_buffer->push_back({.type=event_type_t::trap, .pc=exec_result.pc, .val1=exec_result.trap.cause, .val2=exec_result.trap.tval});
        }
        last_trap = exec_result.trap.cause;
        privilege_module.handle_exception(exec_result);
    } else {
        last_trap = std::nullopt;
        privilege_module.handle_interrupt(exec_result);
    }

    assert(exec_result.type == exec_result_type_t::retire);
    if (exec_result.retire.rd!=0) {
        if (event_buffer!=nullptr) {
            event_buffer->push_back({.type=event_type_t::reg_write, .pc=exec_result.pc, .val1=exec_result.retire.rd, .val2=exec_result.retire.value});
        }
        user_core.gpr[exec_result.retire.rd] = exec_result.retire.value;
    }

    exec_result.pc = exec_result.next_pc;
    if (this->mmio_bus != nullptr) {
        this->mmio_bus->next_cycle();
    }
}

bool rv64i_cpu_system::stopped(void) const {
    return is_stopped;
}

std::optional<uint64_t> rv64i_cpu_system::get_trap(void) const {
    return last_trap;
}

std::optional<uint64_t> rv64i_cpu_system::pmem_peek(word_t addr, libvio::width_t width) const {
    return data_bus->read(addr, width);
}

} // namespace libcpu
