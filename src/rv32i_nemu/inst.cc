#include <libcpu/rv32i_cpu_nemu.hh>
#include <libcpu/rv32i.hh>

// In this file, arithmetic and control flow instructions are implemented.

using namespace libcpu;
using namespace libcpu::rv32i;

// ==================== Arithmetic & Logic ====================
void rv32i_cpu_nemu::add(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] + cpu->gpr[decode.rs2];
}

void rv32i_cpu_nemu::sub(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] - cpu->gpr[decode.rs2];
}

void rv32i_cpu_nemu::sll(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] << (cpu->gpr[decode.rs2] & 0x1f);
}

void rv32i_cpu_nemu::slt(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = ((int32_t)cpu->gpr[decode.rs1] < (int32_t)cpu->gpr[decode.rs2]) ? 1 : 0;
}

void rv32i_cpu_nemu::sltu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (cpu->gpr[decode.rs1] < cpu->gpr[decode.rs2]) ? 1 : 0;
}

void rv32i_cpu_nemu::xor_(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] ^ cpu->gpr[decode.rs2];
}

void rv32i_cpu_nemu::srl(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (cpu->gpr[decode.rs2] & 0x1f);
}

void rv32i_cpu_nemu::sra(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (int32_t)cpu->gpr[decode.rs1] >> (cpu->gpr[decode.rs2] & 0x1f);
}

void rv32i_cpu_nemu::or_(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] | cpu->gpr[decode.rs2];
}

void rv32i_cpu_nemu::and_(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] & cpu->gpr[decode.rs2];
}

// ==================== Immediate Operations ====================
void rv32i_cpu_nemu::addi(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] + decode.imm;
}

void rv32i_cpu_nemu::slti(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = ((int32_t)cpu->gpr[decode.rs1] < (int32_t)decode.imm) ? 1 : 0;
}

void rv32i_cpu_nemu::sltiu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (cpu->gpr[decode.rs1] < decode.imm) ? 1 : 0;
}

void rv32i_cpu_nemu::xori(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] ^ decode.imm;
}

void rv32i_cpu_nemu::ori(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] | decode.imm;
}

void rv32i_cpu_nemu::andi(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] & decode.imm;
}

void rv32i_cpu_nemu::slli(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] << (decode.imm & 0x1f);
}

void rv32i_cpu_nemu::srli(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (decode.imm & 0x1f);
}

void rv32i_cpu_nemu::srai(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (int32_t)cpu->gpr[decode.rs1] >> (decode.imm & 0x1f);
}

// ==================== Control Flow ====================
void rv32i_cpu_nemu::jal(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->pc + 4;
    cpu->next_pc = cpu->pc + decode.imm;
}

void rv32i_cpu_nemu::jalr(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t target = (cpu->gpr[decode.rs1] + decode.imm) & ~1;
    cpu->gpr[decode.rd] = cpu->pc + 4;
    cpu->next_pc = target;
}

void rv32i_cpu_nemu::beq(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->next_pc = (cpu->gpr[decode.rs1] == cpu->gpr[decode.rs2])
        ? cpu->pc + decode.imm
        : cpu->pc + 4;
}

void rv32i_cpu_nemu::bne(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->next_pc = (cpu->gpr[decode.rs1] != cpu->gpr[decode.rs2])
        ? cpu->pc + decode.imm
        : cpu->pc + 4;
}

void rv32i_cpu_nemu::blt(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    bool taken = (int32_t)cpu->gpr[decode.rs1] < (int32_t)cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

void rv32i_cpu_nemu::bge(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    bool taken = (int32_t)cpu->gpr[decode.rs1] >= (int32_t)cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

void rv32i_cpu_nemu::bltu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    bool taken = cpu->gpr[decode.rs1] < cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

void rv32i_cpu_nemu::bgeu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    bool taken = cpu->gpr[decode.rs1] >= cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

// ==================== Upper Immediate ====================
void rv32i_cpu_nemu::lui(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = decode.imm;
}

void rv32i_cpu_nemu::auipc(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->pc + decode.imm;
}

// ==================== Multiply/Divide ====================
void rv32i_cpu_nemu::mul(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] * cpu->gpr[decode.rs2];
}

void rv32i_cpu_nemu::mulh(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    int64_t result = (int64_t)(int32_t)cpu->gpr[decode.rs1] * (int64_t)(int32_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
}

void rv32i_cpu_nemu::mulhsu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    int64_t result = (int64_t)(int32_t)cpu->gpr[decode.rs1] * (uint64_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
}

void rv32i_cpu_nemu::mulhu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint64_t result = (uint64_t)cpu->gpr[decode.rs1] * (uint64_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
}

void rv32i_cpu_nemu::div(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    int32_t a = (int32_t)cpu->gpr[decode.rs1];
    int32_t b = (int32_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? -1 : (a / b);
}

void rv32i_cpu_nemu::divu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t a = cpu->gpr[decode.rs1];
    uint32_t b = cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? 0xffffffff : (a / b);
}

void rv32i_cpu_nemu::rem(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    int32_t a = (int32_t)cpu->gpr[decode.rs1];
    int32_t b = (int32_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? a : (a % b);
}

void rv32i_cpu_nemu::remu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t a = cpu->gpr[decode.rs1];
    uint32_t b = cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? a : (a % b);
}
