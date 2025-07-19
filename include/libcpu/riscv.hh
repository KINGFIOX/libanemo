#ifndef LIBCPU_RISCV_HH
#define LIBCPU_RISCV_HH

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace libcpu::riscv {

enum class priv_level_t {
    u = 0,
    h = 1,
    s = 2,
    m = 3,
};

/**
 * @brief Enumeration of RISC-V general-purpose register addresses.
 */
enum gpr_addr_t: uint8_t {
    X0  = 0,   ///< Hard-wired zero (x0). Always returns 0 when read.
    RA  = 1,   ///< Return address (x1). Stores return address for function calls.
    SP  = 2,   ///< Stack pointer (x2). Points to the top of the stack.
    GP  = 3,   ///< Global pointer (x3). Points to global data area.
    TP  = 4,   ///< Thread pointer (x4). Used for thread-local storage.
    T0  = 5,   ///< Temporary/alternate link register (x5).
    T1  = 6,   ///< Temporary register (x6).
    T2  = 7,   ///< Temporary register (x7).
    S0  = 8,   ///< Saved register/frame pointer (x8). Also called FP.
    S1  = 9,   ///< Saved register (x9). Preserved across function calls.
    A0  = 10,  ///< Function argument/return value (x10). First argument.
    A1  = 11,  ///< Function argument/return value (x11). Second argument.
    A2  = 12,  ///< Function argument (x12). Third argument.
    A3  = 13,  ///< Function argument (x13). Fourth argument.
    A4  = 14,  ///< Function argument (x14). Fifth argument.
    A5  = 15,  ///< Function argument (x15). Sixth argument.
    A6  = 16,  ///< Function argument (x16). Seventh argument.
    A7  = 17,  ///< Function argument (x17). Eighth argument (syscall number).
    S2  = 18,  ///< Saved register (x18).
    S3  = 19,  ///< Saved register (x19).
    S4  = 20,  ///< Saved register (x20).
    S5  = 21,  ///< Saved register (x21).
    S6  = 22,  ///< Saved register (x22).
    S7  = 23,  ///< Saved register (x23).
    S8  = 24,  ///< Saved register (x24).
    S9  = 25,  ///< Saved register (x25).
    S10 = 26,  ///< Saved register (x26).
    S11 = 27,  ///< Saved register (x27).
    T3  = 28,  ///< Temporary register (x28).
    T4  = 29,  ///< Temporary register (x29).
    T5  = 30,  ///< Temporary register (x30).
    T6  = 31   ///< Temporary register (x31).
};

inline constexpr const char *gpr_names[] = {
    "x0", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", 
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", 
    "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};
    
inline const char* gpr_name(uint8_t addr) {
    return gpr_names[addr];
}

inline int8_t gpr_addr(const char* name) {
    if (name[0] == 'x') {
        auto num = strtoul(name+1, nullptr, 10);
        return static_cast<uint8_t>(num);
    }
    for (uint8_t i=0; i<32; ++i) {
        if (strcmp(name,gpr_names[i]) == 0) {
            return i;
        }
    }
    return 0;
}

}

#endif
