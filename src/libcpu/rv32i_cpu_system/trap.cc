#include <cstdint>
#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_system.hh>

// In this file, trap related instructions and functions are defined.

using namespace libcpu;
using namespace libcpu::rv32i;

void rv32i_cpu_system::ecall(rv32i_cpu_system* cpu, const decode_t& decode) {
    switch (cpu->priv_level) {
        case priv_level_t::m:
            cpu->raise_exception(EXCEPTION_M_ECALL, 0);
            break;
        case priv_level_t::h:
            cpu->raise_exception(EXCEPTION_M_ECALL, 0);
            break;
        case priv_level_t::s:
            cpu->raise_exception(EXCEPTION_S_ECALL, 0);
            break;
        case priv_level_t::u:
            cpu->raise_exception(EXCEPTION_U_ECALL, 0);
            break;
    }
}

void rv32i_cpu_system::ebreak(rv32i_cpu_system* cpu, const decode_t& decode) {
    cpu->ebreak_flag = true;
}

void rv32i_cpu_system::mret(rv32i_cpu_system* cpu, const decode_t& decode) {
    uint32_t mepc = cpu->csr_read(CSR_ADDR_MEPC);
    if (cpu->event_buffer!=nullptr) {
        uint32_t mstatus = cpu->csr_read(CSR_ADDR_MSTATUS);
        cpu->event_buffer->push_back({.type=event_type_t::trap_ret, .pc=cpu->pc, .val1=mepc, .val2=mstatus});
    }
    // retore pc
    cpu->next_pc = mepc;
    cpu->priv_level = static_cast<priv_level_t>(cpu->csr_read_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MPPH|MSTATUS_BIT_MPPL)>>11);
    // restore MIE bit
    uint32_t mie = cpu->csr_read_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MPIE) >> 4;
    cpu->csr_write_bits(CSR_ADDR_MSTATUS, mie, MSTATUS_BIT_MIE);
    // restore the MPIE bit
    cpu->csr_set_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MPIE);
    // retsore MPP bits
    cpu->csr_clear_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MPPH|MSTATUS_BIT_MPPL);
}

// returns the pc of the next instruction
uint32_t rv32i_cpu_system::handle_trap(void) {
    // handle exceptions
    if (exception_flag) {
        exception_flag = false;
        // set MPP and MPIE
        csr_write_bits(CSR_ADDR_MSTATUS, static_cast<uint32_t>(priv_level)<<11, MSTATUS_BIT_MPPH|MSTATUS_BIT_MPPL);
        uint32_t mpie = csr_read_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MIE) << 4;
        csr_write_bits(CSR_ADDR_MSTATUS, mpie, MSTATUS_BIT_MPIE);
        // clear MIE
        csr_clear_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MIE);
        // set other CSRs
        csr_write(CSR_ADDR_MEPC, pc);
        csr_write(CSR_ADDR_MCAUSE, exception_cause&(~MCAUSE_BIT_INTERRUPT));
        csr_write(CSR_ADDR_MTVAL, exception_mtval);
        // exception handler is never vectored
        uint32_t mtvec = csr_read(CSR_ADDR_MTVEC);
        uint32_t vector_base = mtvec & (~MTVEC_BIT_VECTORED);
        if (event_buffer!=nullptr) {
            event_buffer->push_back({.type=event_type_t::trap, .pc=pc, .val1=exception_cause, .val2=exception_mtval});
        }
        priv_level = priv_level_t::m;
        next_trap = {csr_read(CSR_ADDR_MCAUSE)};
        return vector_base;
    }
    
    // handle interrupts
    uint32_t mip = csr[0]; // csr_read(CSR_ADDR_MIP) is slower
    if (mip != 0) {
        uint32_t mstatus = csr_read(CSR_ADDR_MSTATUS);
        // if MIE=0 and the privilege mode is M, do not handle interrupts
        if (priv_level==priv_level_t::m && !(mstatus&MSTATUS_BIT_MIE)) {
            return next_pc;
        }
        uint32_t mie = csr_read(CSR_ADDR_MIE);
        for (uint32_t cause=0; cause<n_interrupt; ++cause) {
            if ((mip>>cause)&1 && (mie>>cause)&1) {
                // set MPP and MPIE
                csr_write_bits(CSR_ADDR_MSTATUS, static_cast<uint32_t>(priv_level)<<11, MSTATUS_BIT_MPPH|MSTATUS_BIT_MPPL);
                uint32_t mpie = csr_read_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MIE) << 4;
                csr_write_bits(CSR_ADDR_MSTATUS, mpie, MSTATUS_BIT_MPIE);
                // clear MIE
                csr_clear_bits(CSR_ADDR_MSTATUS, MSTATUS_BIT_MIE);
                // set other CSRs
                csr_write(CSR_ADDR_MEPC, next_pc);
                csr_write(CSR_ADDR_MCAUSE, cause|MCAUSE_BIT_INTERRUPT);
                csr_write(CSR_ADDR_MTVAL, 0);
                // jump to the interrupt handler
                // interrupt handler can be vectored
                uint32_t mtvec = csr_read(CSR_ADDR_MTVEC);
                uint32_t vector_base = mtvec & (~MTVEC_BIT_VECTORED);
                uint32_t is_vectord = mtvec & MTVEC_BIT_VECTORED;
                if (event_buffer!=nullptr) {
                    event_buffer->push_back({.type=event_type_t::trap, .pc=next_pc, .val1=cause|MCAUSE_BIT_INTERRUPT, .val2=0});
                }
                priv_level = priv_level_t::m;
                next_trap = {csr_read(CSR_ADDR_MCAUSE)};
                if (is_vectord) {
                    return vector_base + cause*4;
                } else {
                    return vector_base;
                }
            }
        }
    }
    next_trap = {};
    return next_pc;
}

void rv32i_cpu_system::raise_exception(mcause_t mcause_code, uint32_t mtval) {
    exception_flag = true;
    exception_cause = mcause_code;
    exception_mtval = mtval;
}

void rv32i_cpu_system::raise_interrupt(mcause_t mcause_code) {
    if (mcause_code < n_interrupt) {
        csr_set_bits(CSR_ADDR_MIP, 1<<mcause_code);
    }
}
