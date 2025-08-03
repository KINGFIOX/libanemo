#ifndef LIBCPU_RISCV_CPU_SYSYTEM_HH
#define LIBCPU_RISCV_CPU_SYSYTEM_HH

#include <cstdint>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/riscv_user_core.hh>
#include <libcpu/riscv_privilege_module.hh>

namespace libcpu {

template <typename WORD_T>
class riscv_cpu_system: public abstract_cpu<WORD_T> {
    public:
        using dispatch_t  = riscv_user_core<WORD_T>::dispatch_t;
        using exec_result_type_t = riscv_user_core<WORD_T>::exec_result_type_t;
        using exec_result_t = riscv_user_core<WORD_T>::exec_result_t;

        virtual uint8_t n_gpr(void) const override;
        virtual const char *gpr_name(uint8_t addr) const override;
        virtual uint8_t gpr_addr(const char *name) const override;
        virtual void reset(WORD_T init_pc) override;
        virtual WORD_T get_pc(void) const override;
        virtual const WORD_T *get_gpr(void) const override;
        virtual WORD_T get_gpr(uint8_t addr) const override;
        virtual void next_cycle(void) override;
        virtual void next_instruction(void) override;
        virtual bool stopped(void) const override;
        virtual std::optional<WORD_T> get_trap(void) const override;

    private:
        exec_result_t exec_result;
        riscv_user_core<WORD_T> user_core;
        riscv_privilege_module<WORD_T> privilege_module;
        std::optional<WORD_T> last_trap;
        bool is_stopped;
};

}

#endif
