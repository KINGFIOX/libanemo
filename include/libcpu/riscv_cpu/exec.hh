#ifndef LIBCPU_RISCV_CPU_EXEC_HH
#define LIBCPU_RISCV_CPU_EXEC_HH

#include <libcpu/riscv_cpu/riscv_cpu.hh>

namespace libcpu {

template <typename WORD_T>
void riscv_cpu<WORD_T>::next_cycle(void) {
    next_instruction();
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::next_instruction(void) {
    // fetch the instruction
    std::optional<uint32_t> instruction_opt;
    uint32_t instruction;
    size_t decode_cache_offset;
    decode_t decode;
    if (pc%4 != 0) {
        raise_exception(riscv::mcause<WORD_T>::except_instr_misalign, pc);
        goto next_instr;
    }
    instruction_opt = this->instr_bus->read(pc, libvio::width_t::word);
    if (!instruction_opt.has_value()) {
        raise_exception(riscv::mcause<WORD_T>::except_instr_fault, pc);
        goto next_instr;
    }

    // Check the decode cache
    instruction = instruction_opt.value();
    decode_cache_offset = (pc>>2) & decode_cache_addr_mask;
    // if there is a cache invalidation, update the cache
    decode = decode_cache[decode_cache_offset];
    if (decode.instr != instruction) {
        decode = decode_instruction(instruction);
        decode_cache[decode_cache_offset] = decode;
    }

    // log the instruction
    if (this->event_buffer!=nullptr) {
        this->event_buffer->push_back({.type=event_type_t::issue, .pc=pc, .val1=instruction, .val2=0});
    }

    // Check if the instruction is valid
    if (decode.op == nullptr) {
        this->raise_exception(riscv::mcause<WORD_T>::except_illegal_instr, instruction);
        goto next_instr;
    }

    // execute the instuction
    // decode.op() may change `next_pc`
    next_pc = pc + 4;
    decode.op(this, decode);
    gpr[0] = 0;

    // log register writing
    // memory events are logged by implementations of memory accessing instructions
    // traps are logged by handle_trap()
    if (this->event_buffer!=nullptr && !except_mcause.has_value() && decode.rd!=0) {
        this->event_buffer->push_back({.type=event_type_t::reg_write, .pc=pc, .val1=decode.rd, .val2=gpr[decode.rd]});
    }

    next_instr:
    // handle traps
    handle_trap();
    pc = next_pc;

    if (this->mmio_bus != nullptr) {
        this->mmio_bus->next_cycle();
    }
}

}

#endif
