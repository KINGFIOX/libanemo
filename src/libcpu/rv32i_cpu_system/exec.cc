#include <cstddef>
#include <cstdint>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_system.hh>

using namespace libcpu;
using namespace libcpu::rv32i;

void rv32i_cpu_system::next_cycle(void) {
    next_instruction();
}

void rv32i_cpu_system::next_instruction(void) {
    if (pc%4 != 0) {
        raise_exception(EXCEPTION_INSTRUCTION_ADDRESS_MISALIGNED, pc);
        return;
    }
    
    auto instruction_opt = instr_bus->read(pc, libvio::width_t::word);
    if (!instruction_opt.has_value()) {
        raise_exception(rv32i::EXCEPTION_INSTRUCTION_ACCESS_FAULT, pc);
        return;
    }

    uint32_t instruction = instruction_opt.value();
    size_t decode_cache_offset = (pc>>2) & decode_cache_addr_mask;
    decode_t decode;
    // if there is a cache invalidation, update the cache
    decode = decode_cache[decode_cache_offset];
    if (decode.instr != instruction) {
        decode = decode_instruction(instruction);
        decode_cache[decode_cache_offset] = decode;
    }
    // log the instruction
    if (event_buffer!=nullptr) {
        event_buffer->push_back({.type=event_type_t::issue, .pc=pc, .val1=instruction, .val2=0});
    }
    // execute the instuction
    // decode.op() may change `next_pc`
    next_pc = pc + 4;
    if (decode.op != nullptr) {
        decode.op(this, decode);
        gpr[0] = 0;
    } else {
        raise_exception(rv32i::EXCEPTION_ILLEGAL_INSTRUCTION, instruction);
    }
    // log register writing
    // memory events are logged by implements of memory accessing instructions
    // traps are logged by handle_trap()
    if (event_buffer!=nullptr && !exception_flag && decode.rd!=0) {
        event_buffer->push_back({.type=event_type_t::reg_write, .pc=pc, .val1=decode.rd, .val2=gpr[decode.rd]});
    }

    // handle traps
    next_pc = handle_trap();
    pc = next_pc;

    if (mmio_bus != nullptr) {
        mmio_bus->next_cycle();
    }
}
