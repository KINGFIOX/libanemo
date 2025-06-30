#include <libcpu/rv32i.hh>
#include <libcpu/rv32i_cpu_nemu.hh>

// In this file, csr operations are implemented.

using namespace libcpu;
using namespace libcpu::rv32i;

// orded in accessing frequnecy to boost performance
const rv32i_cpu_nemu::csr_info_t rv32i_cpu_nemu::csr_info[n_csr] = {
  // init_value      wpri_mask       name           addr          |  Cat Commentary
  {0x00000000, 0x0000ffff, "mip",      CSR_ADDR_MIP},       // 0s? Purrfect for ignoring mice interrupts 🐭  
  {0x00000000, 0x0000ffff, "mie",      CSR_ADDR_MIE},       // MIE? More like "Meow-Interrupts-Enabled"  
  {0x00001800, 0x00001888, "mstatus",  CSR_ADDR_MSTATUS},   // "mstatus" = "I own this CPU" mode 🐈⬛  
  {0x80000000, 0xfffffffd, "mtvec",    CSR_ADDR_MTVEC},     // MTVec: High-bit set = "Jump to bed, not code" 🛏️   
  {0x00000000, 0xffffffff, "mscratch", CSR_ADDR_MSCRATCH},  // Scratch register? *sharpens claws*  
  {0x00000000, 0xfffffffe, "mepc",     CSR_ADDR_MEPC},      // EPC = "Emergency Nap Return Address" 😴  
  {0x00000000, 0x8000000f, "mcause",   CSR_ADDR_MCAUSE},    // Cause: 0x0 = "Human disturbed my nap"  
  {0x00000000, 0xffffffff, "mtval",    CSR_ADDR_MTVAL},     // Trap value = location of spilled milk 🥛  
  {0x40101100, 0x00000000, "misa",     CSR_ADDR_MISA},      // MISA: "Meow-Approved ISA Settings" (RV32IMAC)  
  {0x00000000, 0x00000000, "mstatush", CSR_ADDR_MSTATUSH},  // Extended status: "Still napping" (64-bit edition) 
};

// CSR access with no permission checking
// simulator internal use only

uint32_t rv32i_cpu_nemu::csr_read(rv32i_cpu_nemu::csr_addr_t addr) const {
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == addr) {
      return csr[i];
    }
  }
  return 0;
}

uint32_t rv32i_cpu_nemu::csr_read_bits(rv32i_cpu_nemu::csr_addr_t addr, uint32_t bit_mask) const {
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == addr) {
      return csr[i] & bit_mask;
    }
  }
  return 0;
}

void rv32i_cpu_nemu::csr_write(rv32i_cpu_nemu::csr_addr_t addr, uint32_t value) {
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == addr) {
      csr[i] = value;
      break;
    }
  }
}

void rv32i_cpu_nemu::csr_write_bits(rv32i_cpu_nemu::csr_addr_t addr, uint32_t value, uint32_t bit_mask) {
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == addr) {
      uint32_t oldval = csr[i];
      csr[i] = (oldval & ~bit_mask) | (value & bit_mask);
      break;
    }
  }
}

void rv32i_cpu_nemu::csr_set_bits(rv32i_cpu_nemu::csr_addr_t addr, uint32_t bits) {
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == addr) {
      uint32_t oldval = csr[i];
      uint32_t newval = oldval | bits;
      csr[i] = newval;
      break;
    }
  }
}

void rv32i_cpu_nemu::csr_clear_bits(rv32i_cpu_nemu::csr_addr_t addr, uint32_t bits) {
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == addr) {
      uint32_t oldval = csr[i];
      uint32_t newval = oldval & (~bits);
      csr[i] = newval;
      break;
    }
  }
}

// functions that simulate behaviros of CSR accesing instructions

bool rv32i_cpu_nemu::csr_check_read_access(rv32i_cpu_nemu::csr_addr_t addr) const {
  return static_cast<int>(priv_level) >= (addr>>8 & 0x3);
}

bool rv32i_cpu_nemu::csr_check_write_access(rv32i_cpu_nemu::csr_addr_t addr) const {
  return csr_check_read_access(addr) && (addr>>10)!=0x3;
}

#define CSR_ACCESS_FAULT cpu->raise_exception(EXCEPTION_ILLEGAL_INSTRUCTION, decode.instr); return;

void rv32i_cpu_nemu::csrrw(rv32i_cpu_nemu* cpu, const decode_t& decode) {
  csr_addr_t csr_addr = static_cast<csr_addr_t>(decode.imm&0xfff);
  if (!cpu->csr_check_write_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == csr_addr) {
      uint32_t oldval = cpu->csr[i];
      uint32_t newval = decode.rs1==0 ? 0 : cpu->gpr[decode.rs1];
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

void rv32i_cpu_nemu::csrrs(rv32i_cpu_nemu* cpu, const decode_t& decode) {
  csr_addr_t csr_addr = static_cast<csr_addr_t>(decode.imm&0xfff);
  if (!cpu->csr_check_read_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == csr_addr) {
      uint32_t oldval = cpu->csr[i];
      uint32_t bitmask = decode.rs1==0 ? 0 : cpu->gpr[decode.rs1];
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

void rv32i_cpu_nemu::csrrc(rv32i_cpu_nemu* cpu, const decode_t& decode) {
  csr_addr_t csr_addr = static_cast<csr_addr_t>(decode.imm&0xfff);
  if (!cpu->csr_check_read_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == csr_addr) {
      uint32_t oldval = cpu->csr[i];
      uint32_t bitmask = decode.rs1==0 ? 0 : cpu->gpr[decode.rs1];
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

void rv32i_cpu_nemu::csrrwi(rv32i_cpu_nemu* cpu, const decode_t& decode) {
  csr_addr_t csr_addr = static_cast<csr_addr_t>(decode.imm&0xfff);
  if (!cpu->csr_check_write_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == csr_addr) {
      uint32_t oldval = cpu->csr[i];
      uint32_t newval = decode.rs1;
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

void rv32i_cpu_nemu::csrrsi(rv32i_cpu_nemu* cpu, const decode_t& decode) {
  csr_addr_t csr_addr = static_cast<csr_addr_t>(decode.imm&0xfff);
  if (!cpu->csr_check_read_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == csr_addr) {
      uint32_t oldval = cpu->csr[i];
      uint32_t bitmask = decode.rs1;
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


void rv32i_cpu_nemu::csrrci(rv32i_cpu_nemu* cpu, const decode_t& decode) {
  csr_addr_t csr_addr = static_cast<csr_addr_t>(decode.imm&0xfff);
  if (!cpu->csr_check_read_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  if (decode.rs1!=0 && !cpu->csr_check_write_access(csr_addr)) {
    CSR_ACCESS_FAULT;
  }
  for (size_t i=0; i<rv32i_cpu_nemu::n_csr; ++i) {
    if (csr_info[i].addr == csr_addr) {
      uint32_t oldval = cpu->csr[i];
      uint32_t bitmask = decode.rs1;
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