#ifndef LIBCPU_RV32I_CPU_SYSTEM_HH
#define LIBCPU_RV32I_CPU_SYSTEM_HH

#include <cstdint>
#include <libcpu/riscv_cpu.hh>
#include <vector>

namespace libcpu {

class rv64i_cpu_system: public riscv_cpu<uint64_t> {
    public:
        static std::vector<csr_def_t> csr_list;
        virtual const std::vector<csr_def_t> &get_csr_list(void) const override;
        decode_t decode_instruction(uint32_t instruction) const override; // Added compressed instructions and mret, sret
        void next_instruction(void) override; // Now it accepts compressed instructions
        std::optional<uint64_t> vaddr_to_paddr(uint64_t addr) const override; // Supports virtual memory
    protected:
        virtual void handle_trap(void) override;
        static void mret(riscv_cpu<uint64_t>* cpu, const decode_t& decode);
        static void sret(riscv_cpu<uint64_t>* cpu, const decode_t& decode);
};

}

#endif
