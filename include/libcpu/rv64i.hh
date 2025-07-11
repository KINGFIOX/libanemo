#ifndef LIBCPU_RV64I_HH
#define LIBCPU_RV64I_HH

#include <cstdint>
#include <libcpu/riscv.hh>
#include <libvio/frontend.hh>

namespace libcpu::rv64i {

using width_t = libvio::width_t;
using priv_level_t = riscv::priv_level_t;
using gpr_addr_t = riscv::gpr_addr_t;

enum csr_addr_t {
    CSR_ADDR_MSTATUS  = 0x300,  // Purr-ivilege mode status
    CSR_ADDR_MISA     = 0x301,
    CSR_ADDR_MIE      = 0x304,  // Meow-terrupt Enable
    CSR_ADDR_MTVEC    = 0x305,  // Meow-achine Trap Vector
    CSR_ADDR_MSCRATCH = 0x340,  // Scratch reg for catnip calculations
    CSR_ADDR_MEPC     = 0x341,  // Where to resume after nap... err, trap
    CSR_ADDR_MCAUSE   = 0x342,  // "Why did I get interrupted?" register
    CSR_ADDR_MTVAL    = 0x343,  // Bad address or instruction (hairball info)
    CSR_ADDR_MIP      = 0x344   // Meow-Interrupt Pending (laser pointer detected!)
};

enum csr_bitmask_t: uint64_t {  // Changed to uint64_t for 64-bit registers
    // MTVEC bits
    MTVEC_BIT_VECTORED = 1,

    // MIP bits (0x344) - Interrupt Pending
    MIP_BIT_SSIP    = 1ULL << 1,   // Supervisor Soft-paw Interrupt Pending
    MIP_BIT_MSIP    = 1ULL << 3,   // Machine Soft-paw Interrupt Pending
    MIP_BIT_STIP    = 1ULL << 5,   // Supervisor Tuna Timer Interrupt Pending
    MIP_BIT_MTIP    = 1ULL << 7,   // Machine Tuna Timer Interrupt Pending
    MIP_BIT_SEIP    = 1ULL << 9,   // Supervisor External Laser Pointer Pending
    MIP_BIT_MEIP    = 1ULL << 11,  // Machine External Laser Pointer Pending
    MIP_BIT_LCOFIP  = 1ULL << 13,  // Local Catnip Overflow Interrupt (if implemented)

    // MIE bits (0x304) - Interrupt Enable
    MIE_BIT_SSIE    = 1ULL << 1,   // Enable Supervisor Belly Rub Requests
    MIE_BIT_MSIE    = 1ULL << 3,   // Enable Machine Belly Rub Requests
    MIE_BIT_STIE    = 1ULL << 5,   // Enable Supervisor Nap Timer
    MIE_BIT_MTIE    = 1ULL << 7,   // Enable Machine Nap Timer
    MIE_BIT_SEIE    = 1ULL << 9,   // Enable Supervisor Door Opening Detection
    MIE_BIT_MEIE    = 1ULL << 11,  // Enable Machine Door Opening Detection
    MIE_BIT_LCOFIE  = 1ULL << 13,  // Enable Catnip Overflow Notifications

