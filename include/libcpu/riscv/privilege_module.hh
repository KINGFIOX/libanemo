#ifndef LIBCPU_RISCV_PRIVILEGE_MODULE_HH
#define LIBCPU_RISCV_PRIVILEGE_MODULE_HH

#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <libcpu/memory.hh>
#include <libcpu/riscv/riscv.hh>
#include <libcpu/riscv/user_core.hh>
#include <libvio/bus.hh>
#include <optional>

namespace libcpu::riscv {

/**
 * @brief RISC-V Privilege Module
 * 
 * This class implements the RISC-V privilege architecture as defined in the
 * RISC-V Privileged Specification. It manages:
 * - Privilege levels (User, Supervisor, Machine)
 * - Control/Status Registers (CSRs)
 * - Address translation (via SATP register)
 * - Exception and interrupt handling
 * - Privilege transitions
 * 
 * The module works in conjunction with the user core to handle all privileged
 * operations while maintaining separation of concerns.
 * 
 * @tparam WORD_T The word type for the architecture (uint32_t for RV32, uint64_t for RV64)
 * 
 * @note All CSR accesses and privilege transitions must go through this module
 * @note Memory operations are delegated to this module
 * @see riscv_user_core for the unprivileged counterpart
 */
template <typename WORD_T>
class privilege_module {
    public:
        using dispatch_t  = riscv::dispatch_t;
        using decode_t = riscv::decode_t;
        using exec_result_type_t = riscv::exec_result_type_t;
        using exec_result_t = riscv::exec_result_t<WORD_T>;
        using satp_mode_t = riscv::satp_mode_t;

        priv_level_t priv_level;
        WORD_T mepc, mtvec, mcause, mtval, mscratch, mie, mip, medeleg, mideleg;
        WORD_T sepc, stvec, scause, stval, sscratch, sie, sip;

        memory_view *instr_bus;
        memory_view *data_bus;
        libvio::io_agent *mmio_bus;

        struct {
            priv_level_t mpp;
            bool spp; bool mpie; bool spie; bool mie; bool sie;
        } status;

        struct {
            satp_mode_t mode;
            uint16_t asid;
            WORD_T ppn;
        } satp;

        /**
         * @brief Translate virtual address to physical address
         * 
         * Performs address translation based on current privilege level and SATP register.
         * This function returns a 64-bit address regardless of WORD_T since the physical address
         * in sv32 is also wider than 32 bits.
         * 
         * @param vaddr Virtual address to translate
         * @return Physical address if translation succeeds, empty if fails
         */
        std::optional<uint64_t> vaddr_to_paddr(WORD_T vaddr) const;

        /**
         * @brief Fetch instruction using physical address
         * 
         * This function is designed for simulating processors without virtual memory.
         * `op` will be populated with the `fetch` variant if successful, or `trap` if not.
         * 
         * @param op Execution result structure to populate with fetch details
         */
        void paddr_fetch_instruction(exec_result_t &op) const;

        /**
         * @brief Fetch instruction using virtual address
         * 
         * Handles instruction fetch with virtual address translation.
         * `op` will be populated with the `fetch` variant if successful, or `trap` if not.
         * 
         * @param op Execution result structure to populate with fetch details
         */
        void vaddr_fetch_instruction(exec_result_t &op) const;

        /**
         * @brief Perform physical address load operation
         * 
         * This function is designed for simulating processors without virtual memory.
         * `op` will be populated with the `retire` variant if successful, or the `trap` variant if not.
         * 
         * @param op Execution result structure containing load details
         */
        void paddr_load(exec_result_t &op);

        /**
         * @brief Perform physical address store operation
         * 
         * This function is designed for simulating processors without virtual memory.
         * `op` will be populated with the `retire` variant if successful, or the `trap` variant if not.
         * 
         * @param op Execution result structure containing store details
         */
        void paddr_store(exec_result_t &op);

        /**
         * @brief Perform virtual address load operation
         * 
         * Handles memory load operations using virtual addresses, including full address translation
         * and privilege checks according to the current memory management configuration.
         * `op` will be populated with the `retire` variant if successful, or the `trap` variant
         * if translation fails or access privileges are violated.
         * 
         * @param op Execution result structure containing load details
         */
        void vaddr_load(exec_result_t &op);

