#include <cstdint>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_nemu.hh>
#include <optional>

// In this file, memory access instructions are implemented.

using namespace libcpu;
using namespace libcpu::rv32i;

#define ram_end (ram_base+cpu->memory.size())
#define log_load if (cpu->trace_on) { cpu->event_buffer.push_back({.type=event_type_t::load, .val1=addr, .val2=data}); }
#define log_store if (cpu->trace_on) { cpu->event_buffer.push_back({.type=event_type_t::store, .val1=addr, .val2=data}); }

void rv32i_cpu_nemu::lb(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint32_t data = 0;
    if (addr>=ram_base && addr<ram_end) {
        data = (int32_t)(int8_t)cpu->memory[addr-ram_base];
        cpu->gpr[decode.rd] = data;
    } else {
        auto res = cpu->mmio_bus->mmio_read(addr, width_t::byte);
        if (res.has_value()) {
            data = (int32_t)(int8_t)(*res);
            cpu->gpr[decode.rd] = data;
        } else {
            cpu->raise_exception(EXCEPTION_LOAD_ACCESS_FAULT, addr);
            return;
        }
    }
    log_load;
}

void rv32i_cpu_nemu::lh(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint32_t data = 0;
    if (addr%2 != 0) {
        cpu->raise_exception(EXCEPTION_LOAD_ADDRESS_MISALIGNED, addr);
        return;
    } else if (addr>=ram_base && addr+1<ram_end) {
        data = (int32_t)(int16_t)(uint16_t(cpu->memory[addr-ram_base+1])<<8 | uint16_t(cpu->memory[addr-ram_base]));
        cpu->gpr[decode.rd] = data;
    } else {
        auto res = cpu->mmio_bus->mmio_read(addr, width_t::half);
        if (res.has_value()) {
            data = (int32_t)(int16_t)(*res);
            cpu->gpr[decode.rd] = data;
        } else {
            cpu->raise_exception(EXCEPTION_LOAD_ACCESS_FAULT, addr);
            return;
        }
    }
    log_load;
}

void rv32i_cpu_nemu::lw(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint32_t data = 0;
    if (addr%4 != 0) {
        cpu->raise_exception(EXCEPTION_LOAD_ADDRESS_MISALIGNED, addr);
        return;
    } else if (addr>=ram_base && addr+3<ram_end) {
        data = (int32_t)(uint32_t(cpu->memory[addr-ram_base+3])<<24 | uint32_t(cpu->memory[addr-ram_base+2])<<16 | uint32_t(cpu->memory[addr-ram_base+1])<<8 | uint32_t(cpu->memory[addr-ram_base]));
        cpu->gpr[decode.rd] = data;
    } else {
        auto res = cpu->mmio_bus->mmio_read(addr, width_t::word);
        if (res.has_value()) {
            data = (int32_t)(*res);
            cpu->gpr[decode.rd] = data;
        } else {
            cpu->raise_exception(EXCEPTION_LOAD_ACCESS_FAULT, addr);
            return;
        }
    }
    log_load;
}

void rv32i_cpu_nemu::lbu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint32_t data = 0;
    if (addr >= ram_base && addr < ram_end) {
        data = cpu->memory[addr-ram_base];
        cpu->gpr[decode.rd] = data;
    } else {
        auto res = cpu->mmio_bus->mmio_read(addr, width_t::byte);
        if (res.has_value()) {
            data = (uint8_t)(*res);
            cpu->gpr[decode.rd] = data;
        } else {
            cpu->raise_exception(EXCEPTION_LOAD_ACCESS_FAULT, addr);
            return;
        }
    }
    log_load;
}

void rv32i_cpu_nemu::lhu(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint32_t data = 0;
    if (addr%2 != 0) {
        cpu->raise_exception(EXCEPTION_LOAD_ADDRESS_MISALIGNED, addr);
        return;
    } else if (addr >= ram_base && addr + 1 < ram_end) {
        data = uint16_t(cpu->memory[addr-ram_base+1])<<8 | uint16_t(cpu->memory[addr-ram_base]);
        cpu->gpr[decode.rd] = data;
    } else {
        auto res = cpu->mmio_bus->mmio_read(addr, width_t::half);
        if (res.has_value()) {
            data = (uint16_t)(*res);
            cpu->gpr[decode.rd] = data;
        } else {
            cpu->raise_exception(EXCEPTION_LOAD_ACCESS_FAULT, addr);
            return;
        }
    }
    log_load;
}

void rv32i_cpu_nemu::sb(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint8_t data = cpu->gpr[decode.rs2] & 0xff;
    if (addr>=ram_base && addr<ram_end) {
        cpu->memory[addr-ram_base] = data;
    } else {
        bool res = cpu->mmio_bus->mmio_write(addr, width_t::byte, data);
        if (!res) {
            cpu->raise_exception(EXCEPTION_STORE_AMO_ACCESS_FAULT, addr);
            return;
        }
    }
    log_store;
}

void rv32i_cpu_nemu::sh(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint16_t data = cpu->gpr[decode.rs2] & 0xffff;
    if (addr%2 != 0) {
        cpu->raise_exception(EXCEPTION_STORE_AMO_ADDRESS_MISALIGNED, addr);
        return;
    } else if (addr>=ram_base && addr+1<ram_end) {
        cpu->memory[addr-ram_base] = data & 0xff;
        cpu->memory[addr-ram_base+1] = data >> 8;
    } else {
        bool res = cpu->mmio_bus->mmio_write(addr, width_t::half, data);
        if (!res) {
            cpu->raise_exception(EXCEPTION_STORE_AMO_ACCESS_FAULT, addr);
            return;
        }
    }
    log_store;
}

void rv32i_cpu_nemu::sw(rv32i_cpu_nemu* cpu, const decode_t& decode) {
    uint32_t addr = cpu->gpr[decode.rs1] + decode.imm;
    uint32_t data = cpu->gpr[decode.rs2];
    if (addr%4 != 0) {
        cpu->raise_exception(EXCEPTION_STORE_AMO_ADDRESS_MISALIGNED, addr);
        return;
    } else if (addr>=ram_base && addr+3<ram_end) {
        cpu->memory[addr-ram_base] = data & 0xff;
        cpu->memory[addr-ram_base+1] = (data>>8) & 0xff;
        cpu->memory[addr-ram_base+2] = (data>>16) & 0xff;
        cpu->memory[addr-ram_base+3] = (data>>24) & 0xff;
    } else {
        bool res = cpu->mmio_bus->mmio_write(addr, width_t::word, data);
        if (!res) {
            cpu->raise_exception(EXCEPTION_STORE_AMO_ACCESS_FAULT, addr);
            return;
        }
    }
    log_store;
}
