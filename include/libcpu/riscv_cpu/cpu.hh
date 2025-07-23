#ifndef LIBCPU_RISCV_CPU_CPU_HH
#define LIBCPU_RISCV_CPU_CPU_HH

#include <libcpu/event.hh>
#include <libcpu/riscv.hh>
#include <libcpu/riscv_cpu/riscv_cpu.hh>

namespace libcpu {

template <typename WORD_T>
void riscv_cpu<WORD_T>::reset(WORD_T init_pc) {
    for (size_t i=0; i<32; ++i) {
        gpr[i] = 0;
    }
    const std::vector<csr_def_t> &csr_info = get_csr_list();
    for (size_t i=0; i<csr_info.size(); ++i) {
        csr.push_back(csr_info[i].init_value);
    }
    pc = init_pc;
    is_stopped = false;
    priv_level = riscv::priv_level_t::m;
    except_mcause = {};
    except_mtval = {};
    next_trap = {};
    decode_cache.resize(4096);
    decode_cache_addr_mask = 0xfff;
};

template <typename WORD_T>
uint8_t riscv_cpu<WORD_T>::n_gpr(void) const {
    return 32;
}

template <typename WORD_T>
const char* riscv_cpu<WORD_T>::gpr_name(uint8_t addr) const {
    return riscv::gpr_name(addr);
}

template <typename WORD_T>
uint8_t riscv_cpu<WORD_T>::gpr_addr(const char* name) const {
    return riscv::gpr_addr(name);
}

template <typename WORD_T>
WORD_T riscv_cpu<WORD_T>::get_gpr(uint8_t gpr_addr) const {
    return gpr[gpr_addr];
}

template <typename WORD_T>
WORD_T riscv_cpu<WORD_T>::get_pc(void) const {
    return pc;
}

template <typename WORD_T>
const WORD_T* riscv_cpu<WORD_T>::get_gpr(void) const {
    return gpr.data();
};

template <typename WORD_T>
riscv::priv_level_t riscv_cpu<WORD_T>::get_priv_level(void) const {
    return priv_level;
};

template <typename WORD_T>
std::optional<WORD_T> riscv_cpu<WORD_T>::pmem_peek(WORD_T addr, libvio::width_t width) const {
    return this->data_bus->peek(addr, width);
}

template <typename WORD_T>
bool riscv_cpu<WORD_T>::stopped(void) const {
    return is_stopped;
}

template <typename WORD_T>
std::optional<WORD_T> riscv_cpu<WORD_T>::get_trap(void) const {
    return next_trap;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::raise_exception(WORD_T mcause, WORD_T mtval) {
    except_mcause = mcause;
    except_mtval = mtval;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::handle_trap(void) {
    if (except_mcause.has_value()) {
        if (this->event_buffer != nullptr) {
            this->event_buffer->push_back({event_type_t::trap, pc, except_mcause.value(), except_mtval.value()});
        }
        next_trap = except_mcause;
        is_stopped = true;
    }
}

}

#endif