        /**
         * @brief Perform virtual address store operation
         * 
         * Handles memory store operations using virtual addresses, including full address translation
         * and privilege checks according to the current memory management configuration.
         * `op` will be populated with the `retire` variant if successful, or the `trap` variant
         * if translation fails or access privileges are violated.
         * 
         * @param op Execution result structure containing store details
         */
        void vaddr_store(exec_result_t &op);

        /**
         * @brief Reset privilege module
         * 
         * Initializes all privilege-related registers and state to their reset values.
         */
        void reset(void);

        /**
         * @brief Raise an interrupt
         * 
         * Records interrupt cause and updates interrupt pending bits.
         * 
         * @param cause Interrupt cause code
         */
        void raise_interrupt(WORD_T cause);

        /**
         * @brief Handle exception
         * 
         * Processes exceptions by updating CSRs and potentially changing privilege level.
         * `op` will be populated with the `retire` varient, and `op.next_pc` will be changed
         * if there is an exception to handle.
         * 
         * @param op Execution result structure containing exception details
         */
        void handle_exception(exec_result_t &op);

        /**
         * @brief Handle interrupt
         * 
         * Processes interrupts by updating CSRs and potentially changing privilege level.
         * `op` will be populated with the `retire` varient, and `op.next_pc` will be changed
         * if there is an interrupt to handle.
         * 
         * @param op Execution result structure containing interrupt details
         */
        void handle_interrupt(exec_result_t &op);

        /**
         * @brief Perform CSR operation
         * 
         * Executes CSR read/write/set/clear operations with proper privilege checks.
         * `op` will be populated with the `retire` varient if successful, or the `trap` varient if not.
         * 
         * @param op Execution result structure containing CSR operation details
         */
        void csr_op(exec_result_t &op);

