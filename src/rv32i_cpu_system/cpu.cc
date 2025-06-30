#include <cstddef>
#include <cstdint>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_system.hh>
#include <libvio/ringbuffer.hh>

using namespace libcpu;
using namespace libcpu::rv32i;

rv32i_cpu_system::rv32i_cpu_system(size_t memory_size): memory(mem_base, memory_size) {
    for (size_t i=0; i<n_gpr; ++i) {
        gpr[i] = 0;
    }
    for (size_t i=0; i<n_csr; ++i) {
        csr[i] = csr_info[i].init_value;
    }
    priv_level = priv_level_t::m;
};

uint32_t rv32i_cpu_system::get_gpr(uint8_t gpr_addr) const {
    return gpr[gpr_addr];
}

uint32_t rv32i_cpu_system::get_current_pc(void) const {
    return old_pc;
}

uint32_t rv32i_cpu_system::get_next_pc(void) const {
    return pc;
}

priv_level_t rv32i_cpu_system::get_priv_level() const {
    return priv_level;
}

const uint32_t* rv32i_cpu_system::get_gpr(void) const {
    return gpr.data();
};

std::optional<rv32i_cpu_system::word_t> rv32i_cpu_system::pmem_read(word_t addr, libvio::width_t width) const {
    return memory.read(addr, width);
}