    // MSTATUS bits (0x300) - Status Reg
    MSTATUS_BIT_SIE  = 1ULL << 1,  // Supervisor Interrupt Enable
    MSTATUS_BIT_MIE  = 1ULL << 3,  // Machine Interrupt Enable
    MSTATUS_BIT_SPIE = 1ULL << 5,  // Previous S-mode Interrupt State
    MSTATUS_BIT_UBE  = 1ULL << 6,  // User-mode Big-Endian
    MSTATUS_BIT_MPIE = 1ULL << 7,  // Previous M-mode Interrupt State
    MSTATUS_BIT_MPP_SHIFT = 11,    // MPP field shift position
    MSTATUS_BIT_MPP_MASK  = 3ULL << MSTATUS_BIT_MPP_SHIFT,
    MSTATUS_BIT_MPRV = 1ULL << 17, // Memory Privilege Mode
    MSTATUS_BIT_SUM  = 1ULL << 18, // Supervisor User Memory access
    MSTATUS_BIT_MXR  = 1ULL << 19, // Make eXecutable Readable
    MSTATUS_BIT_TVM  = 1ULL << 20, // Trap Virtual Memory ops
    MSTATUS_BIT_TW   = 1ULL << 21, // Timeout Wait
    MSTATUS_BIT_TSR  = 1ULL << 22, // Trap SRET
    MSTATUS_BIT_UXL  = 1ULL << 32, // User XLEN (fixed to 64 in U-mode)
    MSTATUS_BIT_SXL  = 1ULL << 34, // Supervisor XLEN (fixed to 64 in S-mode)
    MSTATUS_BIT_SD   = 1ULL << 63, // Dirty state flag (moved to bit 63)

    // MCAUSE bits
    MCAUSE_BIT_INTERRUPT = 1ULL << 63, // Interrupt flag at bit 63
    MCAUSE_CODE_MASK     = (1ULL << 63) - 1, // Mask for exception code
};

enum mcause_t: uint64_t {  // Changed to uint64_t for 64-bit cause register
    // =^..^=  Interrupts (bit 63 set)  =^..^=
    INTERRUPT_S_SOFTWARE      = (1ULL << 63) | 1,    // Supervisor's yarn ball
    INTERRUPT_M_SOFTWARE      = (1ULL << 63) | 3,    // Machine's laser pointer
    INTERRUPT_S_TIMER         = (1ULL << 63) | 5,    // Supurrvisor alarm clock
    INTERRUPT_M_TIMER         = (1ULL << 63) | 7,    // Meal time interrupt 😼
    INTERRUPT_S_EXTERNAL      = (1ULL << 63) | 9,    // Doorbell ring detected
    INTERRUPT_M_EXTERNAL      = (1ULL << 63) | 11,   // Vacuum cleaner alert!
    INTERRUPT_M_COUNTER_OVERFLOW = (1ULL << 63) | 13,// Too many mice counted

    // =^..^=  Exceptions (bit 63 clear)  =^..^=
    EXCEPTION_INSTRUCTION_ADDRESS_MISALIGNED  = 0,  // Cat stepped on keyboard
    EXCEPTION_INSTRUCTION_ACCESS_FAULT        = 1,  // Forbidden sunbeam area
    EXCEPTION_ILLEGAL_INSTRUCTION             = 2,  // Tried to bark 🐶?!
    EXCEPTION_BREAKPOINT                      = 3,  // Paws on keyboard event
    EXCEPTION_LOAD_ADDRESS_MISALIGNED         = 4,  // Bad tuna can alignment
    EXCEPTION_LOAD_ACCESS_FAULT               = 5,  // Empty food bowl error
    EXCEPTION_STORE_AMO_ADDRESS_MISALIGNED    = 6,  // Litter box placement fail
    EXCEPTION_STORE_AMO_ACCESS_FAULT          = 7,  // Closed door exception

    EXCEPTION_U_ECALL                         = 8,  // User-mode meow request
    EXCEPTION_S_ECALL                         = 9,  // Supervisor pets needed
    EXCEPTION_M_ECALL                         = 11, // Machine wants lap time

    EXCEPTION_INSTRUCTION_PAGE_FAULT          = 12, // Book fell off shelf
    EXCEPTION_LOAD_PAGE_FAULT                 = 13, // Blanket not found 😿
    EXCEPTION_STORE_AMO_PAGE_FAULT            = 15, // Toy storage full

    EXCEPTION_SOFTWARE_CHECK                  = 18, // Hairball detected
    EXCEPTION_HARDWARE_ERROR                  = 19, // Scratched furniture
};

}

#endif

