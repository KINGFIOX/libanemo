#ifndef LIBCPU_RISCV_CPU_RISCV_CPU_HH
#define LIBCPU_RISCV_CPU_RISCV_CPU_HH

#include <libcpu/abstract_cpu.hh>
#include <libcpu/riscv.hh>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace libcpu {

template <typename WORD_T>
class riscv_cpu: public abstract_cpu<WORD_T> {

    public:

        using decode_t = struct decode_t {
            void (*op)(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
            uint32_t instr;
            WORD_T imm;
            uint_fast8_t rs1;
            uint_fast8_t rs2;
            uint_fast8_t rd;
        };

        using csr_def_t = struct {
            const char *name;
            uint16_t addr;
            WORD_T init_value;
            WORD_T wpri_mask;
        };

        static constexpr std::vector<csr_def_t> supported_csrs = {};

    protected:
        // architectural state
        std::array<WORD_T, 32> gpr;
        WORD_T pc;
        WORD_T next_pc;
        std::vector<WORD_T> csr;

        riscv::priv_level_t priv_level;

        std::vector<decode_t> decode_cache;
        WORD_T decode_cache_addr_mask;

        bool is_stopped = false;
        // trap flags
        std::optional<WORD_T> except_mcause = {};
        std::optional<WORD_T> except_mtval = {};
        // for `get_trap`
        std::optional<WORD_T> next_trap = {};  

        // helper functions for memory operations
        void load(const decode_t &decode, libvio::width_t width, bool sign_extend);
        void store(const decode_t &decode, libvio::width_t width);

        // helper functions for trap handling
        virtual void raise_exception(WORD_T mcause, WORD_T mtval);
        virtual void handle_trap(void); // clear trap flags, set `next_trap` and `next_pc`

        // csr operations with no permmission check
        // used for emulating a hardware writing a csr
        virtual bool csr_check_read_access(uint16_t addr) const;
        virtual bool csr_check_write_access(uint16_t addr) const;
        virtual void csr_write(uint16_t addr, WORD_T value);
        virtual void csr_write_bits(uint16_t addr, WORD_T value, WORD_T bit_mask);
        virtual void csr_set_bits(uint16_t addr, WORD_T bits);
        virtual void csr_clear_bits(uint16_t addr, WORD_T bits);

    public:

        void reset(WORD_T init_pc) override;
        
        uint8_t n_gpr(void) const override;
        const char* gpr_name(uint8_t addr) const override;
        uint8_t gpr_addr(const char* name) const override;

        WORD_T get_gpr(uint8_t gpr_addr) const override;
        const WORD_T *get_gpr(void) const override;
        WORD_T get_pc(void) const override;

        void next_cycle(void) override;
        void next_instruction(void) override;
        bool stopped(void) const override;

        std::optional<WORD_T>pmem_peek(WORD_T addr, libvio::width_t width) const override;
        std::optional<WORD_T> get_trap(void) const override;

        static decode_t decode_instruction(uint32_t instruction);

        riscv::priv_level_t get_priv_level(void) const;
        virtual const std::vector<csr_def_t> &csr_list(void) const;
        virtual WORD_T csr_read(uint16_t addr) const;
        virtual WORD_T csr_read_bits(uint16_t addr, WORD_T bit_mask) const;

    protected:

        // ========== Instruction Implementations ==========
        // Arithmetic & Logical
        static void add(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sub(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sll(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void slt(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sltu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void xor_(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void srl(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sra(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void or_(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void and_(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // Immediate Operations
        static void addi(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void slti(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sltiu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void xori(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void ori(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void andi(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void slli(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void srli(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void srai(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // Memory Operations
        static void lb(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void lh(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void lw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void lbu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void lhu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sb(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sh(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // Control Flow
        static void jal(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void jalr(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void beq(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void bne(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void blt(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void bge(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void bltu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void bgeu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // Upper Immediate
        static void lui(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void auipc(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // Multiply/Divide
        static void mul(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void mulh(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void mulhsu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void mulhu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void div(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void divu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void rem(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void remu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // System
        static void ecall(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void ebreak(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // RV64 specific instructions
        static void lwu(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void ld(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sd(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void addiw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void slliw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void srliw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sraiw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void addw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void subw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sllw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void srlw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void sraw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);

        // csr functions with permission check
        // functions emulating the csr accesing instructions
        static void csrrw(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void csrrs(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void csrrc(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void csrrwi(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void csrrsi(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
        static void csrrci(riscv_cpu<WORD_T>* cpu, const decode_t& decode);
};

}

#endif
