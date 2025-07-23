#ifndef LIBCPU_RISCV_CPU_INSTRUCTIONS_HH
#define LIBCPU_RISCV_CPU_INSTRUCTIONS_HH

#include <cstdint>
#include <libcpu/riscv.hh>
#include <libcpu/riscv_cpu/riscv_cpu.hh>
#include <type_traits>

namespace libcpu {

template <typename WORD_T>
void riscv_cpu<WORD_T>::add(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] + cpu->gpr[decode.rs2];
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sub(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] - cpu->gpr[decode.rs2];
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sll(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] << (cpu->gpr[decode.rs2] & 0x1f);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::slt(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]) < std::make_signed_t<WORD_T>(cpu->gpr[decode.rs2])) ? 1 : 0;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sltu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (cpu->gpr[decode.rs1] < cpu->gpr[decode.rs2]) ? 1 : 0;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::xor_(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] ^ cpu->gpr[decode.rs2];
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::srl(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (cpu->gpr[decode.rs2] & 0x1f);
    } else {
        cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (cpu->gpr[decode.rs2] & 0x3f);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sra(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        cpu->gpr[decode.rd] = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]) >> (cpu->gpr[decode.rs2] & 0x1f);
    } else {
        cpu->gpr[decode.rd] = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]) >> (cpu->gpr[decode.rs2] & 0x3f);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::or_(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] | cpu->gpr[decode.rs2];
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::and_(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] & cpu->gpr[decode.rs2];
}

// ==================== Immediate Operations ====================
template <typename WORD_T>
void riscv_cpu<WORD_T>::addi(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] + decode.imm;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::slti(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]) < std::make_signed_t<WORD_T>(decode.imm)) ? 1 : 0;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sltiu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (cpu->gpr[decode.rs1] < decode.imm) ? 1 : 0;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::xori(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] ^ decode.imm;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::ori(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] | decode.imm;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::andi(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] & decode.imm;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::slli(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] << (decode.imm & 0x1f);
    } else {
        cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] << (decode.imm & 0x3f);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::srli(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (decode.imm & 0x1f);
    } else {
        cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (decode.imm & 0x3f);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::srai(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]) >> (decode.imm & 0x1f);
}

// ==================== Control Flow ====================
template <typename WORD_T>
void riscv_cpu<WORD_T>::jal(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->pc + 4;
    cpu->next_pc = cpu->pc + decode.imm;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::jalr(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    uint32_t target = (cpu->gpr[decode.rs1] + decode.imm) & ~1;
    cpu->gpr[decode.rd] = cpu->pc + 4;
    cpu->next_pc = target;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::beq(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->next_pc = (cpu->gpr[decode.rs1] == cpu->gpr[decode.rs2])
        ? cpu->pc + decode.imm
        : cpu->pc + 4;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::bne(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->next_pc = (cpu->gpr[decode.rs1] != cpu->gpr[decode.rs2])
        ? cpu->pc + decode.imm
        : cpu->pc + 4;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::blt(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    bool taken = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]) < std::make_signed_t<WORD_T>(cpu->gpr[decode.rs2]);
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::bge(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    bool taken = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]) >= std::make_signed_t<WORD_T>(cpu->gpr[decode.rs2]);
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::bltu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    bool taken = cpu->gpr[decode.rs1] < cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::bgeu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    bool taken = cpu->gpr[decode.rs1] >= cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

// ==================== Upper Immediate ====================
template <typename WORD_T>
void riscv_cpu<WORD_T>::lui(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = decode.imm;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::auipc(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->pc + decode.imm;
}

// ==================== Multiply/Divide ====================
template <typename WORD_T>
void riscv_cpu<WORD_T>::mul(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] * cpu->gpr[decode.rs2];
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::mulh(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        int64_t result = int64_t(int32_t(cpu->gpr[decode.rs1])) * int64_t(int32_t(cpu->gpr[decode.rs2]));
        cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
    } else {

    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::mulhsu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        int64_t result = int64_t(int32_t(cpu->gpr[decode.rs1])) * uint64_t(cpu->gpr[decode.rs2]);
        cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
    } else {

    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::mulhu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        uint64_t result = uint64_t(cpu->gpr[decode.rs1]) * uint64_t(cpu->gpr[decode.rs2]);
        cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
    } else {
        
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::div(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    int32_t a = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]);
    int32_t b = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs2]);
    cpu->gpr[decode.rd] = (b == 0) ? -1 : (a / b);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::divu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    uint32_t a = cpu->gpr[decode.rs1];
    uint32_t b = cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? ~WORD_T(0) : (a / b);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::rem(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    int32_t a = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs1]);
    int32_t b = std::make_signed_t<WORD_T>(cpu->gpr[decode.rs2]);
    cpu->gpr[decode.rd] = (b == 0) ? a : (a % b);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::remu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    uint32_t a = cpu->gpr[decode.rs1];
    uint32_t b = cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? a : (a % b);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::ecall(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    switch (cpu->priv_level) {
        case riscv::priv_level_t::u:
            cpu->raise_exception(riscv::mcause<WORD_T>::except_env_call_u, 0);
            break;
        case riscv::priv_level_t::s:
            cpu->raise_exception(riscv::mcause<WORD_T>::except_env_call_s, 0);
            break;
        case riscv::priv_level_t::h:
        case riscv::priv_level_t::m:
            cpu->raise_exception(riscv::mcause<WORD_T>::except_env_call_m, 0);
            break;
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::ebreak(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->raise_exception(riscv::mcause<WORD_T>::except_breakpoint, 0);
}

}

#endif