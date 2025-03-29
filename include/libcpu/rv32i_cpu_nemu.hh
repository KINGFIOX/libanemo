#ifndef LIBCPU_RV32I_CPU_NEMU_HH
#define LIBCPU_RV32I_CPU_NEMU_HH

#include <libcpu/rv32i_cpu.hh>
#include <libcpu/rv32i.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/frontend.hh>
#include <libvio/bus.hh>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace libcpu {

class rv32i_cpu_nemu: public rv32i_cpu {

    public:

        static constexpr size_t n_csr = 10;
        static constexpr size_t n_interrupts = 16;

        using decode_t = struct decode_t {
            void (*op)(rv32i_cpu_nemu* cpu, const decode_t& decode);
            uint32_t instr;
            uint32_t imm;
            uint_fast8_t rs1;
            uint_fast8_t rs2;
            uint_fast8_t rd;
        };

        using csr_info_t = struct csr_info_t {
            uint32_t init_value;  // Purrfect starting value
            uint32_t wpri_mask;   // Masking like a cat hides its claws
            const char *name;     // The name's the game, meow
            csr_addr_t addr;      // Address? I prefer cardboard boxes
        };

        static const rv32i_cpu_nemu::csr_info_t csr_info[libcpu::rv32i_cpu_nemu::n_csr];

    private:
        // architectural state
        std::array<uint32_t, n_gpr> gpr;
        std::array<uint32_t, n_csr> csr;
        uint32_t pc = 0x80000000;
        priv_level_t priv_level;

        uint32_t instruction;
        uint32_t next_pc;

        std::vector<decode_t> decode_cache;

        bool exception_flag = false;
        bool ebreak_flag = false;
        uint32_t exception_cause;
        uint32_t exception_mtval;

        void raise_exception(mcause_t mcause, uint32_t mtval);
        uint32_t handle_trap(void);

        // csr operations with no permmission check
        // used for emulating a hardware writing a csr
        bool csr_check_read_access(csr_addr_t addr) const;
        bool csr_check_write_access(csr_addr_t addr) const;
        void csr_write(csr_addr_t addr, uint32_t value);
        void csr_write_bits(csr_addr_t addr, uint32_t value, uint32_t bit_mask);
        void csr_set_bits(csr_addr_t addr, uint32_t bits);
        void csr_clear_bits(csr_addr_t addr, uint32_t bits);


    public:

        uint32_t gpr_read(uint8_t gpr_addr) const override;
        uint32_t get_pc(void) const override;
        priv_level_t get_priv_level(void) const override;
        const uint32_t* get_regfile(void) const override;

        rv32i_cpu_nemu(size_t event_buffer_size, size_t memory_size, libvio::bus* mmio_bus);

        void next_cycle(void) override;
        void next_instruction(void) override;
        bool get_ebreak(void) const override;

        void raise_interrupt(mcause_t mcause) override;

        uint32_t csr_read(csr_addr_t addr) const override;
        uint32_t csr_read_bits(csr_addr_t addr, uint32_t bit_mask) const override;

        static decode_t decode_instruction(uint32_t instruction);

    private:

        // ========== Instruction Implementations ==========
        // Arithmetic & Logical
        static void add(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sub(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sll(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void slt(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sltu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void xor_(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void srl(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sra(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void or_(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void and_(rv32i_cpu_nemu* cpu, const decode_t& decode);

        // Immediate Operations
        static void addi(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void slti(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sltiu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void xori(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void ori(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void andi(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void slli(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void srli(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void srai(rv32i_cpu_nemu* cpu, const decode_t& decode);

        // Memory Operations
        static void lb(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void lh(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void lw(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void lbu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void lhu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sb(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sh(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void sw(rv32i_cpu_nemu* cpu, const decode_t& decode);

        // Control Flow
        static void jal(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void jalr(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void beq(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void bne(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void blt(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void bge(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void bltu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void bgeu(rv32i_cpu_nemu* cpu, const decode_t& decode);

        // Upper Immediate
        static void lui(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void auipc(rv32i_cpu_nemu* cpu, const decode_t& decode);

        // Multiply/Divide
        static void mul(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void mulh(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void mulhsu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void mulhu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void div(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void divu(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void rem(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void remu(rv32i_cpu_nemu* cpu, const decode_t& decode);

        // System
        static void ecall(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void ebreak(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void mret(rv32i_cpu_nemu* cpu, const decode_t& decode);
        // csr functions with permission check
        // functions emulating the csr accesing instructions
        static void csrrw(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void csrrs(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void csrrc(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void csrrwi(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void csrrsi(rv32i_cpu_nemu* cpu, const decode_t& decode);
        static void csrrci(rv32i_cpu_nemu* cpu, const decode_t& decode);
};

}

#endif
