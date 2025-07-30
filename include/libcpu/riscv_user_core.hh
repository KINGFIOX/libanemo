#ifndef LIBCPU_RISCV_USER_CORE_HH
#define LIBCPU_RISCV_USER_CORE_HH 

#include <cstdint>
#include <limits>
#include <libcpu/event.hh>
#include <libcpu/memory.hh>
#include <libcpu/memory_staging.hh>
#include <libcpu/riscv.hh>
#include <libvio/agent.hh>
#include <libvio/bus.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/width.hh>
#include <type_traits>

namespace libcpu {

/**
 * @brief RISC-V User Mode Core Template Class
 * 
 * @tparam WORD_T The word type for the RISC-V core (typically uint32_t or uint64_t)
 * 
 * This class implements a RISC-V user-mode core with instruction decoding and execution capabilities.
 * It supports the base integer instruction set (RV32I/RV64I) along with some additional extensions.
 */
template <typename WORD_T>
class riscv_user_core {
    public:
        WORD_T pc;      ///< Program counter register
        WORD_T gpr[32]; ///< General purpose registers (x0-x31)

        abstract_memory<WORD_T> *data_bus = nullptr;
        libvio::io_agent *mmio_bus;
        libvio::ringbuffer<event_t<WORD_T>> *event_buffer = nullptr;
        
        /**
         * @brief Enumeration of dispatchable instruction types
         */
        using dispatch_t = enum class dispatch_t {
            // Arithmetic & Logical
            add, sub, sll, slt, sltu, xor_, srl, sra, or_, and_,
            // Immediate Operations
            addi, slti, sltiu, xori, ori, andi, slli, srli, srai,
            // Memory Operations
            lb, lh, lw, lbu, lhu, sb, sh, sw,
            // Control Flow
            jal, jalr, beq, bne, blt, bge, bltu, bgeu,
            // Upper Immediate
            lui, auipc,
            // Multiply/Divide
            mul, mulh, mulhsu, mulhu, div, divu, rem, remu,
            // System
            ecall, ebreak,
            // RV64 specific instructions
            lwu, ld, sd, addiw, slliw, srliw, sraiw, addw, subw, sllw, srlw, sraw,
            // CSR functions
            csrrw, csrrs, csrrc, csrrwi, csrrsi, csrrci,
            // invalid instrucion
            invalid
        };

        /**
         * @brief Decoded instruction structure
         */
        using decode_t = struct decode_t {
            uint32_t instr;      ///< Raw instruction word
            dispatch_t dispatch; ///< Dispatched instruction type
            WORD_T imm;          ///< Decoded immediate value
            uint8_t rs1;         ///< Source register 1 index
            uint8_t rs2;         ///< Source register 2 index
            uint8_t rd;          ///< Destination register index
        };

        /**
         * @brief Execution result types
         */
        using exec_result_type_t = enum class exec_result_type_t {
            retire,       ///< Committed or trap handled
            trap,         ///< Trap firing
            privileged    ///< Privileged operation
        };

        /**
         * @brief Execution result structure
         *
         * This structure describes the modifications to unprivileged arhitectural states.
         * Modifications to privileged architectural states should be implemented with the priviliged module.
         * The privileged and unprivileged part should be decoupled for performance and code reusing.
         */
        using exec_result_t = struct exec_result_t {
            exec_result_type_t type; ///< Type of execution result
            WORD_T next_pc;
            union { 
                struct {
                    WORD_T mcause;    ///< Trap cause
                    WORD_T mtval;     ///< Trap value
                } trap;               ///< Trap firing data
                struct {
                    dispatch_t dispatch; ///< Privileged instruction type
                    uint8_t rs1;         ///< Source register 1
                    uint8_t rs2;         ///< Source register 2
                    uint8_t rd;          ///< Destination register
                    uint16_t zimm;       ///< Zero-extended immediate
                } privileged;            ///< Privileged operation data
            };
        };

        /**
         * @brief Decode a RISC-V instruction
         * 
         * @param instr The raw instruction word to decode
         * @return decode_t The decoded instruction information
         */
        static decode_t decode_instr(uint32_t instr);

        /**
         * @brief Execute a decoded instruction
         * 
         * @param decode The decoded instruction to execute
         * @return exec_result_t The execution result
         */
        exec_result_t decode_exec(decode_t decode);

