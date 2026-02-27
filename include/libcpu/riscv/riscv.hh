#ifndef LIBCPU_RISCV_RISCV_HH
#define LIBCPU_RISCV_RISCV_HH

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <libvio/width.hh>

namespace libcpu::riscv {

enum class priv_level_t : uint8_t {
  u = 0,
  s = 1,
  h = 2,
  m = 3,
};

/**
 * @brief Enumeration of RISC-V general-purpose register addresses.
 */
enum gpr_addr_t : uint8_t {
  X0 = 0,
  RA = 1,
  SP = 2,
  GP = 3,
  TP = 4,
  T0 = 5,
  T1 = 6,
  T2 = 7,
  S0 = 8,
  S1 = 9,
  A0 = 10,
  A1 = 11,
  A2 = 12,
  A3 = 13,
  A4 = 14,
  A5 = 15,
  A6 = 16,
  A7 = 17,
  S2 = 18,
  S3 = 19,
  S4 = 20,
  S5 = 21,
  S6 = 22,
  S7 = 23,
  S8 = 24,
  S9 = 25,
  S10 = 26,
  S11 = 27,
  T3 = 28,
  T4 = 29,
  T5 = 30,
  T6 = 31
};

/**
 * @brief Array of general purpose register names
 *
 * Contains the ABI names for all 32 RISC-V general purpose registers.
 * Index corresponds to register number (x0-x31).
 */
inline constexpr const char *gpr_names[] = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

/**
 * @brief Get the name of a general purpose register
 * @param addr Register number (0-31)
 * @return Register name string
 */
inline constexpr const char *gpr_name(uint8_t addr) { return gpr_names[addr]; }

/**
 * @brief Get the address/number of a general purpose register by name
 * @param name Register name (either ABI name like "ra" or numeric like "x1")
 * @return Register number (0-31)
 *
 * @note Returns 0 if name is not found (matches x0 behavior)
 */
inline constexpr uint_fast8_t gpr_addr(const char *name) {
  if (name[0] == 'x') {
    auto num = strtoul(name + 1, nullptr, 10);
    return static_cast<uint8_t>(num);
  }
  for (uint8_t i = 0; i < 32; ++i) {
    if (strcmp(name, gpr_names[i]) == 0) {
      return i;
    }
  }
  return 0;
}

/**
 * @brief Control and Status Register (CSR) addresses
 *
 * Contains all standard RISC-V CSR addresses as static constants
 */
struct csr_addr {
  // Supervisor-Level CSRs
  static constexpr uint16_t sstatus = 0x100; ///< Supervisor status register
  static constexpr uint16_t sie =
      0x104; ///< Supervisor interrupt enable register
  static constexpr uint16_t stvec =
      0x105; ///< Supervisor trap handler base address
  static constexpr uint16_t scounteren =
      0x106; ///< Supervisor counter enable register
  static constexpr uint16_t senvcfg =
      0x10A; ///< Supervisor environment configuration register
  static constexpr uint16_t scountinhibit =
      0x120; ///< Supervisor counter inhibit register
  static constexpr uint16_t sscratch = 0x140; ///< Supervisor scratch register
  static constexpr uint16_t sepc =
      0x141; ///< Supervisor exception program counter
  static constexpr uint16_t scause = 0x142; ///< Supervisor trap cause register
  static constexpr uint16_t stval =
      0x143; ///< Supervisor bad address or instruction
  static constexpr uint16_t sip =
      0x144; ///< Supervisor interrupt pending register
  static constexpr uint16_t scountovf =
      0xDA0; ///< Supervisor counter overflow register
  static constexpr uint16_t satp =
      0x180; ///< Supervisor address translation and protection register
  static constexpr uint16_t scontext = 0x5A8; ///< Supervisor context register

  // Machine Information Registers
  static constexpr uint16_t mvendorid = 0xF11; ///< Vendor ID register
  static constexpr uint16_t marchid = 0xF12;   ///< Architecture ID register
  static constexpr uint16_t mimpid = 0xF13;    ///< Implementation ID register
  static constexpr uint16_t mhartid = 0xF14;   ///< Hardware thread ID register
  static constexpr uint16_t mconfigptr =
      0xF15; ///< Machine configuration pointer register

