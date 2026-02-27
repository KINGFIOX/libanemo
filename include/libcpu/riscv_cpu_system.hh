#ifndef LIBCPU_RISCV_CPU_SYSYTEM_HH
#define LIBCPU_RISCV_CPU_SYSYTEM_HH

#include <cstdint>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/riscv/privilege_module.hh>
#include <libcpu/riscv/riscv.hh>
#include <libcpu/riscv/user_core.hh>

namespace libcpu {

template <typename WORD_T>
class riscv_cpu_system : public abstract_cpu<WORD_T> {
public:
  using dispatch_t = riscv::dispatch_t;
  using decode_t = riscv::decode_t;
  using exec_result_type_t = riscv::exec_result_type_t;
  using exec_result_t = riscv::exec_result_t<WORD_T>;

  virtual uint8_t n_gpr(void) const override;
  virtual const char *gpr_name(uint8_t addr) const override;
  virtual uint8_t gpr_addr(const char *name) const override;
  virtual void reset(WORD_T init_pc) override;
  virtual WORD_T get_pc(void) const override;
  virtual const WORD_T *get_gpr(void) const override;
  virtual WORD_T get_gpr(uint8_t addr) const override;
  virtual void next_cycle(void) override;
  virtual void next_instruction(void) override;
  virtual bool stopped(void) const override;
  virtual std::optional<WORD_T> get_trap(void) const override;

private:
  exec_result_t exec_result;
  riscv::user_core<WORD_T> user_core;
  riscv::privilege_module<WORD_T> privilege_module;
  std::optional<WORD_T> last_trap;
  bool is_stopped;
};

template <typename WORD_T> uint8_t riscv_cpu_system<WORD_T>::n_gpr(void) const {
  return 32;
}

template <typename WORD_T>
const char *riscv_cpu_system<WORD_T>::gpr_name(uint8_t addr) const {
  return riscv::gpr_name(addr);
}

template <typename WORD_T>
uint8_t riscv_cpu_system<WORD_T>::gpr_addr(const char *name) const {
  return riscv::gpr_addr(name);
}

template <typename WORD_T>
void riscv_cpu_system<WORD_T>::reset(WORD_T init_pc) {
  privilege_module.mem_bus = this->mem_bus;
  privilege_module.mmio_bus = this->mmio_bus;
  user_core.reset();
  privilege_module.reset();
  exec_result.pc = init_pc;
  last_trap = std::nullopt;
  is_stopped = false;
}

template <typename WORD_T> WORD_T riscv_cpu_system<WORD_T>::get_pc(void) const {
  return exec_result.pc;
}

template <typename WORD_T>
const WORD_T *riscv_cpu_system<WORD_T>::get_gpr(void) const {
  return user_core.gpr;
}

template <typename WORD_T>
WORD_T riscv_cpu_system<WORD_T>::get_gpr(uint8_t addr) const {
  return user_core.gpr[addr];
}

template <typename WORD_T> void riscv_cpu_system<WORD_T>::next_cycle(void) {
  next_instruction();
}

template <typename WORD_T>
void riscv_cpu_system<WORD_T>::next_instruction(void) {
  privilege_module.paddr_fetch_instruction(exec_result);

  if (exec_result.type == exec_result_type_t::fetch) {
    if (this->event_buffer != nullptr) {
      this->event_buffer->push_back({.type = event_type_t::issue,
                                     .pc = exec_result.pc,
                                     .val1 = exec_result.instr,
                                     .val2 = 0});
    }
    riscv::user_core<WORD_T>::decode(exec_result);
  }

  if (exec_result.type == exec_result_type_t::decode) {
    user_core.execute(exec_result);
  }

  // Do privileged operations
  if (exec_result.type == exec_result_type_t::load) {
    if (this->event_buffer != nullptr) {
      auto [addr, width, sign_extend, rd] = exec_result.load;
      privilege_module.paddr_load(exec_result);
      if (exec_result.type == exec_result_type_t::retire) {
        this->event_buffer->push_back(
            {.type = event_type_t::load,
             .pc = exec_result.pc,
             .val1 = addr,
             .val2 = libvio::zero_truncate(exec_result.retire.value, width)});
      }
    } else {
      privilege_module.paddr_load(exec_result);
    }
  } else if (exec_result.type == exec_result_type_t::store) {
    if (this->event_buffer != nullptr) {
      auto [addr, width, data] = exec_result.store;
      privilege_module.paddr_store(exec_result);
      if (exec_result.type == exec_result_type_t::retire) {
        this->event_buffer->push_back(
            {.type = event_type_t::store,
             .pc = exec_result.pc,
             .val1 = addr,
             .val2 = libvio::zero_truncate(data, width)});
      }
    } else {
      privilege_module.paddr_store(exec_result);
    }
  } else if (exec_result.type == exec_result_type_t::csr_op) {
    privilege_module.csr_op(exec_result);
  } else if (exec_result.type == exec_result_type_t::sys_op) {
    privilege_module.sys_op(exec_result);
    if (this->event_buffer != nullptr &&
        exec_result.type == exec_result_type_t::retire) {
      if (exec_result.sys_op.mret) {
        this->event_buffer->push_back({.type = event_type_t::trap_ret,
                                       .pc = exec_result.pc,
                                       .val1 = privilege_module.mepc,
                                       .val2 = 0});
      } else if (exec_result.sys_op.sret) {
        this->event_buffer->push_back({.type = event_type_t::trap_ret,
                                       .pc = exec_result.pc,
                                       .val1 = privilege_module.sepc,
                                       .val2 = 0});
      }
    }
  }

  if (exec_result.type == exec_result_type_t::trap) {
    if (exec_result.trap.cause == riscv::mcause<WORD_T>::except_breakpoint) {
      is_stopped = true;
      return;
    }
    if (this->event_buffer != nullptr) {
      this->event_buffer->push_back({.type = event_type_t::trap,
                                     .pc = exec_result.pc,
                                     .val1 = exec_result.trap.cause,
                                     .val2 = exec_result.trap.tval});
    }
    last_trap = exec_result.trap.cause;
    privilege_module.handle_exception(exec_result);
  } else {
    last_trap = std::nullopt;
    privilege_module.handle_interrupt(exec_result);
  }

  assert(exec_result.type == exec_result_type_t::retire);
  if (exec_result.retire.rd != 0) {
    if (this->event_buffer != nullptr) {
      this->event_buffer->push_back({.type = event_type_t::reg_write,
                                     .pc = exec_result.pc,
                                     .val1 = exec_result.retire.rd,
                                     .val2 = exec_result.retire.value});
    }
    user_core.gpr[exec_result.retire.rd] = exec_result.retire.value;
  }

  exec_result.pc = exec_result.next_pc;
}

template <typename WORD_T> bool riscv_cpu_system<WORD_T>::stopped(void) const {
  return is_stopped;
}

template <typename WORD_T>
std::optional<WORD_T> riscv_cpu_system<WORD_T>::get_trap(void) const {
  return last_trap;
}

} // namespace libcpu

#endif
