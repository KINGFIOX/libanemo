#ifndef LIBCPU_RISCV_CPU_CSR_HH
#define LIBCPU_RISCV_CPU_CSR_HH

#include <libcpu/riscv.hh>
#include <libcpu/riscv_cpu/riscv_cpu.hh>

namespace libcpu {

template <typename WORD_T>
const std::vector<typename riscv_cpu<WORD_T>::csr_def_t> &riscv_cpu<WORD_T>::csr_list(void) const {
    return supported_csrs;
}

// CSR access with no permission checking
// simulator internal use only

template <typename WORD_T>
WORD_T riscv_cpu<WORD_T>::csr_read(uint16_t addr) const {
    const std::vector<csr_def_t> &csr_info = csr_list();
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == addr) {
            return csr[i];
        }
    }
    return 0;
}

template <typename WORD_T>
WORD_T riscv_cpu<WORD_T>::csr_read_bits(uint16_t addr, WORD_T bit_mask) const {
    const std::vector<csr_def_t> &csr_info = csr_list();
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == addr) {
            return csr[i] & bit_mask;
        }
    }
    return 0;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csr_write(uint16_t addr, WORD_T value) {
    const std::vector<csr_def_t> &csr_info = csr_list();
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == addr) {
            csr[i] = value;
            break;
        }
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csr_write_bits(uint16_t addr, WORD_T value, WORD_T bit_mask) {
    const std::vector<csr_def_t> &csr_info = csr_list();
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == addr) {
            WORD_T oldval = csr[i];
            csr[i] = (oldval & ~bit_mask) | (value & bit_mask);
            break;
        }
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csr_set_bits(uint16_t addr, WORD_T bits) {
    const std::vector<csr_def_t> &csr_info = csr_list();
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == addr) {
            WORD_T oldval = csr[i];
            WORD_T newval = oldval | bits;
            csr[i] = newval;
            break;
        }
    }
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csr_clear_bits(uint16_t addr, WORD_T bits) {
    const std::vector<csr_def_t> &csr_info = csr_list();
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == addr) {
            WORD_T oldval = csr[i];
            WORD_T newval = oldval & (~bits);
            csr[i] = newval;
            break;
        }
    }
}

// functions that simulate behaviros of CSR accesing instructions

template <typename WORD_T>
bool riscv_cpu<WORD_T>::csr_check_read_access(uint16_t addr) const {
    return static_cast<int>(priv_level) >= (addr>>8 & 0x3);
}

template <typename WORD_T>
bool riscv_cpu<WORD_T>::csr_check_write_access(uint16_t addr) const {
    return csr_check_read_access(addr) && (addr>>10)!=0x3;
}

#define CSR_ACCESS_FAULT cpu->raise_exception(riscv::mcause<WORD_T>::except_illegal_instr, decode.instr); return;

