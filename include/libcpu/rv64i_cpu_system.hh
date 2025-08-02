#ifndef LIBCPU_RV64I_CPU_SYSYTEM_HH
#define LIBCPU_RV64I_CPU_SYSYTEM_HH

#include <cstdint>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/memory_staging.hh>
#include <libcpu/riscv_user_core.hh>
#include <libcpu/riscv_privilege_module.hh>

namespace libcpu {

class rv64i_cpu_system : public abstract_cpu<uint64_t> {
    public:
        using dispatch_t  = riscv_user_core<word_t>::dispatch_t;
        using exec_result_type_t = riscv_user_core<word_t>::exec_result_type_t;
        using exec_result_t = riscv_user_core<word_t>::exec_result_t;

        memory_staging *instr_bus = nullptr;
        memory_staging *data_bus = nullptr;

        virtual uint8_t n_gpr(void) const override;
        virtual const char *gpr_name(uint8_t addr) const override;
        virtual uint8_t gpr_addr(const char *name) const override;
        virtual void reset(word_t init_pc) override;
        virtual word_t get_pc(void) const override;
        virtual const word_t *get_gpr(void) const override;
        virtual word_t get_gpr(uint8_t addr) const override;
        virtual void next_cycle(void) override;
        virtual void next_instruction(void) override;
        virtual bool stopped(void) const override;
        virtual std::optional<word_t> get_trap(void) const override;
        std::optional<word_t>pmem_peek(word_t addr, libvio::width_t width) const override;

    private:
        exec_result_t exec_result;
        riscv_user_core<word_t> user_core;
        riscv_privilege_module<word_t> privilege_module;
        std::optional<word_t> last_trap;
        bool is_stopped;
};

}

#endif // LIBCPU_RV32I_CPU_STAGING_HH