        /**
         * @brief Handle ecall, mret, sret instructions
         * 
         * @param op Execution result structure containing execution details
         */
        void sys_op(exec_result_t &op);
};

template <typename WORD_T>
void privilege_module<WORD_T>::reset(void) {
    priv_level = priv_level_t::m;
    mepc = 0; sepc = 0;
    mtvec = 0; stvec = 0;
    mcause = 0; scause = 0;
    mtval = 0; stval = 0;
    mscratch = 0; sscratch = 0;
    medeleg = 0; mideleg = 0;
    mie = 0; sie = 0;
    mip = 0; sip = 0;
    status = {
        .mpp=priv_level_t::m, .spp=false,
        .mpie=false, .spie=false,
        .mie=false, .sie=false,
    };
    satp = {
        .mode=satp_mode_t::bare,
        .asid=0, .ppn=0,
    };
}

template <typename WORD_T>
std::optional<uint64_t> privilege_module<WORD_T>::vaddr_to_paddr(WORD_T vaddr) const {
    if (priv_level == priv_level_t::m) {
        return vaddr;
    } else if (priv_level == priv_level_t::s) {
        return vaddr;
    } else if (priv_level == priv_level_t::u) {
        return vaddr;
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::paddr_fetch_instruction(exec_result_t &op) const {
    WORD_T paddr = op.pc;
    std::optional<uint32_t> instr_opt = instr_bus->read(paddr, libvio::width_t::word);
    if (instr_opt.has_value()) {
        op.type = exec_result_type_t::fetch;
        op.instr = instr_opt.value();
    } else {
        op.type = exec_result_type_t::trap;
        op.trap = {
            .cause = riscv::mcause<WORD_T>::except_instr_fault,
            .tval = paddr,
        };
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::vaddr_fetch_instruction(exec_result_t &op) const {
    WORD_T vaddr = op.pc;
    std::optional<WORD_T> paddr_opt = vaddr_to_paddr(op.load.addr);
    if (paddr_opt.has_value()) {
        WORD_T paddr = paddr_opt.value();
        std::optional<uint32_t> instr_opt = instr_bus->read(paddr, libvio::width_t::word);
        if (instr_opt.has_value()) {
            op.type = exec_result_type_t::fetch;
            op.instr = instr_opt.value();
        } else {
            op.type = exec_result_type_t::trap;
            op.trap = {
                .cause = riscv::mcause<WORD_T>::except_instr_fault,
                .tval = vaddr,
            };
        }
    } else {
        op.type = exec_result_type_t::trap;
        op.trap = {
            .cause = riscv::mcause<WORD_T>::except_instr_page_fault,
            .tval = vaddr,
        };
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::paddr_load(exec_result_t &op) {
    assert(op.type == exec_result_type_t::load);
    auto [paddr, width, sign_extend, rd] = op.load;
    std::optional<uint64_t> data_opt = data_bus->read(paddr, width);
    // fall back to MMIO if the address is out of RAM
    if (!data_opt.has_value() && mmio_bus!=nullptr) {
        data_opt = mmio_bus->read(paddr, width);
    }
    if (data_opt.has_value()) {
        // `data` is zero extended
        WORD_T data = data_opt.value();
        if (sign_extend) {
            data = libvio::sign_extend<WORD_T>(data, width);
        }
        op.type = exec_result_type_t::retire;
        op.retire = {
            .rd=rd, .value=data,
        };
    } else {
        op.type = exec_result_type_t::trap;
        op.trap = {
            .cause = riscv::mcause<WORD_T>::except_load_fault,
            .tval = paddr,
        };
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::paddr_store(exec_result_t &op) {
    assert(op.type == exec_result_type_t::store);
    auto [paddr, width, data] = op.store;
    bool success = data_bus->write(paddr, width, data);
    // fall back to MMIO
    if (!success && mmio_bus!=nullptr) {
        success = mmio_bus->write(paddr, width, data);
    }
    if (success) {
        op.type = exec_result_type_t::retire;
        op.retire = {
            .rd=0, .value=0,
        };
    } else {
        // both RAM and MMIO failed
        op.type = exec_result_type_t::trap;
        op.trap = {
            .cause = riscv::mcause<WORD_T>::except_store_fault,
            .tval = paddr,
        };
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::vaddr_load(exec_result_t &op) {
    assert(op.type == exec_result_type_t::load);
    auto [vaddr, width, sign_extend, rd] = op.load;
    std::optional<uint64_t> paddr_opt = vaddr_to_paddr(op.load.addr);
    if (paddr_opt.has_value()) {
        uint64_t paddr = paddr_opt.value();
        std::optional<uint64_t> data_opt = data_bus->read(paddr, width);
        // fall back to MMIO if the address is out of RAM
        if (!data_opt.has_value() && mmio_bus!=nullptr) {
            data_opt = mmio_bus->read(paddr, width);
        }
        if (data_opt.has_value()) {
            // `data` is zero extended
            WORD_T data = data_opt.value();
            if (sign_extend) {
                data = libvio::sign_extend<WORD_T>(data, width);
            }
            op.type = exec_result_type_t::retire;
            op.retire = {
                .rd=rd, .value=data,
            };
        } else {
            op.type = exec_result_type_t::trap;
            op.trap = {
                .cause = riscv::mcause<WORD_T>::except_load_fault,
                .tval = vaddr,
            };
        }
    } else {
        op.type = exec_result_type_t::trap;
        op.trap = {
            .cause = riscv::mcause<WORD_T>::except_load_page_fault,
            .tval = vaddr,
        };
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::vaddr_store(exec_result_t &op) {
    assert(op.type == exec_result_type_t::store);
    auto [vaddr, width, data] = op.store;
    std::optional<uint64_t> paddr_opt = vaddr_to_paddr(vaddr);
    if (paddr_opt.has_value()) {
        uint64_t paddr = paddr_opt.value();
        bool success = data_bus->write(paddr, width, data);
        // fall back to MMIO
        if (!success && mmio_bus!=nullptr) {
            success = mmio_bus->write(paddr, width, data);
        }
        if (success) {
            op.type = exec_result_type_t::retire;
            op.retire = {
                .rd=0, .value=0,
            };
        } else {
            // both RAM and MMIO failed
            op.type = exec_result_type_t::trap;
            op.trap = {
                .cause = riscv::mcause<WORD_T>::except_store_fault,
                .tval = vaddr,
            };
        }
    } else {
        op.type = exec_result_type_t::trap;
        op.trap = {
            .cause = riscv::mcause<WORD_T>::except_store_page_fault,
            .tval = vaddr,
        };
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::raise_interrupt(WORD_T cause) {
    WORD_T cause_mask = 1 << (cause & ~riscv::mcause<WORD_T>::intr_mask);
    if (mideleg & cause_mask) {
        sip |= cause_mask;
    } else {
        mip |= cause_mask;
    }
}

template <typename WORD_T>
void privilege_module<WORD_T>::handle_interrupt(exec_result_t &op) {
    constexpr size_t word_size = sizeof(WORD_T) * CHAR_BIT;

    assert(op.type == exec_result_type_t::retire);
    WORD_T pc = op.next_pc;
    priv_level_t target_priv_level;
    WORD_T cause;

    if ((mie&mip) && (status.mie||priv_level!=priv_level_t::m)) {
        for (size_t i=0; i<word_size; ++i) {
            if (((mie&mip)>>i)&1) {
                target_priv_level = priv_level_t::m;
                cause = i;
                break;
            }
        }
    } else if ((sie&sip) && priv_level!=priv_level_t::m && (status.sie||priv_level==priv_level_t::u)) {
        for (size_t i=0; i<word_size; ++i) {
            if (((sie&sip)>>i)&1) {
                target_priv_level = priv_level_t::s;
                cause = i;
                break;
            }
        }
    } else {
        return;
    }

    WORD_T vector_base;
    bool is_vectord;
    if (target_priv_level == priv_level_t::m) {
        vector_base = mtvec & ~riscv::mtvec<WORD_T>::vectored;
        is_vectord = mtvec & riscv::mtvec<WORD_T>::vectored;
    } else {
        vector_base = stvec & ~riscv::mtvec<WORD_T>::vectored;
        is_vectord = stvec & riscv::mtvec<WORD_T>::vectored;
    }
    WORD_T target_addr;
    if (is_vectord) {
        target_addr = vector_base + 4*cause;
    } else {
        target_addr = vector_base;
    }

    if (target_priv_level == priv_level_t::m) {
        mcause = cause | riscv::mcause<WORD_T>::intr_mask;
        mtval = 0;
        mepc = pc;
        status.mpp = priv_level;
        status.mpie = status.mie;
        status.mie = false;
    } else {
        assert(priv_level!=priv_level_t::m);
        scause = cause | riscv::mcause<WORD_T>::intr_mask;
        stval = 0;
        sepc = pc;
        status.spp = priv_level==priv_level_t::s;
        status.spie = status.sie;
        status.sie = false;
    }
    priv_level = target_priv_level;

    op.next_pc = target_addr;
}

template <typename WORD_T>
void privilege_module<WORD_T>::handle_exception(exec_result_t &op) {
    assert(op.type == exec_result_type_t::trap);
    WORD_T pc = op.pc;
    WORD_T cause = op.trap.cause;
    WORD_T tval = op.trap.tval;
    WORD_T trap_no = cause & ~riscv::mcause<WORD_T>::intr_mask;

    priv_level_t target_priv_level;
    if (priv_level!=priv_level_t::m && (medeleg & (1 << cause))) {
        target_priv_level = priv_level_t::s;
    } else {
        target_priv_level = priv_level_t::m;
    }

    WORD_T target_addr;
    if (target_priv_level == priv_level_t::m) {
        target_addr = mtvec & ~riscv::mtvec<WORD_T>::vectored;
    } else {
        target_addr = stvec & ~riscv::mtvec<WORD_T>::vectored;
    }

    if (target_priv_level == priv_level_t::m) {
        mcause = cause;
        mtval = tval;
        mepc = pc;
        status.mpp = priv_level;
        status.mpie = status.mie;
        status.mie = false;
    } else {
        assert(priv_level!=priv_level_t::m);
        scause = cause;
        stval = tval;
        sepc = pc;
        status.spp = priv_level==priv_level_t::s;
        status.spie = status.sie;
        status.sie = false;
    }
    priv_level = target_priv_level;

    op.type = exec_result_type_t::retire;
    op.next_pc = target_addr;
    op.retire.rd = 0;
}

#define CSRRCS(name) \
    case riscv::csr_addr::name: { \
        op.retire.value = name; \
        if (write) { \
            name = value; \
        } else if (set) { \
            name |= value; \
        } else if (clear) { \
            name &= ~value; \
        } \
        break; \
    }

#define STATUSRCS(csr_name, bit_name) \
    if (status.bit_name) { \
        op.retire.value |= riscv::csr_name<WORD_T>::bit_name; \
    } \
    if (write) { \
        status.bit_name = value & riscv::csr_name<WORD_T>::bit_name; \
    } else if (set) { \
        status.bit_name = value&riscv::csr_name<WORD_T>::bit_name ? true : status.bit_name; \
    } else if (clear) { \
        status.bit_name = value&riscv::csr_name<WORD_T>::bit_name ? false : status.bit_name; \
    }

template <typename WORD_T>
void privilege_module<WORD_T>::csr_op(exec_result_t &op) {
    constexpr bool is_rv64 = sizeof(WORD_T) * CHAR_BIT == 64;
    assert(op.type == exec_result_type_t::csr_op);

    auto [addr, rd, read, write, set, clear, value] = op.csr_op;
    bool read_access = static_cast<int>(priv_level) >= (addr>>8 & 0x3);
    if (!read_access && read) {
        op.type = exec_result_type_t::trap;
        op.trap.cause = riscv::mcause<WORD_T>::except_illegal_instr;
        op.trap.tval = op.instr;
    }
    bool write_access = read_access && (addr>>10)!=0x3;
    if (!write_access && (write||set||clear)) {
        op.type = exec_result_type_t::trap;
        op.trap.cause = riscv::mcause<WORD_T>::except_illegal_instr;
        op.trap.tval = op.instr;
    }

    op.type = exec_result_type_t::retire;
    op.retire.rd = rd;
    op.retire.value = 0;
    switch (addr) {
        case riscv::csr_addr::misa: {
            if constexpr (is_rv64) {
                op.retire.value = uint64_t(2)<<62 | 0x101100;
            } else {
                op.retire.value = 0x40101100;
            }
            break;
        }
        CSRRCS(mepc);
        CSRRCS(sepc);
        CSRRCS(mtvec);
        CSRRCS(stvec);
        CSRRCS(mcause);
        CSRRCS(scause);
        CSRRCS(mtval);
        CSRRCS(stval);
        CSRRCS(mscratch);
        CSRRCS(sscratch);
        CSRRCS(medeleg);
        CSRRCS(mideleg);
        CSRRCS(mie);
        CSRRCS(sie);
        CSRRCS(mip);
        CSRRCS(sip);
        case riscv::csr_addr::mstatus: {
            WORD_T mpp = static_cast<WORD_T>(status.mpp);
            op.retire.value |= mpp << 11;
            if (write) {
                mpp = (value>>11) & 3;
            } else if (set) {
                mpp |= (value>>11) & 3;
            } else if (clear) {
                mpp &= ~((value>>11) & 3);
            }
            status.mpp = static_cast<priv_level_t>(mpp);
            // prevent invalid values
            status.mpp = status.mpp!=priv_level_t::u && status.mpp!=priv_level_t::s ? priv_level_t::m : status.mpp;
            STATUSRCS(mstatus, spp);
            STATUSRCS(mstatus, mpie);
            STATUSRCS(mstatus, spie);
            STATUSRCS(mstatus, mie);
            STATUSRCS(mstatus, sie);
            break;
        }
        case riscv::csr_addr::sstatus: {
            STATUSRCS(sstatus, spp);
            STATUSRCS(sstatus, spie);
            STATUSRCS(sstatus, sie);
            break;
        }
    }
}

#undef CSRRCS
#undef STATUSRCS

template <typename WORD_T>
void privilege_module<WORD_T>::sys_op(exec_result_t &op) {
    assert(op.type == exec_result_type_t::sys_op);
    if (op.sys_op.ecall) {
        op.type = exec_result_type_t::trap;
        if (priv_level == priv_level_t::u) {
            op.trap.cause = riscv::mcause<WORD_T>::except_env_call_u;
        } else if (priv_level == priv_level_t::s) {
            op.trap.cause = riscv::mcause<WORD_T>::except_env_call_s;
        } else if (priv_level == priv_level_t::m) {
            op.trap.cause = riscv::mcause<WORD_T>::except_env_call_m;
        }
        op.trap.tval = 0;
    } else if (op.sys_op.mret) {
        if (priv_level != priv_level_t::m) {
            op.type = exec_result_type_t::trap;
            op.trap = {.cause=riscv::mcause<WORD_T>::except_illegal_instr, .tval=op.instr};  
        } else {
            priv_level = status.mpp;
            status.mie = status.mpie;
            status.mpie = 1;
            status.mpp = priv_level_t::u;
            op.type = exec_result_type_t::retire;
            op.retire.rd = 0;
            op.next_pc = mepc;
        }
    } else if (op.sys_op.sret) {
        if (priv_level == priv_level_t::u) {
            op.type = exec_result_type_t::trap;
            op.trap = {.cause=riscv::mcause<WORD_T>::except_illegal_instr, .tval=op.instr};  
        } else {
            priv_level = status.spp ? priv_level_t::s : priv_level_t::u;
            status.sie = status.spie;
            status.spie = 1;
            status.spp = 0;
            op.type = exec_result_type_t::retire;
            op.retire.rd = 0;
            op.next_pc = sepc;
        }
    }
}

}

#endif
