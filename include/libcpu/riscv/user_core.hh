#ifndef LIBCPU_RISCV_USER_CORE_HH
#define LIBCPU_RISCV_USER_CORE_HH 

#include <cassert>
#include <cstdint>
#include <limits>
#include <libcpu/riscv/riscv.hh>
#include <libvio/width.hh>
#include <type_traits>

namespace libcpu::riscv {

/**
 * @brief RISC-V User Mode Core Template Class
 * 
 * @tparam WORD_T The word type for the RISC-V core (typically uint32_t or uint64_t)
 * 
 * This class implements a RISC-V user-mode core with instruction decoding and execution capabilities.
 * It supports the base integer instruction set (RV32I/RV64I) along with some additional extensions.
 */
template <typename WORD_T>
class user_core {
    public:
        using dispatch_t  = riscv::dispatch_t;
        using decode_t = riscv::decode_t;
        using exec_result_type_t = riscv::exec_result_type_t;
        using exec_result_t = riscv::exec_result_t<WORD_T>;

        WORD_T gpr[32]; ///< General purpose registers (x0-x31)
        
        /**
         * @brief Reset the state of the user core
         * 
         * @param init_pc The program counter of the first instruction to execute
         */
        void reset(void);

        static void decode(exec_result_t &op);
        void execute(exec_result_t &op);
};

template <typename WORD_T>
void user_core<WORD_T>::reset(void) {
    for (auto &i: gpr) {
        i = 0;
    }
}

// R-type
#define imm_r(WORD_T, instr) (0)
#define rs1_r(instr) (((instr) >> 15) & 0x1F)
#define rs2_r(instr) (((instr) >> 20) & 0x1F)
#define rd_r(instr)  (((instr) >> 7)  & 0x1F)

// I-type
#define imm_i(WORD_T, instr) ((int32_t(instr)) >> 20)
#define rs1_i(instr) (((instr) >> 15) & 0x1F)
#define rs2_i(instr) (0)
#define rd_i(instr)  (((instr) >> 7)  & 0x1F)

// S-type
#define imm_s(WORD_T, instr) ((int32_t((instr) & 0xfe000000) >> 20) | (((instr) >> 7) & 0x1f))
#define rs1_s(instr) (((instr) >> 15) & 0x1F)
#define rs2_s(instr) (((instr) >> 20) & 0x1F)
#define rd_s(instr)  (0)

// B-type
#define imm_b(WORD_T, instr) ((int32_t((instr) & 0x80000000) >> 19) | (((instr) & 0x80) << 4) | (((instr) >> 20) & 0x7e0) | (((instr) >> 7) & 0x1e))
#define rs1_b(instr) (((instr) >> 15) & 0x1F)
#define rs2_b(instr) (((instr) >> 20) & 0x1F)
#define rd_b(instr)  (0)

// U-type
#define imm_u(WORD_T, instr) (int32_t((instr)&0xfffff000))
#define rs1_u(instr) (0)
#define rs2_u(instr) (0)
#define rd_u(instr)  (((instr) >> 7)  & 0x1F)

// J-type
#define imm_j(WORD_T, instr) ((int32_t((instr) & 0x80000000) >> 11) | ((instr) & 0xff000) | (((instr) >> 9) & 0x800) | (((instr) >> 20) & 0x7fe))
#define rs1_j(instr) (0)
#define rs2_j(instr) (0)
#define rd_j(instr)  (((instr) >> 7)  & 0x1F)

#define RISCV_INSTR_PAT(pattern, mask, encoding, operation) \
    if (((instr^pattern)&mask) == 0) { \
        op.type = exec_result_type_t::decode; \
        op.decode.dispatch = dispatch_t::operation; \
        op.decode.imm = imm_##encoding(WORD_T, instr); \
        op.decode.rs1 = rs1_##encoding(instr);\
        op.decode.rs2 = rs2_##encoding(instr);\
        op.decode.rd  = rd_##encoding(instr);\
    } else

#define RISCV_INSTR_PAT_END \
    { \
        op.type = exec_result_type_t::decode; \
        op.decode.dispatch = dispatch_t::invalid; \
    }

template <typename WORD_T>
void user_core<WORD_T>::decode(exec_result_t &op) {
    assert(op.type == exec_result_type_t::fetch);
    WORD_T instr = op.instr;

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
    RISCV_INSTR_PAT(0b00110000001000000000000001110011, 0b11111111111111111111111111111111, r, mret)
    RISCV_INSTR_PAT(0b00010000001000000000000001110011, 0b11111111111111111111111111111111, r, sret)

    // CSR operations
    RISCV_INSTR_PAT(0b00000000000000000001000001110011, 0b00000000000000000111000001111111, i, csrrw)
    RISCV_INSTR_PAT(0b00000000000000000010000001110011, 0b00000000000000000111000001111111, i, csrrs)
    RISCV_INSTR_PAT(0b00000000000000000011000001110011, 0b00000000000000000111000001111111, i, csrrc)
    RISCV_INSTR_PAT(0b00000000000000000101000001110011, 0b00000000000000000111000001111111, i, csrrwi)
    RISCV_INSTR_PAT(0b00000000000000000110000001110011, 0b00000000000000000111000001111111, i, csrrsi)
    RISCV_INSTR_PAT(0b00000000000000000111000001110011, 0b00000000000000000111000001111111, i, csrrci)

    // RV64 additions
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
    RISCV_INSTR_PAT(0b00000010000000000000000000111011, 0b11111110000000000111000001111111, r, mulw)
    RISCV_INSTR_PAT(0b00000010000000000100000000111011, 0b11111110000000000111000001111111, r, divw)
    RISCV_INSTR_PAT(0b00000010000000000101000000111011, 0b11111110000000000111000001111111, r, divuw)
    RISCV_INSTR_PAT(0b00000010000000000110000000111011, 0b11111110000000000111000001111111, r, remw)
    RISCV_INSTR_PAT(0b00000010000000000111000000111011, 0b11111110000000000111000001111111, r, remuw)
    RISCV_INSTR_PAT_END
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
#undef RISCV_INSTR_PAT_END

#define invalid_instruction() \
    op.type = exec_result_type_t::trap; \
    op.trap.cause = riscv::mcause<WORD_T>::except_illegal_instr; \
    op.trap.tval = op.instr;

template <typename WORD_T>
void user_core<WORD_T>::execute(exec_result_t &op) {
    constexpr bool is_rv64 = sizeof(WORD_T) * CHAR_BIT == 64;
    assert(op.type == exec_result_type_t::decode);

    auto [imm, dispatch, rs1, rs2, rd] = op.decode;
    op.type = exec_result_type_t::retire;
    op.next_pc = (op.instr&3)==3 ? op.pc+4 : op.pc+2;
    op.retire.rd = rd;

    switch (dispatch) {
        // Arithmetic & Logical
        case dispatch_t::add: {
            op.retire.value = gpr[rs1] + gpr[rs2];
            break;
        }
        case dispatch_t::sub: {
            op.retire.value = gpr[rs1] - gpr[rs2];
            break;
        }
        case dispatch_t::sll: {
            if constexpr (is_rv64) {
                op.retire.value = gpr[rs1] << (gpr[rs2] & 0x3F);
            } else {
                op.retire.value = gpr[rs1] << (gpr[rs2] & 0x1F);
            }
            break;
        }
        case dispatch_t::slt: {
            op.retire.value = (std::make_signed_t<WORD_T>(gpr[rs1]) < std::make_signed_t<WORD_T>(gpr[rs2])) ? 1 : 0;
            break;
        }
        case dispatch_t::sltu: {
            op.retire.value = (gpr[rs1] < gpr[rs2]) ? 1 : 0;
            break;
        }
        case dispatch_t::xor_: {
            op.retire.value = gpr[rs1] ^ gpr[rs2];
            break;
        }
        case dispatch_t::srl: {
            if constexpr (is_rv64) {
                op.retire.value = gpr[rs1] >> (gpr[rs2] & 0x3F);
            } else {
                op.retire.value = gpr[rs1] >> (gpr[rs2] & 0x1F);
            }
            break;
        }
        case dispatch_t::sra: {
            if constexpr (is_rv64) {
                op.retire.value = std::make_signed_t<WORD_T>(gpr[rs1]) >> (gpr[rs2] & 0x3F);
            } else {
                op.retire.value = std::make_signed_t<WORD_T>(gpr[rs1]) >> (gpr[rs2] & 0x1F);
            }
            break;
        }
        case dispatch_t::or_: {
            op.retire.value = gpr[rs1] | gpr[rs2];
            break;
        }
        case dispatch_t::and_: {
            op.retire.value = gpr[rs1] & gpr[rs2];
            break;
        }

        // Immediate Operations
        case dispatch_t::addi: {
            op.retire.value = gpr[rs1] + imm;
            break;
        }
        case dispatch_t::slti: {
            op.retire.value = (std::make_signed_t<WORD_T>(gpr[rs1]) < std::make_signed_t<WORD_T>(imm)) ? 1 : 0;
            break;
        }
        case dispatch_t::sltiu: {
            op.retire.value = (gpr[rs1] < static_cast<std::make_unsigned_t<WORD_T>>(imm)) ? 1 : 0;
            break;
        }
        case dispatch_t::xori: {
            op.retire.value = gpr[rs1] ^ imm;
            break;
        }
        case dispatch_t::ori: {
            op.retire.value = gpr[rs1] | imm;
            break;
        }
        case dispatch_t::andi: {
            op.retire.value = gpr[rs1] & imm;
            break;
        }
        case dispatch_t::slli: {
            if constexpr (is_rv64) {
                op.retire.value = gpr[rs1] << (imm & 0x3F);
            } else {
                op.retire.value = gpr[rs1] << (imm & 0x1F);
            }
            break;
        }
        case dispatch_t::srli: {
            if constexpr (is_rv64) {
                op.retire.value = gpr[rs1] >> (imm & 0x3F);
            } else {
                op.retire.value = gpr[rs1] >> (imm & 0x1F);
            }
            break;
        }
        case dispatch_t::srai: {
            if constexpr (is_rv64) {
                op.retire.value = std::make_signed_t<WORD_T>(gpr[rs1]) >> (imm & 0x3F);
            } else {
                op.retire.value = std::make_signed_t<WORD_T>(gpr[rs1]) >> (imm & 0x1F);
            }
            break;
        }

        // Memory Operations
        case dispatch_t::lb: {
            op.type = exec_result_type_t::load;
            op.load = {.addr=gpr[rs1]+imm, .width=libvio::width_t::byte, .sign_extend=true, .rd=rd};
            break;
        }
        case dispatch_t::lh: {
            op.type = exec_result_type_t::load;
            op.load = {.addr=gpr[rs1]+imm, .width=libvio::width_t::half, .sign_extend=true, .rd=rd};
            break;
        }
        case dispatch_t::lw: {
            op.type = exec_result_type_t::load;
            op.load = {.addr=gpr[rs1]+imm, .width=libvio::width_t::word, .sign_extend=true, .rd=rd};
            break;
        }
        case dispatch_t::lbu: {
            op.type = exec_result_type_t::load;
            op.load = {.addr=gpr[rs1]+imm, .width=libvio::width_t::byte, .sign_extend=false, .rd=rd};
            break;
        }
        case dispatch_t::lhu: {
            op.type = exec_result_type_t::load;
            op.load = {.addr=gpr[rs1]+imm, .width=libvio::width_t::half, .sign_extend=false, .rd=rd};
            break;
        }
        case dispatch_t::sb: {
            op.type = exec_result_type_t::store;
            op.store = {.addr=gpr[rs1]+imm, .width=libvio::width_t::byte, .data=gpr[rs2]};
            break;
        }
        case dispatch_t::sh: {
            op.type = exec_result_type_t::store;
            op.store = {.addr=gpr[rs1]+imm, .width=libvio::width_t::half, .data=gpr[rs2]};
            break;
        }
        case dispatch_t::sw: {
            op.type = exec_result_type_t::store;
            op.store = {.addr=gpr[rs1]+imm, .width=libvio::width_t::word, .data=gpr[rs2]};
            break;
        }

        // Control Flow
        case dispatch_t::jal: {
            op.retire.value = op.next_pc;
            op.next_pc = op.pc + imm;
            break;
        }
        case dispatch_t::jalr: {
            op.retire.value = op.next_pc;
            op.next_pc = (gpr[rs1] + imm) & ~static_cast<WORD_T>(1);
            break;
        }
        case dispatch_t::beq: {
            op.retire.value = 0;
            op.next_pc = (gpr[rs1] == gpr[rs2]) ? op.pc + imm : op.next_pc;
            break;
        }
        case dispatch_t::bne: {
            op.retire.value = 0;
            op.next_pc = (gpr[rs1] != gpr[rs2]) ? op.pc + imm : op.next_pc;
            break;
        }
        case dispatch_t::blt: {
            op.retire.value = 0;
            bool taken = std::make_signed_t<WORD_T>(gpr[rs1]) < std::make_signed_t<WORD_T>(gpr[rs2]);
            op.next_pc = taken ? op.pc + imm : op.next_pc;
            break;
        }
        case dispatch_t::bge: {
            op.retire.value = 0;
            bool taken = std::make_signed_t<WORD_T>(gpr[rs1]) >= std::make_signed_t<WORD_T>(gpr[rs2]);
            op.next_pc = taken ? op.pc + imm : op.next_pc;
            break;
        }
        case dispatch_t::bltu: {
            op.retire.value = 0;
            bool taken = gpr[rs1] < gpr[rs2];
            op.next_pc = taken ? op.pc + imm : op.next_pc;
            break;
        }
        case dispatch_t::bgeu: {
            op.retire.value = 0;
            bool taken = gpr[rs1] >= gpr[rs2];
            op.next_pc = taken ? op.pc + imm : op.next_pc;
            break;
        }

        // Upper Immediate
        case dispatch_t::lui: {
            op.retire.value = imm;
            break;
        }
        case dispatch_t::auipc: {
            op.retire.value = op.pc + imm;
            break;
        }

        // Multiply/Divide
        case dispatch_t::mul: {
            op.retire.value = gpr[rs1] * gpr[rs2];
            break;
        }
        case dispatch_t::mulh: {
            if constexpr (is_rv64) {
                uint64_t a = gpr[rs1];
                uint64_t b = gpr[rs2];
                __int128 result = static_cast<__int128>(static_cast<int64_t>(a)) * static_cast<__int128>(static_cast<int64_t>(b));
                op.retire.value = static_cast<WORD_T>(result >> 64);
            } else {
                int64_t result = static_cast<int64_t>(static_cast<int32_t>(gpr[rs1])) * static_cast<int64_t>(static_cast<int32_t>(gpr[rs2]));
                op.retire.value = static_cast<WORD_T>(result >> 32);
            }
            break;
        }
        case dispatch_t::mulhsu: {
            if constexpr (is_rv64) {
                uint64_t a = gpr[rs1];
                uint64_t b = gpr[rs2];
                __int128 result = static_cast<__int128>(static_cast<int64_t>(a)) * static_cast<__int128>(b);
                op.retire.value = static_cast<WORD_T>(result >> 64);
            } else {
                int64_t result = static_cast<int64_t>(static_cast<int32_t>(gpr[rs1])) * static_cast<int64_t>(gpr[rs2]);
                op.retire.value = static_cast<WORD_T>(result >> 32);
            }
            break;
        }
        case dispatch_t::mulhu: {
            if constexpr (is_rv64) {
                uint64_t a = gpr[rs1];
                uint64_t b = gpr[rs2];
                __uint128_t result = static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b);
                op.retire.value = static_cast<WORD_T>(result >> 64);
            } else {
                uint64_t result = static_cast<uint64_t>(gpr[rs1]) * static_cast<uint64_t>(gpr[rs2]);
                op.retire.value = static_cast<WORD_T>(result >> 32);
            }
            break;
        }
        case dispatch_t::div: {
            std::make_signed_t<WORD_T> a = gpr[rs1];
            std::make_signed_t<WORD_T> b = gpr[rs2];
            if (b == 0) {
                op.retire.value = static_cast<WORD_T>(-1);
            } else if (a == std::numeric_limits<std::make_signed_t<WORD_T>>::min() && b == -1) {
                op.retire.value = a;
            } else {
                op.retire.value = static_cast<WORD_T>(a / b);
            }
            break;
        }
        case dispatch_t::divu: {
            std::make_unsigned_t<WORD_T> a = gpr[rs1];
            std::make_unsigned_t<WORD_T> b = gpr[rs2];
            if (b == 0) {
                op.retire.value = static_cast<WORD_T>(-1);
            } else {
                op.retire.value = static_cast<WORD_T>(a / b);
            }
            break;
        }
        case dispatch_t::rem: {
            std::make_signed_t<WORD_T> a = gpr[rs1];
            std::make_signed_t<WORD_T> b = gpr[rs2];
            if (b == 0) {
                op.retire.value = a;
            } else if (a == std::numeric_limits<std::make_signed_t<WORD_T>>::min() && b == -1) {
                op.retire.value = 0;
            } else {
                op.retire.value = static_cast<WORD_T>(a % b);
            }
            break;
        }
        case dispatch_t::remu: {
            std::make_unsigned_t<WORD_T> a = gpr[rs1];
            std::make_unsigned_t<WORD_T> b = gpr[rs2];
            if (b == 0) {
                op.retire.value = a;
            } else {
                op.retire.value = static_cast<WORD_T>(a % b);
            }
            break;
        }

        // System
        case dispatch_t::ecall: {
            op.type = exec_result_type_t::sys_op;
            op.sys_op = {.ecall=true, .mret=false, .sret=false};
            break;
        }
        case dispatch_t::ebreak: {
            op.type = exec_result_type_t::trap;
            op.trap.cause = riscv::mcause<WORD_T>::except_breakpoint;
            op.trap.tval = op.pc;
            break;
        };
        case dispatch_t::mret: {
            op.type = exec_result_type_t::sys_op;
            op.sys_op = {.ecall=false, .mret=true, .sret=false};
            break;
        }
        case dispatch_t::sret: {
            op.type = exec_result_type_t::sys_op;
            op.sys_op = {.ecall=false, .mret=false, .sret=true};
            break;
        }
        case dispatch_t::csrrw: {
            op.type = exec_result_type_t::csr_op;
            op.csr_op.addr = imm & 0xfff;
            op.csr_op.rd = rd;
            op.csr_op.read = rd != 0;
            op.csr_op.write = true;
            op.csr_op.set = false;
            op.csr_op.clear = false;
            op.csr_op.value = gpr[rs1];
            break;
        }
        case dispatch_t::csrrs: {
            op.type = exec_result_type_t::csr_op;
            op.csr_op.addr = imm & 0xfff;
            op.csr_op.rd = rd;
            op.csr_op.read = true;
            op.csr_op.write = false;
            op.csr_op.set = rs1!=0;
            op.csr_op.clear = false;
            op.csr_op.value = gpr[rs1];
            break;
        }
        case dispatch_t::csrrc: {
            op.type = exec_result_type_t::csr_op;
            op.csr_op.addr = imm & 0xfff;
            op.csr_op.rd = rd;
            op.csr_op.read = true;
            op.csr_op.write = false;
            op.csr_op.set = false;
            op.csr_op.clear = rs1!=0;
            op.csr_op.value = gpr[rs1];
            break;
        }
        case dispatch_t::csrrwi: {
            op.type = exec_result_type_t::csr_op;
            op.csr_op.addr = imm & 0xfff;
            op.csr_op.rd = rd;
            op.csr_op.read = rd!=0;
            op.csr_op.write = true;
            op.csr_op.set = false;
            op.csr_op.clear = false;
            op.csr_op.value = rs1;
            break;
        }
        case dispatch_t::csrrsi: {
            op.type = exec_result_type_t::csr_op;
            op.csr_op.addr = imm & 0xfff;
            op.csr_op.rd = rd;
            op.csr_op.read = true;
            op.csr_op.write = false;
            op.csr_op.set = rs1!=0;
            op.csr_op.clear = false;
            op.csr_op.value = rs1;
            break;
        }
        case dispatch_t::csrrci: {
            op.type = exec_result_type_t::csr_op;
            op.csr_op.addr = imm & 0xfff;
            op.csr_op.rd = rd;
            op.csr_op.read = true;
            op.csr_op.write = false;
            op.csr_op.set = false;
            op.csr_op.clear = rs1!=0;
            op.csr_op.value = rs1;
            break;
        }

        // RV64 specific instructions
        case dispatch_t::lwu: {
            if constexpr (is_rv64) {
                op.type = exec_result_type_t::load;
                op.load = {.addr=gpr[rs1]+imm, .width=libvio::width_t::word, .sign_extend=false, .rd=rd};
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::ld: {
            if constexpr (is_rv64) {
                op.type = exec_result_type_t::load;
                op.load = {.addr=gpr[rs1]+imm, .width=libvio::width_t::dword, .sign_extend=true, .rd=rd};
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sd: {
            if constexpr (is_rv64) {
                op.type = exec_result_type_t::store;
                op.store = {.addr=gpr[rs1]+imm, .width=libvio::width_t::dword, .data=gpr[rs2]};
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::addiw: {
            if constexpr (is_rv64) {
                int32_t a = static_cast<int32_t>(gpr[rs1]);
                int32_t b = static_cast<int32_t>(imm);
                int32_t res32 = a + b;
                op.retire.value = static_cast<WORD_T>(static_cast<int64_t>(res32));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::slliw: {
            if constexpr (is_rv64) {
                uint8_t shamt = imm & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[rs1]);
                val <<= shamt;
                op.retire.value = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::srliw: {
            if constexpr (is_rv64) {
                uint8_t shamt = imm & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[rs1]);
                val >>= shamt;
                op.retire.value = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sraiw: {
            if constexpr (is_rv64) {
                uint8_t shamt = imm & 0x1F;
                int32_t val = static_cast<int32_t>(gpr[rs1]);
                val >>= shamt;
                op.retire.value = static_cast<WORD_T>(val);
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::addw: {
            if constexpr (is_rv64) {
                int32_t a = static_cast<int32_t>(gpr[rs1]);
                int32_t b = static_cast<int32_t>(gpr[rs2]);
                int32_t res32 = a + b;
                op.retire.value = static_cast<WORD_T>(static_cast<int64_t>(res32));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::subw: {
            if constexpr (is_rv64) {
                int32_t a = static_cast<int32_t>(gpr[rs1]);
                int32_t b = static_cast<int32_t>(gpr[rs2]);
                int32_t res32 = a - b;
                op.retire.value = static_cast<WORD_T>(static_cast<int64_t>(res32));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sllw: {
            if constexpr (is_rv64) {
                uint8_t shamt = gpr[rs2] & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[rs1]);
                val <<= shamt;
                op.retire.value = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::srlw: {
            if constexpr (is_rv64) {
                uint8_t shamt = gpr[rs2] & 0x1F;
                uint32_t val = static_cast<uint32_t>(gpr[rs1]);
                val >>= shamt;
                op.retire.value = static_cast<WORD_T>(static_cast<int32_t>(val));
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::sraw: {
            if constexpr (is_rv64) {
                uint8_t shamt = gpr[rs2] & 0x1F;
                int32_t val = static_cast<int32_t>(gpr[rs1]);
                val >>= shamt;
                op.retire.value = static_cast<WORD_T>(val);
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::mulw: {
            if constexpr (is_rv64) {
                constexpr uint64_t mask = 0xffffffff;
                op.retire.value = static_cast<uint32_t>(gpr[rs1]&mask) * static_cast<uint32_t>(gpr[rs2]&mask);
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::divw: {
            if constexpr (is_rv64) {
                constexpr uint64_t mask = 0xffffffff;
                int32_t a = gpr[rs1] & mask;
                int32_t b = gpr[rs2] & mask;
                if (b == 0) {
                    op.retire.value = -1;
                } else if (a == std::numeric_limits<int32_t>::min() && b == -1) {
                    op.retire.value = a;
                } else {
                    op.retire.value = int64_t(int32_t(a/b));
                }
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::divuw: {
            if constexpr (is_rv64) {
                constexpr uint64_t mask = 0xffffffff;
                uint32_t a = gpr[rs1] & mask;
                uint32_t b = gpr[rs2] & mask;
                if (b == 0) {
                    op.retire.value = -1;
                } else {
                    op.retire.value = int64_t(int32_t(a/b));
                }
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::remw: {
            if constexpr (is_rv64) {
                constexpr uint64_t mask = 0xffffffff;
                int32_t a = gpr[rs1] & mask;
                int32_t b = gpr[rs2] & mask;
                if (b == 0) {
                    op.retire.value = a;
                } else if (a == std::numeric_limits<int32_t>::min() && b == -1) {
                    op.retire.value = 0;
                } else {
                    op.retire.value = int64_t(int32_t(a%b));
                }
            } else {
                invalid_instruction();
            }
            break;
        }
        case dispatch_t::remuw: {
            if constexpr (is_rv64) {
                constexpr uint64_t mask = 0xffffffff;
                uint32_t a = gpr[rs1] & mask;
                uint32_t b = gpr[rs2] & mask;
                if (b == 0) {
                    op.retire.value = a;
                } else {
                    op.retire.value = int64_t(int32_t(a%b));
                }
            } else {
                invalid_instruction();
            }
            break;
        }

        default: {
            invalid_instruction();
            break;
        }
    }
}

#undef invalid_instruction

}

#endif
