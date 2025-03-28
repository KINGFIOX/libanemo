#ifndef LIBCPU_RV32I_CPU_HH
#define LIBCPU_RV32I_CPU_HH

#include <cstddef>
#include <libcpu/rv32i.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/frontend.hh>
#include <libvio/bus.hh>
#include <cstdint>
#include <vector>

namespace libcpu {

class rv32i_cpu {

    public:

        static constexpr size_t n_gpr = 32;
        static constexpr uint32_t ram_base = 0x80000000;

        using width_t = libvio::width_t;
        using priv_level_t = rv32i::priv_level_t;
        using csr_addr_t = rv32i::csr_addr_t;
        using event_type_t = rv32i::event_type_t;
        using event_t = rv32i::event_t;
        using mcause_t = rv32i::mcause_t;

    protected:

        // event ring buffer for tracing and difftest
        libvio::ringbuffer<event_t> event_buffer;

    public:

        rv32i_cpu(size_t event_buffer_size, size_t memory_size, libvio::bus* mmio_bus): event_buffer(event_buffer_size), memory(memory_size), mmio_bus(mmio_bus) {};

        virtual uint32_t gpr_read(uint8_t gpr_addr) const = 0;
        virtual uint32_t get_pc(void) const = 0;
        virtual priv_level_t get_priv_level(void) const = 0;

        std::vector<uint8_t> memory;

        libvio::bus* mmio_bus = nullptr;

        bool trace_on = false;

        virtual void next_cycle(void) = 0;
        virtual void next_instruction(void) = 0;
        virtual bool get_ebreak(void) const = 0;

        virtual void raise_interrupt(mcause_t mcause) = 0;

        virtual uint32_t csr_read(csr_addr_t addr) const = 0;
        virtual uint32_t csr_read_bits(csr_addr_t addr, uint32_t bit_mask) const = 0;
        
        virtual const libvio::ringbuffer<event_t>& get_events(void) const { return event_buffer; };

        virtual uint32_t pmem_read(uint32_t addr, width_t width) const {
            if (addr >= ram_base && addr < ram_base + memory.size()) {
                uint32_t offset = addr - ram_base;
                switch (width) {
                    case width_t::byte:
                        return uint32_t(memory[offset]);
                    case width_t::half:
                        return uint32_t(memory[offset]) | (uint32_t(memory[offset+1]) << 8);
                    case width_t::word:
                        return uint32_t(memory[offset]) | (uint32_t(memory[offset+1]) << 8) |
                               (uint32_t(memory[offset+2]) << 16) | (uint32_t(memory[offset+3]) << 24);
                    default:
                        return 0;
                }
            } else {
                return 0;
            }
        }
};

}

#endif
