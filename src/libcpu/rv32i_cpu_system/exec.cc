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
    
    auto instruction_opt = memory.read(pc, libvio::width_t::word);
    if (! instruction_opt.has_value()) {
        raise_exception(rv32i::EXCEPTION_INSTRUCTION_ACCESS_FAULT, pc);
        return;
    }

    uint32_t instruction = instruction_opt.value();
    size_t decode_cache_offset = (pc-mem_base) << 2;
    decode_t decode;
    if (decode_cache_offset >= decode_cache.size()) {
        // if the instruction is not in the decode cache
        // decode it and store the decoding into the cache
        size_t new_size = std::max(decode_cache.size()*2, decode_cache_offset+1);
        new_size = std::min(new_size, size_t(memory.get_size())>>2);
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
}

void rv32i_cpu_system::pre_decode(void) {
    decode_cache.resize(memory.get_size()/4);
    for (size_t i=0; i<memory.get_size(); i+=4) {
        instruction = memory.read_offset(i, libvio::width_t::word).value_or(0);
        decode_cache[i/4] = decode_instruction(instruction);
    }
}

void rv32i_cpu_system::next_instruction_pre_decode(void) {
    // try to execute the instruction
    uint32_t offset = pc - mem_base;
    size_t decode_cache_offset = offset >> 2;
    decode_t decode = decode_cache[decode_cache_offset];
    next_pc = pc + 4;
    if (decode.op != nullptr) {
        decode.op(this, decode);
        gpr[0] = 0;
    } else {
        raise_exception(rv32i::EXCEPTION_ILLEGAL_INSTRUCTION, instruction);
    }
    // handle traps
    next_pc = handle_trap();
    pc = next_pc;
}

