#include <cstddef>
#include <cstdint>
#include <libcpu/riscv.hh>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_system.hh>
#include <libvio/ringbuffer.hh>

using namespace libcpu;
using namespace libcpu::rv32i;

void rv32i_cpu_system::reset(word_t init_pc) {
    for (size_t i=0; i<32; ++i) {
        gpr[i] = 0;
    }
    for (size_t i=0; i<n_csr; ++i) {
        csr[i] = csr_info[i].init_value;
    }
    pc = init_pc;
    priv_level = priv_level_t::m;
    decode_cache.resize(4096);
    decode_cache_addr_mask = 0xfff;
};

uint8_t rv32i_cpu_system::n_gpr(void) const {
    return 32;
}

const char* rv32i_cpu_system::gpr_name(uint8_t addr) const {
    return riscv::gpr_name(addr);
}

uint8_t rv32i_cpu_system::gpr_addr(const char* name) const {
    return riscv::gpr_addr(name);
}

uint32_t rv32i_cpu_system::get_gpr(uint8_t gpr_addr) const {
    return gpr[gpr_addr];
}

uint32_t rv32i_cpu_system::get_pc(void) const {
    return pc;
}

priv_level_t rv32i_cpu_system::get_priv_level() const {
    return priv_level;
}

const uint32_t* rv32i_cpu_system::get_gpr(void) const {
    return gpr.data();
};

std::optional<rv32i_cpu_system::word_t> rv32i_cpu_system::pmem_peek(word_t addr, libvio::width_t width) const {
    return data_bus->peek(addr, width);
}

bool rv32i_cpu_system::stopped(void) const {
    return ebreak_flag;
}

std::optional<rv32i_cpu_system::word_t> rv32i_cpu_system::get_trap(void) const {
    return next_trap;
}
