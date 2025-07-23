#ifndef LIBCPU_RV32I_CPU_SYSTEM_HH
#define LIBCPU_RV32I_CPU_SYSTEM_HH

#include <cstdint>
#include <libcpu/riscv_cpu.hh>
#include <vector>

namespace libcpu {

class rv32i_cpu_system: public riscv_cpu<uint32_t> {
    public:
        static std::vector<csr_def_t> csr_list;
        virtual const std::vector<csr_def_t> &get_csr_list(void) const override;
        decode_t decode_instruction(uint32_t instruction) const override;
    protected:
        virtual void handle_trap(void) override;
        static void mret(riscv_cpu<uint32_t>* cpu, const decode_t& decode);
};

}

#endif
