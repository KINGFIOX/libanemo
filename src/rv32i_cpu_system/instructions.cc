#include <libcpu/rv32i_cpu_system.hh>
#include <libcpu/rv32i.hh>

// In this file, arithmetic and control flow instructions are implemented.

using namespace libcpu;
using namespace libcpu::rv32i;

// ==================== Arithmetic & Logic ====================
void rv32i_cpu_system::add(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] + cpu->gpr[decode.rs2];
}

void rv32i_cpu_system::sub(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] - cpu->gpr[decode.rs2];
}

void rv32i_cpu_system::sll(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] << (cpu->gpr[decode.rs2] & 0x1f);
}

void rv32i_cpu_system::slt(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = ((int32_t)cpu->gpr[decode.rs1] < (int32_t)cpu->gpr[decode.rs2]) ? 1 : 0;
}

void rv32i_cpu_system::sltu(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (cpu->gpr[decode.rs1] < cpu->gpr[decode.rs2]) ? 1 : 0;
}

void rv32i_cpu_system::xor_(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] ^ cpu->gpr[decode.rs2];
}

void rv32i_cpu_system::srl(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (cpu->gpr[decode.rs2] & 0x1f);
}

void rv32i_cpu_system::sra(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (int32_t)cpu->gpr[decode.rs1] >> (cpu->gpr[decode.rs2] & 0x1f);
}

void rv32i_cpu_system::or_(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] | cpu->gpr[decode.rs2];
}

void rv32i_cpu_system::and_(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] & cpu->gpr[decode.rs2];
}

// ==================== Immediate Operations ====================
void rv32i_cpu_system::addi(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] + decode.imm;
}

void rv32i_cpu_system::slti(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = ((int32_t)cpu->gpr[decode.rs1] < (int32_t)decode.imm) ? 1 : 0;
}

void rv32i_cpu_system::sltiu(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (cpu->gpr[decode.rs1] < decode.imm) ? 1 : 0;
}

void rv32i_cpu_system::xori(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] ^ decode.imm;
}

void rv32i_cpu_system::ori(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] | decode.imm;
}

void rv32i_cpu_system::andi(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] & decode.imm;
}

void rv32i_cpu_system::slli(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] << (decode.imm & 0x1f);
}

void rv32i_cpu_system::srli(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] >> (decode.imm & 0x1f);
}

void rv32i_cpu_system::srai(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = (int32_t)cpu->gpr[decode.rs1] >> (decode.imm & 0x1f);
}

// ==================== Control Flow ====================
void rv32i_cpu_system::jal(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->pc + 4;
    cpu->next_pc = cpu->pc + decode.imm;
}

void rv32i_cpu_system::jalr(rv32i_cpu_system* cpu, const decode_t& decode) {
    uint32_t target = (cpu->gpr[decode.rs1] + decode.imm) & ~1;
    cpu->gpr[decode.rd] = cpu->pc + 4;
    cpu->next_pc = target;
}

void rv32i_cpu_system::beq(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->next_pc = (cpu->gpr[decode.rs1] == cpu->gpr[decode.rs2])
        ? cpu->pc + decode.imm
        : cpu->pc + 4;
}

void rv32i_cpu_system::bne(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->next_pc = (cpu->gpr[decode.rs1] != cpu->gpr[decode.rs2])
        ? cpu->pc + decode.imm
        : cpu->pc + 4;
}

void rv32i_cpu_system::blt(rv32i_cpu_system* cpu, const decode_t& decode) {
    bool taken = (int32_t)cpu->gpr[decode.rs1] < (int32_t)cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

void rv32i_cpu_system::bge(rv32i_cpu_system* cpu, const decode_t& decode) {
    bool taken = (int32_t)cpu->gpr[decode.rs1] >= (int32_t)cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

void rv32i_cpu_system::bltu(rv32i_cpu_system* cpu, const decode_t& decode) {
    bool taken = cpu->gpr[decode.rs1] < cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

void rv32i_cpu_system::bgeu(rv32i_cpu_system* cpu, const decode_t& decode) {
    bool taken = cpu->gpr[decode.rs1] >= cpu->gpr[decode.rs2];
    cpu->next_pc = taken ? cpu->pc + decode.imm : cpu->pc + 4;
}

// ==================== Upper Immediate ====================
void rv32i_cpu_system::lui(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = decode.imm;
}

void rv32i_cpu_system::auipc(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->pc + decode.imm;
}

// ==================== Multiply/Divide ====================
void rv32i_cpu_system::mul(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->gpr[decode.rd] = cpu->gpr[decode.rs1] * cpu->gpr[decode.rs2];
}

void rv32i_cpu_system::mulh(rv32i_cpu_system* cpu, const decode_t& decode) {
    int64_t result = (int64_t)(int32_t)cpu->gpr[decode.rs1] * (int64_t)(int32_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
}

void rv32i_cpu_system::mulhsu(rv32i_cpu_system* cpu, const decode_t& decode) {
    int64_t result = (int64_t)(int32_t)cpu->gpr[decode.rs1] * (uint64_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
}

void rv32i_cpu_system::mulhu(rv32i_cpu_system* cpu, const decode_t& decode) {
    uint64_t result = (uint64_t)cpu->gpr[decode.rs1] * (uint64_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (result >> 32) & 0xffffffff;
}

void rv32i_cpu_system::div(rv32i_cpu_system* cpu, const decode_t& decode) {
    int32_t a = (int32_t)cpu->gpr[decode.rs1];
    int32_t b = (int32_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? -1 : (a / b);
}

void rv32i_cpu_system::divu(rv32i_cpu_system* cpu, const decode_t& decode) {
    uint32_t a = cpu->gpr[decode.rs1];
    uint32_t b = cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? 0xffffffff : (a / b);
}

void rv32i_cpu_system::rem(rv32i_cpu_system* cpu, const decode_t& decode) {
    int32_t a = (int32_t)cpu->gpr[decode.rs1];
    int32_t b = (int32_t)cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? a : (a % b);
}

void rv32i_cpu_system::remu(rv32i_cpu_system* cpu, const decode_t& decode) {
    uint32_t a = cpu->gpr[decode.rs1];
    uint32_t b = cpu->gpr[decode.rs2];
    cpu->gpr[decode.rd] = (b == 0) ? a : (a % b);
}