template <typename WORD_T>
void riscv_cpu<WORD_T>::csrrw(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    const std::vector<csr_def_t> &csr_info = cpu->csr_list();
    uint16_t csr_addr = static_cast<uint16_t>(decode.imm&0xfff);
    if (!cpu->csr_check_write_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == csr_addr) {
            WORD_T oldval = cpu->csr[i];
            WORD_T newval = decode.rs1==0 ? 0 : cpu->gpr[decode.rs1];
            if (decode.rd != 0) {
                // write access implies read access
                // so no need to check here
                cpu->gpr[decode.rd] = oldval;
                // read side effects here
            }
            cpu->csr[i] = (oldval & ~csr_info[i].wpri_mask) | (newval & csr_info[i].wpri_mask);
            // write side effects here
            return;
        }
    }
    // trap if attempting to access a non-existing CSR
    CSR_ACCESS_FAULT;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csrrs(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    const std::vector<csr_def_t> &csr_info = cpu->csr_list();
    uint16_t csr_addr = static_cast<uint16_t>(decode.imm&0xfff);
    if (!cpu->csr_check_read_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == csr_addr) {
            WORD_T oldval = cpu->csr[i];
            WORD_T bitmask = decode.rs1==0 ? 0 : cpu->gpr[decode.rs1];
            if (decode.rd != 0) {
                cpu->gpr[decode.rd] = oldval;
            }
            // read side effects here
            if (decode.rs1 != 0) {
                bitmask &= csr_info[i].wpri_mask;
                cpu->csr[i] = oldval | bitmask;
                // write side effects here
            }
            return;
        }
    }
    // trap if attempting to access a non-existing CSR
    CSR_ACCESS_FAULT;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csrrc(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    const std::vector<csr_def_t> &csr_info = cpu->csr_list();
    uint16_t csr_addr = static_cast<uint16_t>(decode.imm&0xfff);
    if (!cpu->csr_check_read_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == csr_addr) {
            WORD_T oldval = cpu->csr[i];
            WORD_T bitmask = decode.rs1==0 ? 0 : cpu->gpr[decode.rs1];
            // write access implies read access
            // so no need to check here
            if (decode.rd != 0) {
                cpu->gpr[decode.rd] = oldval;
            }
            // read side effects here
            if (decode.rs1 != 0) {
                bitmask &= csr_info[i].wpri_mask;
                cpu->csr[i] = oldval & ~bitmask;
                // write side effects here
            }
            return;
        }
    }
    // trap if attempting to access a non-existing CSR
    CSR_ACCESS_FAULT;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csrrwi(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    const std::vector<csr_def_t> &csr_info = cpu->csr_list();
    uint16_t csr_addr = static_cast<uint16_t>(decode.imm&0xfff);
    if (!cpu->csr_check_write_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == csr_addr) {
            WORD_T oldval = cpu->csr[i];
            WORD_T newval = decode.rs1;
            if (decode.rd != 0) {
                // write access implies read access
                // so no need to check here
                cpu->gpr[decode.rd] = oldval;
                // read side effects here
            }
            cpu->csr[i] = (oldval & ~csr_info[i].wpri_mask) | (newval & csr_info[i].wpri_mask);
            // write side effects here
            return;
        }
    }
    // trap if attempting to access a non-existing CSR
    CSR_ACCESS_FAULT;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csrrsi(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    const std::vector<csr_def_t> &csr_info = cpu->csr_list();
    uint16_t csr_addr = static_cast<uint16_t>(decode.imm&0xfff);
    if (!cpu->csr_check_read_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == csr_addr) {
            WORD_T oldval = cpu->csr[i];
            WORD_T bitmask = decode.rs1;
            if (decode.rd != 0) {
                cpu->gpr[decode.rd] = oldval;
            }
            // read side effects here
            if (decode.rs1 != 0) {
                bitmask &= csr_info[i].wpri_mask;
                cpu->csr[i] = oldval | bitmask;
                // write side effects here
            }
            return;
        }
    }
    // trap if attempting to access a non-existing CSR
    CSR_ACCESS_FAULT;
}

template <typename WORD_T>
void riscv_cpu<WORD_T>::csrrci(riscv_cpu<WORD_T>* cpu, const decode_t& decode) {
    const std::vector<csr_def_t> &csr_info = cpu->csr_list();
    uint16_t csr_addr = static_cast<uint16_t>(decode.imm&0xfff);
    if (!cpu->csr_check_read_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
        CSR_ACCESS_FAULT;
    }
    for (size_t i=0; i<csr_info.size(); ++i) {
        if (csr_info[i].addr == csr_addr) {
            WORD_T oldval = cpu->csr[i];
            WORD_T bitmask = decode.rs1;
            // write access implies read access
            // so no need to check here
            if (decode.rd != 0) {
                cpu->gpr[decode.rd] = oldval;
            }
            // read side effects here
            if (decode.rs1 != 0) {
                bitmask &= csr_info[i].wpri_mask;
                cpu->csr[i] = oldval & ~bitmask;
                // write side effects here
            }
            return;
        }
    }
    // trap if attempting to access a non-existing CSR
    CSR_ACCESS_FAULT;
}

}

#endif
