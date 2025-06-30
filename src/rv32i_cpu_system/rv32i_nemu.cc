#include <cstddef>
#include <cstdint>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu.hh>
#include <libcpu/rv32i_cpu_nemu.hh>
#include <libvio/ringbuffer.hh>


using namespace libcpu;
using namespace libcpu::rv32i;

rv32i_cpu_nemu::rv32i_cpu_nemu(size_t event_buffer_size, size_t memory_size, libvio::bus* mmio_bus): rv32i_cpu(event_buffer_size, memory_size, mmio_bus) {
    for (size_t i=0; i<n_gpr; ++i) {
        gpr[i] = 0;
    }
    for (size_t i=0; i<n_csr; ++i) {
        csr[i] = csr_info[i].init_value;
    }
    priv_level = priv_level_t::m;
};

uint32_t rv32i_cpu_nemu::gpr_read(uint8_t gpr_addr) const {
    return gpr[gpr_addr];
}

uint32_t rv32i_cpu_nemu::get_pc(void) const {
    return pc;
}

priv_level_t rv32i_cpu_nemu::get_priv_level() const {
    return priv_level;
}

const uint32_t* rv32i_cpu_nemu::get_regfile(void) const {
    return gpr.data();
};