    private:
        exec_result_t load(decode_t decode, libvio::width_t width, bool sign_extend);
        exec_result_t store(decode_t decode, libvio::width_t width);
};

// R-type
#define imm_r(WORD_T, instr) (0)
#define rs1_r(instr) (((instr) >> 15) & 0x1F)
#define rs2_r(instr) (((instr) >> 20) & 0x1F)
#define rd_r(instr)  (((instr) >> 7)  & 0x1F)

// I-type
#define imm_i(WORD_T, instr) ((std::make_signed_t<WORD_T>(instr)) >> 20)
#define rs1_i(instr) (((instr) >> 15) & 0x1F)
#define rs2_i(instr) (0)
#define rd_i(instr)  (((instr) >> 7)  & 0x1F)

// S-type
#define imm_s(WORD_T, instr) ((std::make_signed_t<WORD_T>((instr) & 0xfe000000) >> 20) | (((instr) >> 7) & 0x1f))
#define rs1_s(instr) (((instr) >> 15) & 0x1F)
#define rs2_s(instr) (((instr) >> 20) & 0x1F)
#define rd_s(instr)  (0)

// B-type
#define imm_b(WORD_T, instr) ((std::make_signed_t<WORD_T>((instr) & 0x80000000) >> 19) | (((instr) & 0x80) << 4) | (((instr) >> 20) & 0x7e0) | (((instr) >> 7) & 0x1e))
#define rs1_b(instr) (((instr) >> 15) & 0x1F)
#define rs2_b(instr) (((instr) >> 20) & 0x1F)
#define rd_b(instr)  (0)

// U-type
#define imm_u(WORD_T, instr) (std::make_signed_t<WORD_T>((instr)&0xfffff000))
#define rs1_u(instr) (0)
#define rs2_u(instr) (0)
#define rd_u(instr)  (((instr) >> 7)  & 0x1F)

// J-type
#define imm_j(WORD_T, instr) ((std::make_signed_t<WORD_T>((instr) & 0x80000000) >> 11) | ((instr) & 0xff000) | (((instr) >> 9) & 0x800) | (((instr) >> 20) & 0x7fe))
#define rs1_j(instr) (0)
#define rs2_j(instr) (0)
#define rd_j(instr)  (((instr) >> 7)  & 0x1F)

#define RISCV_INSTR_PAT(pattern, mask, type, operation) \
    if (((instr^pattern)&mask) == 0) { \
        decode.dispatch = dispatch_t::operation; \
        decode.imm = imm_##type(WORD_T, instr); \
        decode.rs1 = rs1_##type(instr);\
        decode.rs2 = rs2_##type(instr);\
        decode.rd  = rd_##type(instr);\
    } else

template <typename WORD_T>
riscv_user_core<WORD_T>::decode_t riscv_user_core<WORD_T>::decode_instr(uint32_t instr) {
    decode_t decode;
    decode.instr = instr;

    // U-type instructions
    RISCV_INSTR_PAT(0b00000000000000000000000000110111, 0b00000000000000000000000001111111, u, lui)
    RISCV_INSTR_PAT(0b00000000000000000000000000010111, 0b00000000000000000000000001111111, u, auipc)

    // J-type
    RISCV_INSTR_PAT(0b00000000000000000000000001101111, 0b00000000000000000000000001111111, j, jal)

    // I-type (jalr)
    RISCV_INSTR_PAT(0b00000000000000000000000001100111, 0b00000000000000000111000001111111, i, jalr)

    // B-type
    RISCV_INSTR_PAT(0b00000000000000000000000001100011, 0b00000000000000000111000001111111, b, beq)
    RISCV_INSTR_PAT(0b00000000000000000001000001100011, 0b00000000000000000111000001111111, b, bne)
    RISCV_INSTR_PAT(0b00000000000000000100000001100011, 0b00000000000000000111000001111111, b, blt)
    RISCV_INSTR_PAT(0b00000000000000000101000001100011, 0b00000000000000000111000001111111, b, bge)
    RISCV_INSTR_PAT(0b00000000000000000110000001100011, 0b00000000000000000111000001111111, b, bltu)
    RISCV_INSTR_PAT(0b00000000000000000111000001100011, 0b00000000000000000111000001111111, b, bgeu)

    // Loads (I-type)
    RISCV_INSTR_PAT(0b00000000000000000000000000000011, 0b00000000000000000111000001111111, i, lb)
    RISCV_INSTR_PAT(0b00000000000000000001000000000011, 0b00000000000000000111000001111111, i, lh)
    RISCV_INSTR_PAT(0b00000000000000000010000000000011, 0b00000000000000000111000001111111, i, lw)
    RISCV_INSTR_PAT(0b00000000000000000100000000000011, 0b00000000000000000111000001111111, i, lbu)
    RISCV_INSTR_PAT(0b00000000000000000101000000000011, 0b00000000000000000111000001111111, i, lhu)

    // Stores (S-type)
    RISCV_INSTR_PAT(0b00000000000000000000000000100011, 0b00000000000000000111000001111111, s, sb)
    RISCV_INSTR_PAT(0b00000000000000000001000000100011, 0b00000000000000000111000001111111, s, sh)
    RISCV_INSTR_PAT(0b00000000000000000010000000100011, 0b00000000000000000111000001111111, s, sw)

    // I-type ALU
    RISCV_INSTR_PAT(0b00000000000000000000000000010011, 0b00000000000000000111000001111111, i, addi)
    RISCV_INSTR_PAT(0b00000000000000000010000000010011, 0b00000000000000000111000001111111, i, slti)
    RISCV_INSTR_PAT(0b00000000000000000011000000010011, 0b00000000000000000111000001111111, i, sltiu)
    RISCV_INSTR_PAT(0b00000000000000000100000000010011, 0b00000000000000000111000001111111, i, xori)
    RISCV_INSTR_PAT(0b00000000000000000110000000010011, 0b00000000000000000111000001111111, i, ori)
    RISCV_INSTR_PAT(0b00000000000000000111000000010011, 0b00000000000000000111000001111111, i, andi)
    RISCV_INSTR_PAT(0b00000000000000000001000000010011, 0b11111100000000000111000001111111, i, slli)
    RISCV_INSTR_PAT(0b00000000000000000101000000010011, 0b11111100000000000111000001111111, i, srli)
    RISCV_INSTR_PAT(0b01000000000000000101000000010011, 0b11111100000000000111000001111111, i, srai)

    // R-type
    RISCV_INSTR_PAT(0b00000000000000000000000000110011, 0b11111110000000000111000001111111, r, add)
    RISCV_INSTR_PAT(0b01000000000000000000000000110011, 0b11111110000000000111000001111111, r, sub)
    RISCV_INSTR_PAT(0b00000000000000000001000000110011, 0b11111110000000000111000001111111, r, sll)
    RISCV_INSTR_PAT(0b00000000000000000010000000110011, 0b11111110000000000111000001111111, r, slt)
    RISCV_INSTR_PAT(0b00000000000000000011000000110011, 0b11111110000000000111000001111111, r, sltu)
    RISCV_INSTR_PAT(0b00000000000000000100000000110011, 0b11111110000000000111000001111111, r, xor_)
    RISCV_INSTR_PAT(0b00000000000000000101000000110011, 0b11111110000000000111000001111111, r, srl)
    RISCV_INSTR_PAT(0b01000000000000000101000000110011, 0b11111110000000000111000001111111, r, sra)
    RISCV_INSTR_PAT(0b00000000000000000110000000110011, 0b11111110000000000111000001111111, r, or_)
    RISCV_INSTR_PAT(0b00000000000000000111000000110011, 0b11111110000000000111000001111111, r, and_)

    // M-extension
    RISCV_INSTR_PAT(0b00000010000000000000000000110011, 0b11111110000000000111000001111111, r, mul)
    RISCV_INSTR_PAT(0b00000010000000000001000000110011, 0b11111110000000000111000001111111, r, mulh)
    RISCV_INSTR_PAT(0b00000010000000000010000000110011, 0b11111110000000000111000001111111, r, mulhsu)
    RISCV_INSTR_PAT(0b00000010000000000011000000110011, 0b11111110000000000111000001111111, r, mulhu)
    RISCV_INSTR_PAT(0b00000010000000000100000000110011, 0b11111110000000000111000001111111, r, div)
    RISCV_INSTR_PAT(0b00000010000000000101000000110011, 0b11111110000000000111000001111111, r, divu)
    RISCV_INSTR_PAT(0b00000010000000000110000000110011, 0b11111110000000000111000001111111, r, rem)
    RISCV_INSTR_PAT(0b00000010000000000111000000110011, 0b11111110000000000111000001111111, r, remu)

    // System
    RISCV_INSTR_PAT(0b00000000000000000000000001110011, 0b11111111111111111111111111111111, r, ecall)
    RISCV_INSTR_PAT(0b00000000000100000000000001110011, 0b11111111111111111111111111111111, r, ebreak)

    // CSR operations
    RISCV_INSTR_PAT(0b00000000000000000001000001110011, 0b00000000000000000111000001111111, i, csrrw)
    RISCV_INSTR_PAT(0b00000000000000000010000001110011, 0b00000000000000000111000001111111, i, csrrs)
    RISCV_INSTR_PAT(0b00000000000000000011000001110011, 0b00000000000000000111000001111111, i, csrrc)
    RISCV_INSTR_PAT(0b00000000000000000101000001110011, 0b00000000000000000111000001111111, i, csrrwi)
    RISCV_INSTR_PAT(0b00000000000000000110000001110011, 0b00000000000000000111000001111111, i, csrrsi)
    RISCV_INSTR_PAT(0b00000000000000000111000001110011, 0b00000000000000000111000001111111, i, csrrci)
    RISCV_INSTR_PAT(0b00000000000000000110000000000011, 0b00000000000000000111000001111111, i, lwu)
    RISCV_INSTR_PAT(0b00000000000000000011000000000011, 0b00000000000000000111000001111111, i, ld)
    RISCV_INSTR_PAT(0b00000000000000000011000000100011, 0b00000000000000000111000001111111, s, sd)
    RISCV_INSTR_PAT(0b00000000000000000000000000011011, 0b00000000000000000111000001111111, i, addiw)
    RISCV_INSTR_PAT(0b00000000000000000001000000011011, 0b11111110000000000111000001111111, i, slliw)
    RISCV_INSTR_PAT(0b00000000000000000101000000011011, 0b11111110000000000111000001111111, i, srliw)
    RISCV_INSTR_PAT(0b01000000000000000101000000011011, 0b11111110000000000111000001111111, i, sraiw)
    RISCV_INSTR_PAT(0b00000000000000000000000000111011, 0b11111110000000000111000001111111, r, addw)
    RISCV_INSTR_PAT(0b01000000000000000000000000111011, 0b11111110000000000111000001111111, r, subw)
    RISCV_INSTR_PAT(0b00000000000000000001000000111011, 0b11111110000000000111000001111111, r, sllw)
    RISCV_INSTR_PAT(0b00000000000000000101000000111011, 0b11111110000000000111000001111111, r, srlw)
    RISCV_INSTR_PAT(0b01000000000000000101000000111011, 0b11111110000000000111000001111111, r, sraw)
    // Fallback case for invalid instructions
    { decode = {.instr=instr, .dispatch=dispatch_t::invalid, .imm=0, .rs1=0, .rs2=0, .rd=0}; }

    return decode;
}

// R-type
#undef imm_r
#undef rs1_r
#undef rs2_r
#undef rd_r

// I-type
#undef imm_i
#undef rs1_i
#undef rs2_i
#undef rd_i

// S-type
#undef imm_s
#undef rs1_s
#undef rs2_s
#undef rd_s

// B-type
#undef imm_b
#undef rs1_b
#undef rs2_b
#undef rd_b

// U-type
#undef imm_u
#undef rs1_u
#undef rs2_u
#undef rd_u

// J-type
#undef imm_j
#undef rs1_j
#undef rs2_j
#undef rd_j

#undef RISCV_INSTR_PAT

#define invalid_instruction() \
    res.type = exec_result_type_t::trap; \
    res.trap.mcause = riscv::mcause<WORD_T>::except_illegal_instr; \
    res.trap.mtval = decode.instr;

template <typename WORD_T>
riscv_user_core<WORD_T>::exec_result_t riscv_user_core<WORD_T>::decode_exec(decode_t decode) {
    constexpr bool is_rv64 = sizeof(WORD_T) * CHAR_BIT == 64;

    if (event_buffer!=nullptr) {
        event_buffer->push_back({.type=event_type_t::issue, .pc=pc, .val1=decode.instr, .val2=0});
    }

    exec_result_t res;
    WORD_T next_pc = (decode.instr&3)==3 ? pc+4 : pc+2;

    switch (decode.dispatch) {
        // Arithmetic & Logical
        case dispatch_t::add: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] + gpr[decode.rs2];
            break;
        }
        case dispatch_t::sub: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] - gpr[decode.rs2];
            break;
        }
        case dispatch_t::sll: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                gpr[decode.rd] = gpr[decode.rs1] << (gpr[decode.rs2] & 0x3F);
            } else {
                gpr[decode.rd] = gpr[decode.rs1] << (gpr[decode.rs2] & 0x1F);
            }
            break;
        }
        case dispatch_t::slt: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = (std::make_signed_t<WORD_T>(gpr[decode.rs1]) < std::make_signed_t<WORD_T>(gpr[decode.rs2])) ? 1 : 0;
            break;
        }
        case dispatch_t::sltu: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = (gpr[decode.rs1] < gpr[decode.rs2]) ? 1 : 0;
            break;
        }
        case dispatch_t::xor_: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] ^ gpr[decode.rs2];
            break;
        }
        case dispatch_t::srl: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                gpr[decode.rd] = gpr[decode.rs1] >> (gpr[decode.rs2] & 0x3F);
            } else {
                gpr[decode.rd] = gpr[decode.rs1] >> (gpr[decode.rs2] & 0x1F);
            }
            break;
        }
        case dispatch_t::sra: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                gpr[decode.rd] = std::make_signed_t<WORD_T>(gpr[decode.rs1]) >> (gpr[decode.rs2] & 0x3F);
            } else {
                gpr[decode.rd] = std::make_signed_t<WORD_T>(gpr[decode.rs1]) >> (gpr[decode.rs2] & 0x1F);
            }
            break;
        }
        case dispatch_t::or_: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] | gpr[decode.rs2];
            break;
        }
        case dispatch_t::and_: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] & gpr[decode.rs2];
            break;
        }

        // Immediate Operations
        case dispatch_t::addi: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] + decode.imm;
            break;
        }
        case dispatch_t::slti: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = (std::make_signed_t<WORD_T>(gpr[decode.rs1]) < std::make_signed_t<WORD_T>(decode.imm)) ? 1 : 0;
            break;
        }
        case dispatch_t::sltiu: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = (gpr[decode.rs1] < static_cast<std::make_unsigned_t<WORD_T>>(decode.imm)) ? 1 : 0;
            break;
        }
        case dispatch_t::xori: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] ^ decode.imm;
            break;
        }
        case dispatch_t::ori: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] | decode.imm;
            break;
        }
        case dispatch_t::andi: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] & decode.imm;
            break;
        }
        case dispatch_t::slli: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                gpr[decode.rd] = gpr[decode.rs1] << (decode.imm & 0x3F);
            } else {
                gpr[decode.rd] = gpr[decode.rs1] << (decode.imm & 0x1F);
            }
            break;
        }
        case dispatch_t::srli: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                gpr[decode.rd] = gpr[decode.rs1] >> (decode.imm & 0x3F);
            } else {
                gpr[decode.rd] = gpr[decode.rs1] >> (decode.imm & 0x1F);
            }
            break;
        }
        case dispatch_t::srai: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                gpr[decode.rd] = std::make_signed_t<WORD_T>(gpr[decode.rs1]) >> (decode.imm & 0x3F);
            } else {
                gpr[decode.rd] = std::make_signed_t<WORD_T>(gpr[decode.rs1]) >> (decode.imm & 0x1F);
            }
            break;
        }

        // Memory Operations
        case dispatch_t::lb: {
            res = load(decode, libvio::width_t::byte, true);
            break;
        }
        case dispatch_t::lh: {
            res = load(decode, libvio::width_t::half, true);
            break;
        }
        case dispatch_t::lw: {
            res = load(decode, libvio::width_t::word, true);
            break;
        }
        case dispatch_t::lbu: {
            res = load(decode, libvio::width_t::byte, false);
            break;
        }
        case dispatch_t::lhu: {
            res = load(decode, libvio::width_t::half, false);
            break;
        }
        case dispatch_t::sb: {
            res = store(decode, libvio::width_t::byte);
            break;
        }
        case dispatch_t::sh: {
            res = store(decode, libvio::width_t::half);
            break;
        }
        case dispatch_t::sw: {
            res = store(decode, libvio::width_t::word);
            break;
        }

        // Control Flow
        case dispatch_t::jal: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = next_pc;
            next_pc = pc + decode.imm;
            break;
        }
        case dispatch_t::jalr: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = next_pc;
            next_pc = (gpr[decode.rs1] + decode.imm) & ~static_cast<WORD_T>(1);
            break;
        }
        case dispatch_t::beq: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = 0;
            next_pc = (gpr[decode.rs1] == gpr[decode.rs2]) ? pc + decode.imm : next_pc;
            break;
        }
        case dispatch_t::bne: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = 0;
            next_pc = (gpr[decode.rs1] != gpr[decode.rs2]) ? pc + decode.imm : next_pc;
            break;
        }
        case dispatch_t::blt: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = 0;
            bool taken = std::make_signed_t<WORD_T>(gpr[decode.rs1]) < std::make_signed_t<WORD_T>(gpr[decode.rs2]);
            next_pc = taken ? pc + decode.imm : next_pc;
            break;
        }
        case dispatch_t::bge: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = 0;
            bool taken = std::make_signed_t<WORD_T>(gpr[decode.rs1]) >= std::make_signed_t<WORD_T>(gpr[decode.rs2]);
            next_pc = taken ? pc + decode.imm : next_pc;
            break;
        }
        case dispatch_t::bltu: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = 0;
            bool taken = gpr[decode.rs1] < gpr[decode.rs2];
            next_pc = taken ? pc + decode.imm : next_pc;
            break;
        }
        case dispatch_t::bgeu: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = 0;
            bool taken = gpr[decode.rs1] >= gpr[decode.rs2];
            next_pc = taken ? pc + decode.imm : next_pc;
            break;
        }

        // Upper Immediate
        case dispatch_t::lui: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = decode.imm;
            break;
        }
        case dispatch_t::auipc: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = pc + decode.imm;
            break;
        }

        // Multiply/Divide
        case dispatch_t::mul: {
            res.type = exec_result_type_t::retire;
            gpr[decode.rd] = gpr[decode.rs1] * gpr[decode.rs2];
            break;
        }
        case dispatch_t::mulh: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                uint64_t a = gpr[decode.rs1];
                uint64_t b = gpr[decode.rs2];
                __int128 result = static_cast<__int128>(static_cast<int64_t>(a)) * static_cast<__int128>(static_cast<int64_t>(b));
                gpr[decode.rd] = static_cast<WORD_T>(result >> 64);
            } else {
                int64_t result = static_cast<int64_t>(static_cast<int32_t>(gpr[decode.rs1])) * static_cast<int64_t>(static_cast<int32_t>(gpr[decode.rs2]));
                gpr[decode.rd] = static_cast<WORD_T>(result >> 32);
            }
            break;
        }
        case dispatch_t::mulhsu: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                uint64_t a = gpr[decode.rs1];
                uint64_t b = gpr[decode.rs2];
                __int128 result = static_cast<__int128>(static_cast<int64_t>(a)) * static_cast<__int128>(b);
                gpr[decode.rd] = static_cast<WORD_T>(result >> 64);
            } else {
                int64_t result = static_cast<int64_t>(static_cast<int32_t>(gpr[decode.rs1])) * static_cast<int64_t>(gpr[decode.rs2]);
                gpr[decode.rd] = static_cast<WORD_T>(result >> 32);
            }
            break;
        }
        case dispatch_t::mulhu: {
            res.type = exec_result_type_t::retire;
            if constexpr (is_rv64) {
                uint64_t a = gpr[decode.rs1];
                uint64_t b = gpr[decode.rs2];
                __uint128_t result = static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b);
                gpr[decode.rd] = static_cast<WORD_T>(result >> 64);
            } else {
                uint64_t result = static_cast<uint64_t>(gpr[decode.rs1]) * static_cast<uint64_t>(gpr[decode.rs2]);
                gpr[decode.rd] = static_cast<WORD_T>(result >> 32);
            }
            break;
        }
        case dispatch_t::div: {
            res.type = exec_result_type_t::retire;
            std::make_signed_t<WORD_T> a = gpr[decode.rs1];
            std::make_signed_t<WORD_T> b = gpr[decode.rs2];
            if (b == 0) {
                gpr[decode.rd] = static_cast<WORD_T>(-1);
            } else if (a == std::numeric_limits<std::make_signed_t<WORD_T>>::min() && b == -1) {
                gpr[decode.rd] = a;
            } else {
                gpr[decode.rd] = static_cast<WORD_T>(a / b);
            }
            break;
        }
        case dispatch_t::divu: {
            res.type = exec_result_type_t::retire;
            std::make_unsigned_t<WORD_T> a = gpr[decode.rs1];
            std::make_unsigned_t<WORD_T> b = gpr[decode.rs2];
            if (b == 0) {
                gpr[decode.rd] = static_cast<WORD_T>(-1);
            } else {
                gpr[decode.rd] = static_cast<WORD_T>(a / b);
            }
            break;
        }
        case dispatch_t::rem: {
            res.type = exec_result_type_t::retire;
            std::make_signed_t<WORD_T> a = gpr[decode.rs1];
            std::make_signed_t<WORD_T> b = gpr[decode.rs2];
            if (b == 0) {
                gpr[decode.rd] = a;
            } else if (a == std::numeric_limits<std::make_signed_t<WORD_T>>::min() && b == -1) {
                gpr[decode.rd] = 0;
            } else {
                gpr[decode.rd] = static_cast<WORD_T>(a % b);
            }
            break;
        }
        case dispatch_t::remu: {
            res.type = exec_result_type_t::retire;
            std::make_unsigned_t<WORD_T> a = gpr[decode.rs1];
            std::make_unsigned_t<WORD_T> b = gpr[decode.rs2];
            if (b == 0) {
                gpr[decode.rd] = a;
            } else {
                gpr[decode.rd] = static_cast<WORD_T>(a % b);
            }
            break;
        }

        // System
        case dispatch_t::ebreak: {
            res.type = exec_result_type_t::trap;
            res.trap.mcause = riscv::mcause<WORD_T>::except_breakpoint;
            res.trap.mtval = 0;
            break;
        };
        case dispatch_t::ecall: {
            res.type = exec_result_type_t::privileged;
            res.privileged.dispatch = dispatch_t::ecall;
        }
        case dispatch_t::csrrw:
        case dispatch_t::csrrs:
        case dispatch_t::csrrc: {
            res.type = exec_result_type_t::privileged;
            res.privileged.dispatch = decode.dispatch;
            res.privileged.rs1 = decode.rs1;
            res.privileged.rd = decode.rd;
            break;
        }
        case dispatch_t::csrrwi:
        case dispatch_t::csrrsi:
        case dispatch_t::csrrci: {
            res.type = exec_result_type_t::privileged;
            res.privileged.dispatch = decode.dispatch;
            res.privileged.zimm = static_cast<uint16_t>(decode.imm) & 0x1F;
            res.privileged.rd = decode.rd;
            break;
        }

        // RV64 specific instructions
        case dispatch_t::lwu: {
            if constexpr (is_rv64) {
            res = load(decode, libvio::width_t::word, false);
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::ld: {
            if constexpr (is_rv64) {
            res = load(decode, libvio::width_t::dword, true);
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sd: {
            if constexpr (is_rv64) {
                res = store(decode, libvio::width_t::dword);
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::addiw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    int32_t a = static_cast<int32_t>(gpr[decode.rs1]);
                int32_t b = static_cast<int32_t>(decode.imm);
                int32_t res32 = a + b;
                gpr[decode.rd] = static_cast<WORD_T>(static_cast<int64_t>(res32));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::slliw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    uint8_t shamt = decode.imm & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[decode.rs1]);
                val <<= shamt;
                gpr[decode.rd] = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::srliw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    uint8_t shamt = decode.imm & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[decode.rs1]);
                val >>= shamt;
                gpr[decode.rd] = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sraiw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    uint8_t shamt = decode.imm & 0x1F;
                int32_t val = static_cast<int32_t>(gpr[decode.rs1]);
                val >>= shamt;
                gpr[decode.rd] = static_cast<WORD_T>(val);
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::addw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    int32_t a = static_cast<int32_t>(gpr[decode.rs1]);
                int32_t b = static_cast<int32_t>(gpr[decode.rs2]);
                int32_t res32 = a + b;
                gpr[decode.rd] = static_cast<WORD_T>(static_cast<int64_t>(res32));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::subw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    int32_t a = static_cast<int32_t>(gpr[decode.rs1]);
                int32_t b = static_cast<int32_t>(gpr[decode.rs2]);
                int32_t res32 = a - b;
                gpr[decode.rd] = static_cast<WORD_T>(static_cast<int64_t>(res32));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sllw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    uint8_t shamt = gpr[decode.rs2] & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[decode.rs1]);
                val <<= shamt;
                gpr[decode.rd] = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::srlw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    uint8_t shamt = gpr[decode.rs2] & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[decode.rs1]);
                val >>= shamt;
                gpr[decode.rd] = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sraw: {
            if constexpr (is_rv64) {
                res.type = exec_result_type_t::retire;
                    uint8_t shamt = gpr[decode.rs2] & 0x1F;
                int32_t val = static_cast<int32_t>(gpr[decode.rs1]);
                val >>= shamt;
                gpr[decode.rd] = static_cast<WORD_T>(val);
            } else {
                invalid_instruction();
            }
            break;
        }

        // invalid instruction
        case dispatch_t::invalid: {
            invalid_instruction();
            break;
        }
    }

    gpr[0] = 0;
    if (res.type==exec_result_type_t::retire) {
        if (decode.rd!=0 && event_buffer!=nullptr) {
            event_buffer->push_back({.type=event_type_t::reg_write, .pc=pc, .val1=decode.rd, .val2=gpr[decode.rd]});
        }
        pc= next_pc;
    }

    return res;
}

