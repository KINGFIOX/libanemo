#ifndef LIBCPU_RV32I_HH
#define LIBCPU_RV32I_HH

// In this file, some architecture dependent but implementation independent constants are defined

#include <cstdint>
#include <libcpu/riscv.hh>
#include <libvio/frontend.hh>

namespace libcpu::rv32i {

using width_t = libvio::width_t;
using priv_level_t = riscv::priv_level_t;
using gpr_addr_t = riscv::gpr_addr_t;

enum csr_addr_t {
    CSR_ADDR_MSTATUS  = 0x300,  // Purr-ivilege mode status
    CSR_ADDR_MISA     = 0x301,
    CSR_ADDR_MIE      = 0x304,  // Meow-terrupt Enable
    CSR_ADDR_MTVEC    = 0x305,  // Meow-achine Trap Vector
    CSR_ADDR_MSTATUSH = 0x310,  // Upper bits for RV32 (tail twitch)
    CSR_ADDR_MSCRATCH = 0x340,  // Scratch reg for catnip calculations
    CSR_ADDR_MEPC     = 0x341,  // Where to resume after nap... err, trap
    CSR_ADDR_MCAUSE   = 0x342,  // "Why did I get interrupted?" register
    CSR_ADDR_MTVAL    = 0x343,  // Bad address or instruction (hairball info)
    CSR_ADDR_MIP      = 0x344   // Meow-Interrupt Pending (laser pointer detected!)
};

enum csr_bitmask_t: uint32_t {
    // MTVEC bits
    MTVEC_BIT_VECTORED = 1,

    // MIP bits (0x344) - Interrupt Pending
    MIP_BIT_SSIP    = 1 << 1,   // Supervisor Soft-paw Interrupt Pending
    MIP_BIT_MSIP    = 1 << 3,   // Machine Soft-paw Interrupt Pending
    MIP_BIT_STIP    = 1 << 5,   // Supervisor Tuna Timer Interrupt Pending
    MIP_BIT_MTIP    = 1 << 7,   // Machine Tuna Timer Interrupt Pending
    MIP_BIT_SEIP    = 1 << 9,   // Supervisor External Laser Pointer Pending
    MIP_BIT_MEIP    = 1 << 11,  // Machine External Laser Pointer Pending
    MIP_BIT_LCOFIP  = 1 << 13,  // Local Catnip Overflow Interrupt (if implemented)

    // MIE bits (0x304) - Interrupt Enable
    MIE_BIT_SSIE    = 1 << 1,   // Enable Supervisor Belly Rub Requests
    MIE_BIT_MSIE    = 1 << 3,   // Enable Machine Belly Rub Requests
    MIE_BIT_STIE    = 1 << 5,   // Enable Supervisor Nap Timer
    MIE_BIT_MTIE    = 1 << 7,   // Enable Machine Nap Timer
    MIE_BIT_SEIE    = 1 << 9,   // Enable Supervisor Door Opening Detection
    MIE_BIT_MEIE    = 1 << 11,  // Enable Machine Door Opening Detection
    MIE_BIT_LCOFIE  = 1 << 13,  // Enable Catnip Overflow Notifications

    // MSTATUS bits (0x300) - Status Reg
    MSTATUS_BIT_SIE  = 1 << 1,  // Supervisor Interrupt Enable (for cat staff)
    MSTATUS_BIT_MIE  = 1 << 3,  // Machine Interrupt Enable (for sysadmin cats)
    MSTATUS_BIT_SPIE = 1 << 5,  // Previous S-mode Interrupt State
    MSTATUS_BIT_UBE  = 1 << 6,  // User-mode Big-Endian (rare for cats)
    MSTATUS_BIT_MPIE = 1 << 7,  // Previous M-mode Interrupt State
    MSTATUS_BIT_MPPL = 1 << 11, // Previous Privilege Level Low
    MSTATUS_BIT_MPPH = 1 << 12, // Previous Privilige Level High
    MSTATUS_BIT_MPRV = 1 << 17, // Memory Privilege Mode (for cat burglary)
    MSTATUS_BIT_SUM  = 1 << 18, // Supervisor User Memory access (controlled sharing)
    MSTATUS_BIT_MXR  = 1 << 19, // Make eXecutable Readable (code = data to cats)
    MSTATUS_BIT_TVM  = 1 << 20, // Trap Virtual Memory ops (protect nap spaces)
    MSTATUS_BIT_TW   = 1 << 21, // Timeout Wait (don't let hoomans wait too long)
    MSTATUS_BIT_TSR  = 1 << 22, // Trap SRET (secure return from cat staff)
    MSTATUS_BIT_SD   = (uint32_t)1 << 31, // Dirty state flag

    // MCAUSE bits
    MCAUSE_BIT_INTERRUPT = (uint32_t)1<<31,
};

enum mcause_t: uint32_t {
    // =^..^=  Interrupts (bit 31 set)  =^..^=
    INTERRUPT_S_SOFTWARE      = (1U << 31) | 1,    // Supervisor's yarn ball
    INTERRUPT_M_SOFTWARE      = (1U << 31) | 3,    // Machine's laser pointer
    INTERRUPT_S_TIMER         = (1U << 31) | 5,    // Supurrvisor alarm clock
    INTERRUPT_M_TIMER         = (1U << 31) | 7,    // Meal time interrupt 😼
    INTERRUPT_S_EXTERNAL      = (1U << 31) | 9,    // Doorbell ring detected
    INTERRUPT_M_EXTERNAL      = (1U << 31) | 11,   // Vacuum cleaner alert!
    INTERRUPT_M_COUNTER_OVERFLOW = (1U << 31) | 13,// Too many mice counted
    
    // Reserved holes in the interrupt fabric 😾
    // 0,2,4,6,8,10,12,14-15 - treat like forbidden catnip
    
    // =^..^=  Exceptions (bit 31 clear)  =^..^=
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
    
    // Reserved territories (no napping allowed!)
    // 10,14,16-17,20-23,32-47,≥64 - like off-limit counters
    
    // Custom scratch posts (24-31, 48-63) 😻
};


}

#endif
