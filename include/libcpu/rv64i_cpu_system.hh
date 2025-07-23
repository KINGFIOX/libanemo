#ifndef LIBCPU_RV64I_CPU_SYSTEM_HH
#define LIBCPU_RV64I_CPU_SYSTEM_HH

#include <libcpu/abstract_cpu.hh>
#include <libcpu/event.hh>
#include <libcpu/memory.hh>
#include <libcpu/riscv.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/frontend.hh>
#include <libvio/bus.hh>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace libcpu {

class rv64i_cpu_system: public abstract_cpu<uint64_t> {

    public:

        static constexpr size_t n_csr = 9;
        static constexpr size_t n_interrupt = 16;

        using decode_t = struct decode_t {
            void (*op)(rv64i_cpu_system* cpu, const decode_t& decode);
            uint32_t instr;
            word_t imm;
            uint_fast8_t rs1;
            uint_fast8_t rs2;
            uint_fast8_t rd;
        };

        using csr_info_t = struct csr_info_t {
            word_t init_value; ///< Initial value on boot
            word_t wpri_mask; ///< The and mask when writing with csr operation instructions.
            const char *name; ///< Name of the CSR
            rv64i::csr_addr_t addr; ///< Address of the CSR
        };

        static const csr_info_t csr_info[n_csr];

    private:
        // architectural state
        std::array<word_t, 32> gpr;
        std::array<word_t, n_csr> csr;
        word_t pc;
        word_t next_pc;
        rv64i::priv_level_t priv_level;

        word_t instruction;

        std::vector<decode_t> decode_cache;
        word_t decode_cache_addr_mask;

        bool exception_flag = false;
        bool ebreak_flag = false;
        word_t exception_cause;
        word_t exception_mtval;
        std::optional<word_t> next_trap = {};  

        // helper functions for memory operations
        void load(const decode_t &decode, libvio::width_t width, bool sign_extend);
        void store(const decode_t &decode, libvio::width_t width);

        void raise_exception(rv64i::mcause_t mcause, word_t mtval);
        word_t handle_trap(void);

        // csr operations with no permmission check
        // used for emulating a hardware writing a csr
        bool csr_check_read_access(rv64i::csr_addr_t addr) const;
        bool csr_check_write_access(rv64i::csr_addr_t addr) const;
        void csr_write(rv64i::csr_addr_t addr, word_t value);
        void csr_write_bits(rv64i::csr_addr_t addr, word_t value, word_t bit_mask);
        void csr_set_bits(rv64i::csr_addr_t addr, word_t bits);
        void csr_clear_bits(rv64i::csr_addr_t addr, word_t bits);

    public:

        void reset(word_t init_pc) override;
        
        uint8_t n_gpr(void) const override;
        const char* gpr_name(uint8_t addr) const override;
        uint8_t gpr_addr(const char* name) const override;

        word_t get_gpr(uint8_t gpr_addr) const override;
        const word_t *get_gpr(void) const override;
        word_t get_pc(void) const override;
        rv64i::priv_level_t get_priv_level(void) const;

        void next_cycle(void) override;
        void next_instruction(void) override;
        bool stopped(void) const override;

        word_t csr_read(rv64i::csr_addr_t addr) const;
        word_t csr_read_bits(rv64i::csr_addr_t addr, word_t bit_mask) const;

        static decode_t decode_instruction(word_t instruction);

        std::optional<word_t>pmem_peek(word_t addr, libvio::width_t width) const override;

        std::optional<word_t> get_trap(void) const override;

    protected:
        
        void raise_interrupt(rv64i::mcause_t mcause);

    private:

        // ========== Instruction Implementations ==========
        // Arithmetic & Logical
        static void add(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sub(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sll(rv64i_cpu_system* cpu, const decode_t& decode);
        static void slt(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sltu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void xor_(rv64i_cpu_system* cpu, const decode_t& decode);
        static void srl(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sra(rv64i_cpu_system* cpu, const decode_t& decode);
        static void or_(rv64i_cpu_system* cpu, const decode_t& decode);
        static void and_(rv64i_cpu_system* cpu, const decode_t& decode);

        // Immediate Operations
        static void addi(rv64i_cpu_system* cpu, const decode_t& decode);
        static void slti(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sltiu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void xori(rv64i_cpu_system* cpu, const decode_t& decode);
        static void ori(rv64i_cpu_system* cpu, const decode_t& decode);
        static void andi(rv64i_cpu_system* cpu, const decode_t& decode);
        static void slli(rv64i_cpu_system* cpu, const decode_t& decode);
        static void srli(rv64i_cpu_system* cpu, const decode_t& decode);
        static void srai(rv64i_cpu_system* cpu, const decode_t& decode);

        // Memory Operations
        static void lb(rv64i_cpu_system* cpu, const decode_t& decode);
        static void lh(rv64i_cpu_system* cpu, const decode_t& decode);
        static void lw(rv64i_cpu_system* cpu, const decode_t& decode);
        static void lbu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void lhu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sb(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sh(rv64i_cpu_system* cpu, const decode_t& decode);
        static void sw(rv64i_cpu_system* cpu, const decode_t& decode);

        // Control Flow
        static void jal(rv64i_cpu_system* cpu, const decode_t& decode);
        static void jalr(rv64i_cpu_system* cpu, const decode_t& decode);
        static void beq(rv64i_cpu_system* cpu, const decode_t& decode);
        static void bne(rv64i_cpu_system* cpu, const decode_t& decode);
        static void blt(rv64i_cpu_system* cpu, const decode_t& decode);
        static void bge(rv64i_cpu_system* cpu, const decode_t& decode);
        static void bltu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void bgeu(rv64i_cpu_system* cpu, const decode_t& decode);

        // Upper Immediate
        static void lui(rv64i_cpu_system* cpu, const decode_t& decode);
        static void auipc(rv64i_cpu_system* cpu, const decode_t& decode);

        // Multiply/Divide
        static void mul(rv64i_cpu_system* cpu, const decode_t& decode);
        static void mulh(rv64i_cpu_system* cpu, const decode_t& decode);
        static void mulhsu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void mulhu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void div(rv64i_cpu_system* cpu, const decode_t& decode);
        static void divu(rv64i_cpu_system* cpu, const decode_t& decode);
        static void rem(rv64i_cpu_system* cpu, const decode_t& decode);
        static void remu(rv64i_cpu_system* cpu, const decode_t& decode);

        // System
        static void ecall(rv64i_cpu_system* cpu, const decode_t& decode);
        static void ebreak(rv64i_cpu_system* cpu, const decode_t& decode);
        static void mret(rv64i_cpu_system* cpu, const decode_t& decode);
        // csr functions with permission check
        // functions emulating the csr accesing instructions
        static void csrrw(rv64i_cpu_system* cpu, const decode_t& decode);
        static void csrrs(rv64i_cpu_system* cpu, const decode_t& decode);
        static void csrrc(rv64i_cpu_system* cpu, const decode_t& decode);
        static void csrrwi(rv64i_cpu_system* cpu, const decode_t& decode);
        static void csrrsi(rv64i_cpu_system* cpu, const decode_t& decode);
        static void csrrci(rv64i_cpu_system* cpu, const decode_t& decode);
};

}

#endif
