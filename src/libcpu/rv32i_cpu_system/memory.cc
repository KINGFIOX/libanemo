#include <cstdint>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_system.hh>
#include <libvio/frontend.hh>
#include <libvio/width.hh>
#include <optional>

// In this file, memory access instructions are implemented.

using namespace libvio;
using namespace libcpu;
using namespace libcpu::rv32i;

void rv32i_cpu_system::load(const decode_t &decode, width_t width, bool sign_extend) {
    uint32_t addr = gpr[decode.rs1] + decode.imm;
    auto data_opt = data_bus->read(addr, width);
    // fall back to MMIO if the address is out of RAM
    if (!data_opt.has_value() && mmio_bus!=nullptr) {
        data_opt = mmio_bus->read(addr, width);
    }
    if (data_opt.has_value()) {
        // `data` is zero extended
        word_t data = data_opt.value();
        // log the memory operation with *zero extended* data
        if (event_buffer != nullptr) {
            event_buffer->push_back({.type=event_type_t::load, .pc=pc, .val1=addr, .val2=data_opt.value()});
        }
        // sign extend the data
        if (sign_extend) {
            switch (width) {
                case width_t::byte:
                    data = uint32_t(int32_t(int8_t(data)));
                    break;
                case width_t::half:
                    data = uint32_t(int32_t(int16_t(data)));
                    break;
                default:
                    break;
            }
        }
        gpr[decode.rd] = data;
    } else {
        // both RAM and MMIO failed
        raise_exception(EXCEPTION_LOAD_ACCESS_FAULT, addr);
    }
}

void rv32i_cpu_system::store(const decode_t &decode, width_t width) {
    word_t addr = gpr[decode.rs1] + decode.imm;
    word_t data = zero_truncate<uint32_t>(gpr[decode.rs2], width);
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
        raise_exception(EXCEPTION_STORE_AMO_ACCESS_FAULT, addr);
    }
}

void rv32i_cpu_system::lb(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->load(decode, width_t::byte, true);
}

void rv32i_cpu_system::lh(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->load(decode, width_t::half, true);
}

void rv32i_cpu_system::lw(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->load(decode, width_t::word, false);
}

void rv32i_cpu_system::lbu(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->load(decode, width_t::byte, false);
}

void rv32i_cpu_system::lhu(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->load(decode, width_t::half, false);
}

void rv32i_cpu_system::sb(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->store(decode, width_t::byte);
}

void rv32i_cpu_system::sh(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->store(decode, width_t::half);
}

void rv32i_cpu_system::sw(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->store(decode, width_t::word);
}
