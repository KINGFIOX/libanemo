#include <cstdint>
#include <libcpu/rv32i_cpu_nemu.hh>

using namespace libcpu;
using namespace libcpu::rv32i;

rv32i_cpu_nemu::rv32i_cpu_nemu(size_t event_buffer_size, size_t memory_size, libvio::bus* mmio_bus): event_buffer(event_buffer_size), memory(memory_size), mmio_bus(mmio_bus) {};

uint32_t rv32i_cpu_nemu::gpr_read(uint8_t gpr_addr) const {
    return gpr[gpr_addr];
}

const libvio::ringbuffer<event_t>& rv32i_cpu_nemu::get_events(void) const {
    return event_buffer;
}
