#ifndef LIBCPU_RV32I_CPU_STAGING_HH
#define LIBCPU_RV32I_CPU_STAGING_HH

#include <cstdint>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/riscv_user_core.hh>
#include <libcpu/riscv_privilege_module.hh>

namespace libcpu {

class rv32i_cpu_staging : public abstract_cpu<uint32_t> {
    public:
        using dispatch_t  = riscv_user_core<uint32_t>::dispatch_t;
        using exec_result_type_t = riscv_user_core<uint32_t>::exec_result_type_t;
        using exec_result_t = riscv_user_core<uint32_t>::exec_result_t;

        virtual uint8_t n_gpr(void) const override;
        virtual const char *gpr_name(uint8_t addr) const override;
        virtual uint8_t gpr_addr(const char *name) const override;
        virtual void reset(uint32_t init_pc) override;
        virtual uint32_t get_pc(void) const override;
        virtual const uint32_t *get_gpr(void) const override;
        virtual uint32_t get_gpr(uint8_t addr) const override;
        virtual void next_cycle(void) override;
        virtual void next_instruction(void) override;
        virtual bool stopped(void) const override;
        virtual std::optional<uint32_t> get_trap(void) const override;
        std::optional<uint32_t>pmem_peek(uint32_t addr, libvio::width_t width) const override;

    private:
        exec_result_t exec_result;
        riscv_user_core<uint32_t> user_core;
        riscv_privilege_module<uint32_t> privilege_module;
        bool is_stopped;
};

}

#endif // LIBCPU_RV32I_CPU_STAGING_HH
