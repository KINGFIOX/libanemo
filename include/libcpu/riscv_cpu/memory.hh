#ifndef LIBCPU_RISCV_CPU_MEMORY_HH
#define LIBCPU_RISCV_CPU_MEMORY_HH

#include <climits>
#include <libcpu/riscv.hh>
#include <libvio/width.hh>
#include <libcpu/riscv_cpu/riscv_cpu.hh>

namespace libcpu {

template <typename WORD_T>
void riscv_cpu<WORD_T>::load(const decode_t &decode, libvio::width_t width, bool sign_extend) {
    WORD_T addr = gpr[decode.rs1] + decode.imm;
    std::optional<uint64_t> data_opt = this->data_bus->read(addr, width);
    // fall back to MMIO if the address is out of RAM
    if (!data_opt.has_value() && this->mmio_bus!=nullptr) {
        data_opt = this->mmio_bus->read(addr, width);
    }
    if (data_opt.has_value()) {
        // `data` is zero extended
        WORD_T data = data_opt.value();
        // log the memory operation with *zero extended* data
        if (this->event_buffer != nullptr) {
            this->event_buffer->push_back({.type=event_type_t::load, .pc=pc, .val1=addr, .val2=data_opt.value()});
        }
        // sign extend the data
        if (sign_extend) {
            data = libvio::sign_extend<WORD_T>(data, width);
        }
        gpr[decode.rd] = data;
    } else {
        // both RAM and MMIO failed
        raise_exception(riscv::mcause<WORD_T>::except_load_fault, addr);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::store(const decode_t &decode, libvio::width_t width) {
    WORD_T addr = gpr[decode.rs1] + decode.imm;
    WORD_T data = libvio::zero_truncate<WORD_T>(gpr[decode.rs2], width);
    bool success = this->data_bus->write(addr, width, data);
    // fall back to MMIO
    if (!success && this->mmio_bus!=nullptr) {
        success = this->mmio_bus->write(addr, width, data);
    }
    if (success) {
        if (this->event_buffer!=nullptr) {
            this->event_buffer->push_back({.type=event_type_t::store, .pc=pc, .val1=addr, .val2=data});
        }
    } else {
        // both RAM and MMIO failed
        raise_exception(riscv::mcause<WORD_T>::except_store_fault, addr);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::lb(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->load(decode, libvio::width_t::byte, true);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::lh(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->load(decode, libvio::width_t::half, true);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::lw(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->load(decode, libvio::width_t::word, true);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::ld(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        cpu->raise_exception(riscv::mcause<WORD_T>::except_illegal_instr, decode.instr);
    } else {
        cpu->load(decode, libvio::width_t::dword, true);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::lbu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->load(decode, libvio::width_t::byte, false);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::lhu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->load(decode, libvio::width_t::half, false);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::lwu(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        cpu->raise_exception(riscv::mcause<WORD_T>::except_illegal_instr, decode.instr);
    } else {
        cpu->load(decode, libvio::width_t::word, false);
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sb(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->store(decode, libvio::width_t::byte);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sh(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->store(decode, libvio::width_t::half);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sw(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    cpu->store(decode, libvio::width_t::word);
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::sd(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    if constexpr (sizeof(WORD_T)*CHAR_BIT == 32) {
        cpu->raise_exception(riscv::mcause<WORD_T>::except_illegal_instr, decode.instr);
    } else {
        cpu->store(decode, libvio::width_t::dword);
    }
}

}

#endif
