#ifndef LIBCPU_RV32I_CPU_SYSTEM_HH
#define LIBCPU_RV32I_CPU_SYSTEM_HH

#include <libcpu/abstract_cpu.hh>
#include <libcpu/event.hh>
#include <libcpu/memory.hh>
#include <libcpu/rv32i.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/frontend.hh>
#include <libvio/bus.hh>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace libcpu {

class rv32i_cpu_system: public abstract_cpu_system<uint32_t, 32> {

    public:

        static constexpr size_t n_csr = 10;
        static constexpr size_t n_gpr = 32;
        static constexpr size_t n_interrupt = 16;
        static constexpr word_t mem_base = 0x80000000;

        using decode_t = struct decode_t {
            void (*op)(rv32i_cpu_system* cpu, const decode_t& decode);
            uint32_t instr;
            uint32_t imm;
            uint_fast8_t rs1;
            uint_fast8_t rs2;
            uint_fast8_t rd;
        };

        using csr_info_t = struct csr_info_t {
            uint32_t init_value; ///< Initial value on boot
            uint32_t wpri_mask; ///< The and mask when writing with csr operation instructions.
            const char *name; ///< Name of the CSR
            rv32i::csr_addr_t addr; ///< Address of the CSR
        };

        static const csr_info_t csr_info[n_csr];

    private:
        // architectural state
        std::array<uint32_t, n_gpr> gpr;
        std::array<uint32_t, n_csr> csr;
        uint32_t pc = mem_base;
        uint32_t next_pc;
        rv32i::priv_level_t priv_level;

        uint32_t instruction;

        std::vector<decode_t> decode_cache;

        bool exception_flag = false;
        bool ebreak_flag = false;
        uint32_t exception_cause;
        uint32_t exception_mtval;

        // helper functions for memory operations
        void load(const decode_t &decode, libvio::width_t width, bool sign_extend);
        void store(const decode_t &decode, libvio::width_t width);

        void raise_exception(rv32i::mcause_t mcause, uint32_t mtval);
        uint32_t handle_trap(void);

        // csr operations with no permmission check
        // used for emulating a hardware writing a csr
        bool csr_check_read_access(rv32i::csr_addr_t addr) const;
        bool csr_check_write_access(rv32i::csr_addr_t addr) const;
        void csr_write(rv32i::csr_addr_t addr, uint32_t value);
        void csr_write_bits(rv32i::csr_addr_t addr, uint32_t value, uint32_t bit_mask);
        void csr_set_bits(rv32i::csr_addr_t addr, uint32_t bits);
        void csr_clear_bits(rv32i::csr_addr_t addr, uint32_t bits);

    public:

        contiguous_memory<word_t> memory;

        uint32_t get_gpr(uint8_t gpr_addr) const override;
        const uint32_t *get_gpr(void) const override;
        uint32_t get_pc(void) const override;
        rv32i::priv_level_t get_priv_level(void) const;

        rv32i_cpu_system(size_t memory_size);

        void next_cycle(void) override;
        void next_instruction(void) override;
        void next_instruction_pre_decode(void);
        bool stopped(void) const override;

        uint32_t csr_read(rv32i::csr_addr_t addr) const;
        uint32_t csr_read_bits(rv32i::csr_addr_t addr, uint32_t bit_mask) const;

        void pre_decode(void);
        static decode_t decode_instruction(uint32_t instruction);

        std::optional<word_t> pmem_read(word_t addr, libvio::width_t width) const override;

    protected:
        
        void raise_interrupt(rv32i::mcause_t mcause);

    private:

        // ========== Instruction Implementations ==========
        // Arithmetic & Logical
        static void add(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sub(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sll(rv32i_cpu_system* cpu, const decode_t& decode);
        static void slt(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sltu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void xor_(rv32i_cpu_system* cpu, const decode_t& decode);
        static void srl(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sra(rv32i_cpu_system* cpu, const decode_t& decode);
        static void or_(rv32i_cpu_system* cpu, const decode_t& decode);
        static void and_(rv32i_cpu_system* cpu, const decode_t& decode);

        // Immediate Operations
        static void addi(rv32i_cpu_system* cpu, const decode_t& decode);
        static void slti(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sltiu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void xori(rv32i_cpu_system* cpu, const decode_t& decode);
        static void ori(rv32i_cpu_system* cpu, const decode_t& decode);
        static void andi(rv32i_cpu_system* cpu, const decode_t& decode);
        static void slli(rv32i_cpu_system* cpu, const decode_t& decode);
        static void srli(rv32i_cpu_system* cpu, const decode_t& decode);
        static void srai(rv32i_cpu_system* cpu, const decode_t& decode);

        // Memory Operations
        static void lb(rv32i_cpu_system* cpu, const decode_t& decode);
        static void lh(rv32i_cpu_system* cpu, const decode_t& decode);
        static void lw(rv32i_cpu_system* cpu, const decode_t& decode);
        static void lbu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void lhu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sb(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sh(rv32i_cpu_system* cpu, const decode_t& decode);
        static void sw(rv32i_cpu_system* cpu, const decode_t& decode);

        // Control Flow
        static void jal(rv32i_cpu_system* cpu, const decode_t& decode);
        static void jalr(rv32i_cpu_system* cpu, const decode_t& decode);
        static void beq(rv32i_cpu_system* cpu, const decode_t& decode);
        static void bne(rv32i_cpu_system* cpu, const decode_t& decode);
        static void blt(rv32i_cpu_system* cpu, const decode_t& decode);
        static void bge(rv32i_cpu_system* cpu, const decode_t& decode);
        static void bltu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void bgeu(rv32i_cpu_system* cpu, const decode_t& decode);

        // Upper Immediate
        static void lui(rv32i_cpu_system* cpu, const decode_t& decode);
        static void auipc(rv32i_cpu_system* cpu, const decode_t& decode);

        // Multiply/Divide
        static void mul(rv32i_cpu_system* cpu, const decode_t& decode);
        static void mulh(rv32i_cpu_system* cpu, const decode_t& decode);
        static void mulhsu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void mulhu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void div(rv32i_cpu_system* cpu, const decode_t& decode);
        static void divu(rv32i_cpu_system* cpu, const decode_t& decode);
        static void rem(rv32i_cpu_system* cpu, const decode_t& decode);
        static void remu(rv32i_cpu_system* cpu, const decode_t& decode);

        // System
        static void ecall(rv32i_cpu_system* cpu, const decode_t& decode);
        static void ebreak(rv32i_cpu_system* cpu, const decode_t& decode);
        static void mret(rv32i_cpu_system* cpu, const decode_t& decode);
        // csr functions with permission check
        // functions emulating the csr accesing instructions
        static void csrrw(rv32i_cpu_system* cpu, const decode_t& decode);
        static void csrrs(rv32i_cpu_system* cpu, const decode_t& decode);
        static void csrrc(rv32i_cpu_system* cpu, const decode_t& decode);
        static void csrrwi(rv32i_cpu_system* cpu, const decode_t& decode);
        static void csrrsi(rv32i_cpu_system* cpu, const decode_t& decode);
        static void csrrci(rv32i_cpu_system* cpu, const decode_t& decode);
};

}

#endif