  // Machine Trap Setup
  static constexpr uint16_t mstatus = 0x300; ///< Machine status register
  static constexpr uint16_t misa = 0x301;    ///< ISA and extensions register
  static constexpr uint16_t medeleg =
      0x302; ///< Machine exception delegation register
  static constexpr uint16_t mideleg =
      0x303; ///< Machine interrupt delegation register
  static constexpr uint16_t mie = 0x304; ///< Machine interrupt enable register
  static constexpr uint16_t mtvec =
      0x305; ///< Machine trap handler base address
  static constexpr uint16_t mcounteren =
      0x306; ///< Machine counter enable register
  static constexpr uint16_t mstatush =
      0x310; ///< Additional machine status (RV32 only)
  static constexpr uint16_t medeleg_h =
      0x312; ///< Upper 32 bits of medeleg (RV32 only)

  // Machine Trap Handling
  static constexpr uint16_t mscratch = 0x340; ///< Machine scratch register
  static constexpr uint16_t mepc = 0x341; ///< Machine exception program counter
  static constexpr uint16_t mcause = 0x342; ///< Machine trap cause register
  static constexpr uint16_t mtval =
      0x343;                             ///< Machine bad address or instruction
  static constexpr uint16_t mip = 0x344; ///< Machine interrupt pending register
  static constexpr uint16_t mtinst =
      0x34A; ///< Machine trap instruction register
  static constexpr uint16_t mtval2 =
      0x34B; ///< Machine bad guest physical address
};

/**
 * @brief mcause register bit definitions
 * @tparam WORD_T Word type (uint32_t or uint64_t)
 */
template <typename WORD_T> struct mcause {
  static constexpr WORD_T intr_mask =
      WORD_T(1) << (sizeof(WORD_T) * CHAR_BIT - 1); ///< Interrupt mask bit
  static constexpr WORD_T intr_s_software =
      1 | intr_mask; ///< Supervisor software interrupt
  static constexpr WORD_T intr_m_software =
      3 | intr_mask; ///< Machine software interrupt
  static constexpr WORD_T intr_s_timer =
      5 | intr_mask; ///< Supervisor timer interrupt
  static constexpr WORD_T intr_m_timer =
      7 | intr_mask; ///< Machine timer interrupt
  static constexpr WORD_T intr_s_external =
      9 | intr_mask; ///< Supervisor external interrupt
  static constexpr WORD_T intr_m_external =
      11 | intr_mask; ///< Machine external interrupt
  static constexpr WORD_T intr_cnt_overflow =
      13 | intr_mask; ///< Counter overflow interrupt

  static constexpr WORD_T except_instr_misalign =
      0; ///< Instruction address misaligned
  static constexpr WORD_T except_instr_fault = 1; ///< Instruction access fault
  static constexpr WORD_T except_illegal_instr = 2; ///< Illegal instruction
  static constexpr WORD_T except_breakpoint = 3;    ///< Breakpoint
  static constexpr WORD_T except_load_misalign = 4; ///< Load address misaligned
  static constexpr WORD_T except_load_fault = 5;    ///< Load access fault
  static constexpr WORD_T except_store_misalign =
      6;                                          ///< Store address misaligned
  static constexpr WORD_T except_store_fault = 7; ///< Store access fault
  static constexpr WORD_T except_env_call_u =
      8; ///< Environment call from U-mode
  static constexpr WORD_T except_env_call_s =
      9; ///< Environment call from S-mode
  static constexpr WORD_T except_env_call_m =
      11; ///< Environment call from M-mode
  static constexpr WORD_T except_instr_page_fault =
      12; ///< Instruction page fault
  static constexpr WORD_T except_load_page_fault = 13;  ///< Load page fault
  static constexpr WORD_T except_store_page_fault = 15; ///< Store page fault
  static constexpr WORD_T except_software_check =
      18; ///< Software check failure
  static constexpr WORD_T except_hardware_error = 19; ///< Hardware error
};

/**
 * @brief mstatus register bit definitions for both rv32 and rv64
 * @tparam WORD_T Word type (uint32_t or uint64_t)
 */
template <typename WORD_T> struct mstatus_common {
  static constexpr WORD_T sie = WORD_T(1) << 1; ///< Supervisor interrupt enable
  static constexpr WORD_T mie = WORD_T(1) << 3; ///< Machine interrupt enable
  static constexpr WORD_T spie = WORD_T(1)
                                 << 5; ///< Previous supervisor interrupt enable
  static constexpr WORD_T ube = WORD_T(1)
                                << 6; ///< User-mode endianness (1=big-endian)
  static constexpr WORD_T mpie = WORD_T(1)
                                 << 7; ///< Previous machine interrupt enable
  static constexpr WORD_T spp = WORD_T(1)
                                << 8; ///< Supervisor previous privilege mode
  static constexpr WORD_T mpp = WORD_T(3) << 11; ///< Previous privilege mode
  static constexpr WORD_T mppl = WORD_T(1)
                                 << 11; ///< Previous privilege mode (low bit)
  static constexpr WORD_T mpph = WORD_T(1)
                                 << 12; ///< Previous privilege mode (high bit)
  static constexpr WORD_T fs = WORD_T(3) << 12; ///< Floating-point unit status
  static constexpr WORD_T fs0 = WORD_T(1)
                                << 12; ///< Floating-point unit status (bit 0)
  static constexpr WORD_T fs1 = WORD_T(1)
                                << 13; ///< Floating-point unit status (bit 1)
  static constexpr WORD_T xs = WORD_T(3) << 14;  ///< Extension status
  static constexpr WORD_T xsl = WORD_T(1) << 14; ///< Extension status (bit 0)
  static constexpr WORD_T xsh = WORD_T(1) << 15; ///< Extension status (bit 1)
  static constexpr WORD_T vs = WORD_T(3) << 16;  ///< Vector extension status
  static constexpr WORD_T vsl = WORD_T(1)
                                << 16; ///< Vector extension status (bit 0)
  static constexpr WORD_T vsh = WORD_T(1)
                                << 17; ///< Vector extension status (bit 1)
  static constexpr WORD_T mprv =
      WORD_T(1) << 17; ///< Modify privilege for memory accesses
  static constexpr WORD_T sum = WORD_T(1)
                                << 18; ///< Permit supervisor user memory access
  static constexpr WORD_T mxr = WORD_T(1)
                                << 19; ///< Make executable pages readable
  static constexpr WORD_T tvm = WORD_T(1)
                                << 20; ///< Trap virtual memory operations
  static constexpr WORD_T tw = WORD_T(1)
                               << 21; ///< Timeout wait for WFI instruction
  static constexpr WORD_T tsr = WORD_T(1) << 22; ///< Trap SRET instruction
  static constexpr WORD_T sd =
      WORD_T(1) << (sizeof(WORD_T) * CHAR_BIT - 1); ///< State dirty flag
};

template <typename WORD_T> struct mstatus : mstatus_common<WORD_T> {};

template <> struct mstatus<uint32_t> : mstatus_common<uint32_t> {};

/**
 * @brief mstatus register specialization for 64-bit
 */
template <> struct mstatus<uint64_t> : mstatus_common<uint64_t> {
  static constexpr uint64_t uxl = uint64_t(3) << 32;  ///< User XLEN
  static constexpr uint64_t uxll = uint64_t(1) << 32; ///< User XLEN low bit
  static constexpr uint64_t uxlh = uint64_t(1) << 33; ///< User XLEN high bit
  static constexpr uint64_t sxl = uint64_t(3) << 34;  ///< Supervisor XLEN
  static constexpr uint64_t sxll = uint64_t(1)
                                   << 34; ///< Supervisor XLEN low bit
  static constexpr uint64_t sxlh = uint64_t(1)
                                   << 35; ///< Supervisor XLEN high bit
  static constexpr uint64_t sbe = uint64_t(1)
                                  << 36; ///< Supervisor endianness bit
  static constexpr uint64_t mbe = uint64_t(1) << 37; ///< Machine endianness bit
};

/**
 * @brief mstatush register bit definitions (RV32 only)
 */
struct mstatush {
  static constexpr uint32_t sbe = uint32_t(1)
                                  << 4; ///< Supervisor endianness bit
  static constexpr uint32_t mbe = uint32_t(1) << 5; ///< Machine endianness bit
};

/**
 * @brief sstatus register bit definitions
 * @tparam WORD_T Word type (uint32_t or uint64_t)
 */
template <typename WORD_T> struct sstatus {
  static constexpr WORD_T sie = WORD_T(1) << 1; ///< Supervisor interrupt enable
  static constexpr WORD_T spie = WORD_T(1)
                                 << 5; ///< Previous supervisor interrupt enable
  static constexpr WORD_T ube = WORD_T(1)
                                << 6; ///< User-mode endianness (1=big-endian)
  static constexpr WORD_T spp = WORD_T(1)
                                << 8; ///< Previous privilege mode (S=1, U=0)
  static constexpr WORD_T vsl = WORD_T(1) << 9;  ///< Vector unit status
  static constexpr WORD_T vs = WORD_T(3) << 10;  ///< Vector unit status
  static constexpr WORD_T vsh = WORD_T(1) << 10; ///< Vector unit status
  static constexpr WORD_T fs = WORD_T(3) << 13;  ///< Floating-point unit status
  static constexpr WORD_T fsl = WORD_T(1) << 13; ///< Floating-point unit status
  static constexpr WORD_T fsh = WORD_T(1) << 14; ///< Floating-point unit status
  static constexpr WORD_T xs = WORD_T(3) << 15;  ///< Extension status
  static constexpr WORD_T xsl = WORD_T(1) << 15; ///< Extension status
  static constexpr WORD_T xsh = WORD_T(1) << 16; ///< Extension status
  static constexpr WORD_T sum = WORD_T(1)
                                << 18; ///< Permit supervisor user memory access
  static constexpr WORD_T mxr = WORD_T(1)
                                << 19; ///< Make executable pages readable
  static constexpr WORD_T sd =
      WORD_T(1) << (sizeof(WORD_T) * CHAR_BIT - 1); ///< State dirty flag
};

/**
 * @brief mtvec register bit definitions
 * @tparam WORD_T Word type (uint32_t or uint64_t)
 */
template <typename WORD_T> struct mtvec {
  static constexpr WORD_T vectored =
      1; ///< Trap mode: vectored (1) or direct (0)
};

template <typename WORD_T> using stvec = mtvec<WORD_T>;

/**
 * @brief mip register bit definitions
 * @tparam WORD_T Word type (uint32_t or uint64_t)
 */
template <typename WORD_T> struct mip {
  static constexpr WORD_T ssip =
      1 << 1; ///< Supervisor software interrupt pending
  static constexpr WORD_T msip = 1 << 3; ///< Machine software interrupt pending
  static constexpr WORD_T stip = 1 << 5; ///< Supervisor timer interrupt pending
  static constexpr WORD_T mtip = 1 << 7; ///< Machine timer interrupt pending
  static constexpr WORD_T seip =
      1 << 9; ///< Supervisor external interrupt pending
  static constexpr WORD_T meip = 1
                                 << 11; ///< Machine external interrupt pending
  static constexpr WORD_T lcofip =
      1 << 13; ///< Local counter overflow interrupt pending
};

template <typename WORD_T> using stip = mip<WORD_T>;

/**
 * @brief mie register bit definitions
 * @tparam WORD_T Word type (uint32_t or uint64_t)
 */
template <typename WORD_T> struct mie {
  static constexpr WORD_T ssie = 1
                                 << 1; ///< Supervisor software interrupt enable
  static constexpr WORD_T msie = 1 << 3; ///< Machine software interrupt enable
  static constexpr WORD_T stie = 1 << 5; ///< Supervisor timer interrupt enable
  static constexpr WORD_T mtie = 1 << 7; ///< Machine timer interrupt enable
  static constexpr WORD_T seie = 1
                                 << 9; ///< Supervisor external interrupt enable
  static constexpr WORD_T meie = 1 << 11; ///< Machine external interrupt enable
  static constexpr WORD_T lcofie =
      1 << 13; ///< Local counter overflow interrupt enable
};

template <typename WORD_T> using stie = mie<WORD_T>;

/**
 * @brief Enumeration of dispatchable instruction types
 */
enum class dispatch_t : uint8_t {
  // Arithmetic & Logical
  add,
  sub,
  sll,
  slt,
  sltu,
  xor_,
  srl,
  sra,
  or_,
  and_,
  // Immediate Operations
  addi,
  slti,
  sltiu,
  xori,
  ori,
  andi,
  slli,
  srli,
  srai,
  // Memory Operations
  lb,
  lh,
  lw,
  lbu,
  lhu,
  sb,
  sh,
  sw,
  // Control Flow
  jal,
  jalr,
  beq,
  bne,
  blt,
  bge,
  bltu,
  bgeu,
  // Upper Immediate
  lui,
  auipc,
  // Multiply/Divide
  mul,
  mulh,
  mulhsu,
  mulhu,
  div,
  divu,
  rem,
  remu,
  // System
  ecall,
  ebreak,
  mret,
  sret,
  // RV64 specific instructions
  lwu,
  ld,
  sd,
  addiw,
  slliw,
  srliw,
  sraiw,
  addw,
  subw,
  sllw,
  srlw,
  sraw,
  mulw,
  divw,
  divuw,
  remw,
  remuw,
  // CSR functions
  csrrw,
  csrrs,
  csrrc,
  csrrwi,
  csrrsi,
  csrrci,
  // Invalid instruction
  invalid,
};

/**
 * @brief Execution result types
 */
enum class exec_result_type_t : uint8_t {
  fetch,
  decode,
  retire, ///< Committed or trap handled
  load,
  store,
  trap,
  sys_op,
  csr_op
};

/**
 * @brief A decoded RISC-V instruction
 */
struct decode_t {
  int32_t imm;
  dispatch_t dispatch;
  uint8_t rs1;
  uint8_t rs2;
  uint8_t rd;
};

template <typename WORD_T>
/**
 * @brief Execution result structure
 *
 * Describes modifications to unprivileged architectural state or operation
 * details for privileged operations. The user core handles only unprivileged
 * operations; privileged operations must be implemented by dedicated modules.
 *
 * Note: Memory operations are inherently privileged as they may involve address
 * translation and physical memory protection. For flexibility, these operations
 * should be implemented by separate modules. The user core is not responsible
 * for handling MMIO, address translation, or similar memory-related functions.
 *
 * Modifications to privileged architectural state must be implemented by the
 * privileged module. This clear separation between privileged and unprivileged
 * components improves performance and enables code reuse.
 */
struct exec_result_t {
  exec_result_type_t type; ///< Type of execution result
  WORD_T pc;               ///< PC of this instruction
  WORD_T next_pc;          ///< Expected PC of the next instruction
  uint32_t instr;
  union {
    decode_t decode;
    struct {
      uint8_t rd;
      WORD_T value;
    } retire;
    struct {
      WORD_T addr;
      libvio::width_t width;
      bool sign_extend;
      uint8_t rd;
    } load;
    struct {
      WORD_T addr;
      libvio::width_t width;
      WORD_T data;
    } store;
    struct {
      WORD_T cause;
      WORD_T tval;
    } trap;
    struct {
      bool ecall;
      bool mret;
      bool sret;
    } sys_op;
    struct {
      uint16_t addr;
      uint8_t rd;
      bool read;
      bool write;
      bool set;
      bool clear;
      WORD_T value;
    } csr_op;
  };
};

/**
 * @brief RISC-V address translation modes.
 */
enum class satp_mode_t : uint8_t {
  bare = 0,
  sv32 = 1,
  sv39 = 8,
  sv48 = 9,
  sv57 = 10,
  sv64 = 11
};

} // namespace libcpu::riscv

#endif
