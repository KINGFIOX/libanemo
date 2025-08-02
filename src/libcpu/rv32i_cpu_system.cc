#include <cstdint>
#include <libcpu/riscv.hh>
#include <libcpu/rv32i_cpu_system.hh>

namespace libcpu {

std::vector<rv32i_cpu_system::csr_def_t> rv32i_cpu_system::csr_list = {
    // name        addr            init_value      wpri_mask
    {"mip",       riscv::csr_addr::mip,      0x00000000, 0x0000ffff},  // Interrupt pending bits
    {"mie",       riscv::csr_addr::mie,      0x00000000, 0x0000ffff},  // Interrupt enable bits
    {"mstatus",   riscv::csr_addr::mstatus,  0x00001800, 0x00001888},  // Machine state and control
    {"mtvec",     riscv::csr_addr::mtvec,    0x80000000, 0xfffffffd},  // Trap vector configuration
    {"mscratch",  riscv::csr_addr::mscratch, 0x00000000, 0xffffffff},  // Temporary storage
    {"mepc",      riscv::csr_addr::mepc,     0x00000000, 0xfffffffe},  // Exception return address
    {"mcause",    riscv::csr_addr::mcause,   0x00000000, 0x8000000f},  // Exception/interrupt cause
    {"mtval",     riscv::csr_addr::mtval,    0x00000000, 0xffffffff},  // Trap value/bad address
    {"misa",      riscv::csr_addr::misa,     0x40101100, 0x00000000},  // ISA feature flags
    {"mstatush",  riscv::csr_addr::mstatush, 0x00000000, 0x00000000}   // Extended machine status
};

const std::vector<rv32i_cpu_system::csr_def_t> &rv32i_cpu_system::get_csr_list(void) const {
    return rv32i_cpu_system::csr_list;
}

rv32i_cpu_system::decode_t rv32i_cpu_system::decode_instruction(uint32_t instruction) const {
    decode_t decode = riscv_cpu<uint32_t>::decode_instruction(instruction);

    RISCV32_INSTR_PAT(0b00110000001000000000000001110011, 0b11111111111111111111111111111111, r, mret);

    return decode;
}

void rv32i_cpu_system::handle_trap(void) {
    // handle exceptions
    if (except_mcause.has_value()) {
        if (except_mcause.value() == riscv::mcause<uint32_t>::except_breakpoint) {
            is_stopped = true;
            return;
        }
        // set MPP and MPIE
        csr_write_bits(libcpu::riscv::csr_addr::mstatus, static_cast<uint32_t>(priv_level)<<11, 
                      libcpu::riscv::mstatus<uint32_t>::mppl | libcpu::riscv::mstatus<uint32_t>::mpph);
        uint32_t mpie = csr_read_bits(libcpu::riscv::csr_addr::mstatus, libcpu::riscv::mstatus<uint32_t>::mie) << 4;
        csr_write_bits(libcpu::riscv::csr_addr::mstatus, mpie, libcpu::riscv::mstatus<uint32_t>::mpie);
        // clear MIE
        csr_clear_bits(libcpu::riscv::csr_addr::mstatus, libcpu::riscv::mstatus<uint32_t>::mie);
        // set other CSRs
        csr_write(libcpu::riscv::csr_addr::mepc, pc);
        csr_write(libcpu::riscv::csr_addr::mcause, except_mcause.value()&(~libcpu::riscv::mcause<uint32_t>::intr_mask));
        csr_write(libcpu::riscv::csr_addr::mtval, except_mtval.value());
        // exception handler is never vectored
        uint32_t mtvec = csr_read(libcpu::riscv::csr_addr::mtvec);
        uint32_t vector_base = mtvec & (~libcpu::riscv::mtvec<uint32_t>::vectored);
        if (event_buffer!=nullptr) {
            event_buffer->push_back({.type=event_type_t::trap, .pc=pc, .val1=except_mcause.value(), .val2=except_mtval.value()});
        }
        priv_level = libcpu::riscv::priv_level_t::m;
        next_trap = except_mcause;
        next_pc = vector_base;
        except_mcause = {};
        except_mtval = {};
        return;
    }
    
    // handle interrupts
    uint32_t mip = csr[0]; // csr_read(CSR_ADDR_MIP) is slower
    if (mip != 0) {
        uint32_t mstatus = csr_read(libcpu::riscv::csr_addr::mstatus);
        // if MIE=0 and the privilege mode is M, do not handle interrupts
        if (priv_level==libcpu::riscv::priv_level_t::m && !(mstatus&libcpu::riscv::mstatus<uint32_t>::mie)) {
            return;
        }
        uint32_t mie = csr_read(libcpu::riscv::csr_addr::mie);
        for (uint32_t cause=0; cause<16; ++cause) {
            if ((mip>>cause)&1 && (mie>>cause)&1) {
                // set MPP and MPIE
                csr_write_bits(libcpu::riscv::csr_addr::mstatus, static_cast<uint32_t>(priv_level)<<11, 
                              libcpu::riscv::mstatus<uint32_t>::mppl | libcpu::riscv::mstatus<uint32_t>::mpph);
                uint32_t mpie = csr_read_bits(libcpu::riscv::csr_addr::mstatus, libcpu::riscv::mstatus<uint32_t>::mie) << 4;
                csr_write_bits(libcpu::riscv::csr_addr::mstatus, mpie, libcpu::riscv::mstatus<uint32_t>::mpie);
                // clear MIE
                csr_clear_bits(libcpu::riscv::csr_addr::mstatus, libcpu::riscv::mstatus<uint32_t>::mie);
                // set other CSRs
                csr_write(libcpu::riscv::csr_addr::mepc, next_pc);
                csr_write(libcpu::riscv::csr_addr::mcause, cause|libcpu::riscv::mcause<uint32_t>::intr_mask);
                csr_write(libcpu::riscv::csr_addr::mtval, 0);
                // jump to the interrupt handler
                // interrupt handler can be vectored
                uint32_t mtvec = csr_read(libcpu::riscv::csr_addr::mtvec);
                uint32_t vector_base = mtvec & (~libcpu::riscv::mtvec<uint32_t>::vectored);
                uint32_t is_vectord = mtvec & libcpu::riscv::mtvec<uint32_t>::vectored;
                if (event_buffer!=nullptr) {
                    event_buffer->push_back({.type=event_type_t::trap, .pc=next_pc, .val1=cause|libcpu::riscv::mcause<uint32_t>::intr_mask, .val2=0});
                }
                priv_level = libcpu::riscv::priv_level_t::m;
                next_trap = {csr_read(libcpu::riscv::csr_addr::mcause)};
                if (is_vectord) {
                    next_pc = vector_base + cause*4;
                    return;
                } else {
                    next_pc = vector_base;
                    return;
                }
            }
        }
    }
    next_trap = {};
}

void rv32i_cpu_system::mret(riscv_cpu<uint32_t>* riscv_cpu, const decode_t& decode) {
    rv32i_cpu_system *cpu = static_cast<rv32i_cpu_system*>(riscv_cpu);
    if (cpu->get_priv_level() != riscv::priv_level_t::m) {
        cpu->raise_exception(riscv::mcause<uint32_t>::except_illegal_instr, decode.instr);
        return;
    }
    uint32_t mepc = cpu->csr_read(riscv::csr_addr::mepc);
    if (cpu->event_buffer!=nullptr) {
        uint32_t mstatus = cpu->csr_read(riscv::csr_addr::mstatus);
        cpu->event_buffer->push_back({.type=event_type_t::trap_ret, .pc=cpu->pc, .val1=mepc, .val2=0});
    }
    // restore pc
    cpu->next_pc = mepc;
    // restore privilege level from MPP bits
    cpu->priv_level = static_cast<riscv::priv_level_t>(
        cpu->csr_read_bits(riscv::csr_addr::mstatus, 
                          riscv::mstatus<uint32_t>::mppl | riscv::mstatus<uint32_t>::mpph) >> 11);
    
    // restore MIE bit from MPIE
    uint32_t mie = cpu->csr_read_bits(riscv::csr_addr::mstatus, riscv::mstatus<uint32_t>::mpie) >> 4;
    cpu->csr_write_bits(riscv::csr_addr::mstatus, mie, riscv::mstatus<uint32_t>::mie);
    
    // restore the MPIE bit (set to 1)
    cpu->csr_set_bits(riscv::csr_addr::mstatus, riscv::mstatus<uint32_t>::mpie);
    
    // clear MPP bits (set to 0)
    cpu->csr_clear_bits(riscv::csr_addr::mstatus, 
                       riscv::mstatus<uint32_t>::mppl | riscv::mstatus<uint32_t>::mpph);
}


}
