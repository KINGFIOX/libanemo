#ifndef LIBCPU_DIFFTEST_HH
#define LIBCPU_DIFFTEST_HH

#include <cstdint>
#include <initializer_list>
#include <libcpu/rv32i_cpu.hh>
#include <vector>

namespace libcpu {

class rv32i_cpu_difftest: public rv32i_cpu {
    private:

        std::vector<rv32i_cpu*> cpus;
        std::vector<size_t> buffer_index;
        bool difftest_error = false;

    public:

        rv32i_cpu_difftest(size_t event_buffer_size, size_t memory_size, std::initializer_list<rv32i_cpu*> init);
        void sync_memory(void);
        uint32_t gpr_read(uint8_t gpr_addr) const override;
        uint32_t get_pc(void) const override;
        priv_level_t get_priv_level(void) const override;
        void next_cycle(void) override;
        void next_instruction(void) override;
        bool get_ebreak(void) const override;
        void raise_interrupt(mcause_t mcause) override;
        uint32_t csr_read(csr_addr_t addr) const override;
        uint32_t csr_read_bits(csr_addr_t addr, uint32_t bit_mask) const override;
        uint32_t pmem_read(uint32_t addr, width_t width) const override;
        bool get_difftest_error(void) const;
};

}

#endif
