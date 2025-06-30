#include <cstddef>
#include <cstdint>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_nemu.hh>

using namespace libcpu;
using namespace libcpu::rv32i;

void rv32i_cpu_nemu::next_cycle(void) {
    next_instruction();
}

void rv32i_cpu_nemu::next_instruction(void) {
    if (pc%4 != 0) {
        raise_exception(EXCEPTION_INSTRUCTION_ADDRESS_MISALIGNED, pc);
    } else if (pc<ram_base || pc+3>=ram_base+memory.size()) {
        raise_exception(rv32i::EXCEPTION_INSTRUCTION_ACCESS_FAULT, pc);
    } else {
        // try to execute the instruction
        uint32_t offset = pc - ram_base;
        size_t decode_cache_offset = offset >> 2;
        // fetch an instruction
        instruction = uint32_t(memory[offset+3]<<24) | uint32_t(memory[offset+2]<<16) | uint32_t(memory[offset+1]<<8) | uint32_t(memory[offset]);
        decode_t decode;
        if (decode_cache_offset >= decode_cache.size()) {
            // if the instruction is not in the decode cache
            // decode it and store the decoding into the cache
            size_t new_size = std::max(decode_cache.size()*2, decode_cache_offset+1);
            new_size = std::min(new_size, memory.size()>>2);
            decode_cache.resize(new_size);
            decode = decode_instruction(instruction);
            decode_cache[decode_cache_offset] = decode;
        } else {
            // if there is a cache invalidation, update the cache
            decode = decode_cache[decode_cache_offset];
            if (decode.instr != instruction) {
                decode = decode_instruction(instruction);
                decode_cache[decode_cache_offset] = decode;
            }
        }
        // log the instruction
        if (trace_on) {
            event_buffer.push_back({.type=event_type_t::issue, .pc=pc, .val1=instruction, .val2=0});
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
        if (trace_on && !exception_flag && decode.rd!=0) {
            event_buffer.push_back({.type=event_type_t::reg_write, .pc=pc, .val1=decode.rd, .val2=gpr[decode.rd]});
        }
    }
    // handle traps
    next_pc = handle_trap();
    pc = next_pc;
}
