#ifndef LIBCPU_ABSTRACT_CPU_HH
#define LIBCPU_ABSTRACT_CPU_HH

#include <cstddef>
#include <cstdint>
#include <libcpu/event.hh>
#include <libcpu/memory.hh>
#include <libvio/bus.hh>
#include <libvio/frontend.hh>
#include <libvio/ringbuffer.hh>

namespace libcpu {

/**
 * @brief Abstract base class for CPU simulator interface.
 * 
 * @tparam WORD_T The word type for the CPU (typically uint32_t or uint64_t)
 * 
 * This class provides the interface for a CPU simulator, including
 * methods for accessing registers, memory, and controlling execution.
 * It also maintains an event buffer for tracing CPU activity.
 */
template <typename WORD_T>
class abstract_cpu {
    public:
        using word_t = WORD_T;              ///< Type alias for the CPU word type

        abstract_memory<WORD_T> *instr_bus = nullptr; // Pointer to the simulated instruction bus. Ignored if the subclass do not use a simulated memory.
        abstract_memory<WORD_T> *data_bus = nullptr; // Pointer to the simulated data bus. Ignored if the subclass do not use a simulated memory.
        libvio::bus *mmio_bus = nullptr; ///< The virtual MMIO bus. If nullptr, MMIO is disabled. Ignored on user-space emulators.

        libvio::ringbuffer<event_t<WORD_T>> *event_buffer = nullptr;  ///< Buffer for storing CPU events. If nullptr, event tracing is off.


        /**
         * @brief Get the number of the general purpose registers.
         * @return The number of the general purpose registers.
         */
        virtual uint8_t n_gpr(void) const = 0;

        /**
         * @brief Get the name of a general purpose register by its address.
         * @param addr The address of the register to query.
         * @return The name of the register as a null-terminated string.
         */
        virtual const char *gpr_name(uint8_t addr) const = 0;

        /**
         * @brief Get the address of a general purpose register by its name.
         * @param name The name of the register to query (case-sensitive).
         * @return The address of the register.
         */
        virtual uint8_t gpr_addr(const char *name) const = 0;

        /**
         * @brief Reset the CPU to prepare for execution.
         * @param init_pc The initial value of the program counter.
         */
         virtual void reset(WORD_T init_pc) = 0;

        /**
         * @brief Get the program counter value of the the next instruction to be comitted.
         * @return The next program counter value
         */
        virtual WORD_T get_pc(void) const = 0;

        /**
         * @brief Get a pointer to the general purpose register file
         * @return Pointer to the register file array. `nullptr` if not supported.
         */
        virtual const WORD_T *get_gpr(void) const = 0;

        /**
         * @brief Get the value of a specific general purpose register
         * @param addr Register address/index
         * @return The value of the specified register
         */
        virtual WORD_T get_gpr(uint8_t addr) const = 0;

        /**
         * @brief Advance the CPU by one clock cycle
         */
        virtual void next_cycle(void) = 0;

        /**
         * @brief Advance the CPU by n clock cycles
         * @param n The number of cycles to advance
         */
        virtual void next_cycle(size_t n) {
            for (size_t i=0; i<n; ++i) {
                next_cycle();
            }
        }

        /**
         * @brief Advance the CPU until at least one more instruction is committed.
         * @note On superscalar CPUs, this function may execute more than one instruction.
         *      Self-traps are counted as commited.
         */
        virtual void next_instruction(void) = 0;

        /**
         * @brief Advance the CPU until at least n more instructions are committed.
         * @param n The number of instructions to execute
         * @note On superscalar CPUs, this function may execute more than n instructions.
         *      Self-traps are counted as commited.
         */
        virtual void next_instruction(size_t n) {
            for (auto i=0; i<n; ++i) {
                next_instruction();
            }
        };
        
        /**
         * @brief Convert virtual address to physical address
         * @param vaddr Virtual address to convert
         * @return Corresponding physical address, `nullopt` if not mapped
         */
        virtual std::optional<WORD_T> vaddr_to_paddr(WORD_T vaddr) const {
            return vaddr;
        }

        /**
         * @brief Read from virtual memory
         * @param addr Virtual address to read from
         * @param width Width of the read operation
         * @return The value read from memory, `nullopt` if out of bound.
         * @note This function will not read from a MMIO address. It tiggers no side-effect like caching neither.
         *      If a MMIO address is provided, `nullopt` is returned.
         */
        virtual std::optional<WORD_T> vmem_peek(WORD_T addr, libvio::width_t width) const {
            auto paddr = vaddr_to_paddr(addr);
            if (paddr.has_value()) {
                return pmem_peek(paddr.value(), width);
            } else {
                return {};
            }
        }

        /**
         * @brief Read from physical memory
         * @param addr Physical address to read from
         * @param width Width of the read operation
         * @return The value read from memory, `nullopt` if out of bound.
         * @note This function will not read from a MMIO address. It tiggers no side-effect like caching neither.
         *      If a MMIO address is provided, `nullopt` is returned.
         */
        virtual std::optional<WORD_T> pmem_peek(WORD_T addr, libvio::width_t width) const = 0;

        /**
         * @brief Whether the execution has ended.
         * @return Whether the CPU has stopped.
         */
        virtual bool stopped(void) const = 0;

        /**
         * @brief Get the next trap to handle.
         * @return Some value, whose meaning depends on implementation, if at least one trap to handle, or `nullopt` if no trap.
         */
        virtual std::optional<WORD_T> get_trap(void) const = 0;
};

}

#endif
