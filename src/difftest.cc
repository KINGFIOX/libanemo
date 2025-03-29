#include <iostream>
#include <algorithm>
#include <libcpu/rv32i_cpu.hh>
#include <sstream>
#include <cstddef>
#include <cstdint>
#include <libcpu/difftest.hh>
#include <libcpu/rv32i.hh>

using namespace libcpu;
using namespace libcpu::rv32i;

static const char* event_type_to_string(event_type_t type) {
    switch (type) {
        case event_type_t::issue:     return "issue";
        case event_type_t::reg_write: return "reg_write";
        case event_type_t::load:      return "load";
        case event_type_t::store:     return "store";
        case event_type_t::call:      return "call";
        case event_type_t::ret:       return "ret";
        case event_type_t::trap:      return "trap";
        case event_type_t::mret:      return "mret";
        case event_type_t::error:     return "error";
        default:                      return "unknown";
    }
}

static std::string format_event(const event_t& e) {
    std::ostringstream oss;
    oss << event_type_to_string(e.type);
    oss << " pc=0x" << std::hex << e.pc;

    switch (e.type) {
        case event_type_t::issue:
            oss << ", instruction=0x" << e.val1 << ", 0=0x" << e.val2;
            break;
        case event_type_t::reg_write:
            oss << ", rd=0x" << e.val1 << ", data=0x" << e.val2;
            break;
        case event_type_t::load:
            oss << ", addr=0x" << e.val1 << ", data=0x" << e.val2;
            break;
        case event_type_t::store:
            oss << ", addr=0x" << e.val1 << ", data=0x" << e.val2;
            break;
        case event_type_t::call:
            oss << ", target=0x" << e.val1 << ", stack_ptr=0x" << e.val2;
            break;
        case event_type_t::ret:
            oss << ", target=0x" << e.val1 << ", stack_ptr=0x" << e.val2;
            break;
        case event_type_t::trap:
            oss << ", mcause=0x" << e.val1 << ", mtval=0x" << e.val2;
            break;
        case event_type_t::mret:
            oss << ", target=0x" << e.val1 << ", mstatus=0x" << e.val2;
            break;
        case event_type_t::error:
            oss << ", type=0x" << e.val1 << ", instruction=0x" << e.val2;
            break;
        default:
            oss << ", val1=0x" << e.val1 << ", val2=0x" << e.val2;
            break;
    }

    return oss.str();
}

rv32i_cpu_difftest::rv32i_cpu_difftest(size_t event_buffer_size, size_t memory_size, std::initializer_list<rv32i_cpu*> init): rv32i_cpu(event_buffer_size, memory_size, nullptr), cpus(init) {
    buffer_index.resize(cpus.size());
    std::fill(buffer_index.begin(), buffer_index.end(),0);
    for (auto cpu: cpus) {
        cpu->trace_on = true;
    }
}

uint32_t rv32i_cpu_difftest::gpr_read(uint8_t gpr_addr) const {
    return cpus[0]->gpr_read(gpr_addr);
}

uint32_t rv32i_cpu_difftest::get_pc(void) const {
    return cpus[0]->get_pc();
}

priv_level_t rv32i_cpu_difftest::get_priv_level(void) const {
    return cpus[0]->get_priv_level();
}

void rv32i_cpu_difftest::next_cycle(void) {
    next_instruction();
}

void rv32i_cpu_difftest::next_instruction(void) {
    for (auto cpu: cpus) {
        cpu->next_instruction();
    }
    // difftest
    std::optional<event_t> events[static_cast<size_t>(event_type_t::error)] {};
    for (size_t cpu_index=0; cpu_index<cpus.size(); ++cpu_index) {
        // compare the new events in each buffer
        auto const& buffer = cpus[cpu_index]->get_events();
        for (size_t event_index=buffer_index[cpu_index]; event_index<buffer.lastindex(); ++event_index) {
            auto event = buffer[event_index];
            uint32_t event_type = static_cast<uint32_t>(event.type);
            if (events[event_type].has_value()) {
                // it this type of event has been seen before
                // compare it with the recorded one
                if (event != *events[event_type]) {
                    difftest_error = true;
                    std::cerr << "difftest error:" << std::endl;
                    std::cerr << format_event(*events[event_type]) << std::endl;
                    std::cerr << format_event(event) << std::endl;
                    if (trace_on) {
                        event_buffer.push_back({.type=event_type_t::error, .pc=event.pc, .val1=event_type, .val2=get_pc()});
                    }
                }
            } else {
                // if this type event is the first time to be seen in this instruction
                // record it
                events[event_type] = event;
                if (trace_on) {
                    event_buffer.push_back(event);
                }
            }
        }
        buffer_index[cpu_index] =buffer.lastindex();
    }
}

bool rv32i_cpu_difftest::get_ebreak(void) const {
    return difftest_error || cpus[0]->get_ebreak();
}

void rv32i_cpu_difftest::raise_interrupt(mcause_t mcause) {
    for (auto cpu: cpus) {
        cpu->raise_interrupt(mcause);
    }
}

uint32_t rv32i_cpu_difftest::csr_read(csr_addr_t addr) const {
    return cpus[0]->csr_read(addr);
}

uint32_t rv32i_cpu_difftest::csr_read_bits(csr_addr_t addr, uint32_t bit_mask) const {
    return cpus[0]->csr_read_bits(addr, bit_mask);
}

uint32_t rv32i_cpu_difftest::pmem_read(uint32_t addr, width_t width) const {
    return cpus[0]->pmem_read(addr, width);
}

bool rv32i_cpu_difftest::get_difftest_error(void) const {
    return difftest_error;
}

void rv32i_cpu_difftest::sync_memory(void) {
    auto mem_size = memory.size();
    for (auto cpu: cpus) {
        if (cpu->memory.size() < mem_size) {
            cpu->memory.resize(mem_size);
        }
        std::copy(memory.begin(), memory.end(), cpu->memory.begin());
    }
}

const uint32_t* rv32i_cpu_difftest::get_regfile(void) const {
    return cpus[0]->get_regfile();
};