#undef invalid_instruction

template <typename WORD_T>
riscv_user_core<WORD_T>::exec_result_t riscv_user_core<WORD_T>::load(decode_t decode, libvio::width_t width, bool sign_extend) {
    exec_result_t res = {.type=exec_result_type_t::retire};
    WORD_T addr = gpr[decode.rs1] + decode.imm;
    std::optional<uint64_t> data_opt = data_bus->read(addr, width);
    // fall back to MMIO if the address is out of RAM
    if (!data_opt.has_value() && mmio_bus!=nullptr) {
        data_opt = mmio_bus->read(addr, width);
    }
    if (data_opt.has_value()) {
        // `data` is zero extended
        WORD_T data = data_opt.value();
        // log the memory operation with *zero extended* data
        if (event_buffer != nullptr) {
            event_buffer->push_back({.type=event_type_t::load, .pc=pc, .val1=addr, .val2=data_opt.value()});
        }
        // sign extend the data
        if (sign_extend) {
            data = libvio::sign_extend<WORD_T>(data, width);
        }
        gpr[decode.rd] = data;
    } else {
        // both RAM and MMIO failed
        res.type = exec_result_type_t::trap;
        res.trap.mcause = riscv::mcause<WORD_T>::except_load_fault;
        res.trap.mtval = addr;
    }
    return res;
}

template <typename WORD_T>
riscv_user_core<WORD_T>::exec_result_t riscv_user_core<WORD_T>::store(decode_t decode, libvio::width_t width) {
    exec_result_t res = {.type=exec_result_type_t::retire};
    WORD_T addr = gpr[decode.rs1] + decode.imm;
    WORD_T data = libvio::zero_truncate<WORD_T>(gpr[decode.rs2], width);
    bool success = data_bus->write(addr, width, data);
    // fall back to MMIO
    if (!success && mmio_bus!=nullptr) {
        success = mmio_bus->write(addr, width, data);
    }
    if (success) {
        if (event_buffer!=nullptr) {
            event_buffer->push_back({.type=event_type_t::store, .pc=pc, .val1=addr, .val2=data});
        }
    } else {
        // both RAM and MMIO failed
        res.type = exec_result_type_t::trap;
        res.trap.mcause = riscv::mcause<WORD_T>::except_store_fault;
        res.trap.mtval = addr;
    }
    return res;
}

}

#endif
