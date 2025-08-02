#ifndef LIBCPU_DIFFTEST_HH
#define LIBCPU_DIFFTEST_HH

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/event.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/width.hh>
#include <ostream>
#include <vector>

namespace libcpu {

/**
 * @brief Abstract base class for differential testing of CPUs
 * @tparam WORD_T The word type used by the CPU (e.g., uint32_t, uint64_t)
 * 
 * This class provides the interface for comparing the behavior of multiple CPU implementations.
 */
template <typename WORD_T>
class abstract_difftest: public abstract_cpu<WORD_T> {
    public:
        using cpu_t = abstract_cpu<WORD_T>;

        cpu_t *dut = nullptr;
        cpu_t *ref = nullptr;

        // running difftest on CPUs with different numbers of registers is allowed
        // for example, between RV32I and RV32E
        // but the number of available registers need to be limited to the smallest one
        uint8_t n_gpr(void) const override {
            return std::min(dut->n_gpr(), ref->n_gpr());
        }

        virtual const char *gpr_name(uint8_t addr) const override {
            return dut->gpr_name(addr);
        }

        virtual uint8_t gpr_addr(const char *name) const override {
            return dut->gpr_addr(name);
        }

        WORD_T get_pc(void) const override {
            return dut->get_pc();
        }

        const WORD_T *get_gpr(void) const override {
            return dut->get_gpr();
        }

        WORD_T get_gpr(uint8_t addr) const override {
            return dut->get_gpr(addr);
        }

        std::optional<WORD_T> vaddr_to_paddr(WORD_T vaddr) const override {
            return dut->vaddr_to_paddr(vaddr);
        }

        std::optional<WORD_T> vmem_peek(WORD_T addr, libvio::width_t width) const override {
            return dut->vmem_peek(addr, width);
        }

        std::optional<WORD_T> pmem_peek(WORD_T addr, libvio::width_t width) const override {
            return dut->pmem_peek(addr, width);
        }

        /**
        * @brief Get the difftest error of the CPU.
        * @returns Whether there is a difftest error.
        * @note The actual content of the error is put into the event buffer as an event.
        */
        virtual bool get_difftest_error(void) const = 0;

        virtual bool stopped(void) const override {
            if (get_difftest_error()) {
                return true;
            } else if (ref->stopped()) {
                if (!dut->stopped()) {
                    std::cerr << "libcpu: REF has stopped but DUT has not." << std::endl;
                }
                return true;
            } else if (dut->stopped()) {
                if (!ref->stopped()) {
                    std::cerr << "libcpu: DUT has stopped but REF has not." << std::endl;
                }
                return true;
            } else {
                return false;
            }
        }

        virtual void reset(WORD_T init_pc) override {
            dut->reset(init_pc);
            ref->reset(init_pc);
        }

        virtual std::optional<WORD_T> get_trap(void) const override {
            return ref->get_trap();
        }
};

/**
 * @brief A simple differential testing implementation
 * @tparam WORD_T The word type used by the CPU (e.g., uint32_t, uint64_t)
 * 
 * This class implements a simple logic comparing CPU execution events for differential testing.
 * The dut can be any type of CPU, it can execute more than one instructions in a cycle or execute a single instruction in multiple cycles.
 * The ref is assumed to be a single-cycle processor.
 * The comparision is based on the events logged in the event buffer. If the event buffer is a nullptr,
 * the differential testing will fail.
 */
template <typename WORD_T>
class simple_difftest: public abstract_difftest<WORD_T> {
    public:
        using event_buffer_t = libvio::ringbuffer<event_t<WORD_T>>;

    private:
        size_t dut_buffer_index = 0;
        size_t ref_buffer_index = 0;
        bool difftest_error = false;

    public:

        bool get_difftest_error(void) const override {
            return difftest_error;
        }

        void next_instruction(void) override {
            next_cycle();
        }

        void next_cycle(void) override {
            // panic if all the event buffers are not available
            if (this->dut->event_buffer==nullptr || this->ref->event_buffer==nullptr) {
                std::cerr << "libcpu: difftest error: event buffer unavaliable." << std::endl;
                difftest_error = true;
                return;
            }

            // This implementation only compares resister writes and traps
            // Because register being written means some instruction has been commited.
            // And all instructions are commited in order.
            // And traps cannot happen out of ordder.
            // This is the only assumption that can be made across all types of CPUs.
            auto pull_events = [this](std::vector<event_t<WORD_T>> &dest, event_buffer_t *buffer, size_t begin) {
                for (size_t i=begin; i<buffer->lastindex(); ++i) {
                    event_t<WORD_T> event = (*buffer)[i];
                    if (event.type==event_type_t::reg_write || event.type==event_type_t::trap || event.type==event_type_t::trap_ret) {
                        dest.push_back(event);
                    }
                }
                return buffer->lastindex();
            };

            // step the DUT for a cycle
            // zero or one or multiple instructions can be committed
            std::vector<event_t<WORD_T>> dut_events{};
            this->dut->next_cycle();
            // record the events of DUT
            dut_buffer_index = pull_events(dut_events, this->dut->event_buffer, dut_buffer_index);
            // step the ref 
            std::vector<event_t<WORD_T>> ref_events{};
            while (ref_events.size()<dut_events.size() && !this->ref->stopped()) {
                this->ref->next_instruction();
                // record the events of REF
                ref_buffer_index = pull_events(ref_events, this->ref->event_buffer, ref_buffer_index);
            }

            // compare events of DUT and REF
            if (dut_events.size() != ref_events.size()) {
                difftest_error = true;
            }
            if (!difftest_error) {
                for (size_t i=0; i<dut_events.size(); ++i) {
                    if (dut_events[i] != ref_events[i]) {
                        difftest_error = true;
                        break;
                    }
                }
            }

            if (difftest_error) {
                std::cerr << "libcpu: difftest error" << std::endl;
                std::cerr << "dut:" << std::endl;
                for (auto e: dut_events) {
                    std::cout << e.to_string() << std::endl;
                }
                std::cerr << "ref:" << std::endl;
                for (auto e: ref_events) {
                    std::cout << e.to_string() << std::endl;
                }
            }
        }
};

}

#endif
