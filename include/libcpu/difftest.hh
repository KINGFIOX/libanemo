#ifndef LIBCPU_DIFFTEST_HH
#define LIBCPU_DIFFTEST_HH

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <libcpu/abstract_cpu.hh>
#include <libcpu/event.hh>
#include <libvio/ringbuffer.hh>
#include <libvio/width.hh>
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

        abstract_difftest(std::initializer_list<cpu_t*> cpu_list): cpus(cpu_list) {
            // running difftest on CPUs with different numbers of registers is allowed
            // for example, between RV32I and RV32E
            // but the number of available registers need to be limited to the smallest one
            for (auto cpu: cpus) {
                min_n_gpr = std::min(min_n_gpr, cpu->n_gpr());
            }
        }

        size_t n_gpr(void) const override {
            return min_n_gpr;
        }

        WORD_T get_pc(void) const override {
            return cpus[0]->get_pc();
        }

        const WORD_T *get_gpr(void) const override {
            return cpus[0]->get_gpr();
        }

        WORD_T get_gpr(uint8_t addr) const override {
            return cpus[0]->get_gpr(addr);
        }

        std::optional<WORD_T> vaddr_to_paddr(WORD_T vaddr) const override {
            return cpus[0]->vaddr_to_paddr(vaddr);
        }

        std::optional<WORD_T> vmem_read(WORD_T addr, libvio::width_t width) const override {
            return cpus[0]->vmem_read(addr, width);
        }

        std::optional<WORD_T> pmem_read(WORD_T addr, libvio::width_t width) const override {
            return cpus[0]->pmem_read(addr, width);
        }

        /**
        * @brief Get the difftest error of the CPU.
        * @returns Whether there is a difftest error.
        * @note The actual content of the error is put into the event buffer as an event.
        */
        virtual bool get_difftest_error(void) const = 0;

        virtual bool stopped(void) const override {
            return get_difftest_error() || cpus[0]->stopped();
        }

    private:
        size_t min_n_gpr = SIZE_MAX;

    protected:
        std::vector<cpu_t*> cpus = {};
};

/**
 * @brief Ordered differential testing implementation
 * @tparam WORD_T The word type used by the CPU (e.g., uint32_t, uint64_t)
 * 
 * This class implements ordered comparison of CPU execution events for differential testing.
 * The comparision is based on the events logged in the event buffer. If the event buffer is a nullptr,
 * the differential testing will fail.
 * This class assumes that for all the CPUs under test, the same type of event can only happen once
 * in a single instruction, and the same type of events must happen in the same order of the instructions
 * that cause those events.
 * For example, if multiple registers are written by the same instruction, or instructions are issued out of order,
 * a false positive will be reported. 
 */
template <typename WORD_T>
class ordered_difftest: public abstract_difftest<WORD_T> {
    public:
        using cpu_t = abstract_cpu<WORD_T>;
        using event_buffer_t = libvio::ringbuffer<event_t<WORD_T>>;

    private:
        std::vector<size_t> buffer_index;
        bool difftest_error = false;

    public:
        ordered_difftest(std::initializer_list<cpu_t*> cpu_list): abstract_difftest<WORD_T>(cpu_list) {
            buffer_index.resize(cpu_list.size());
            std::fill(buffer_index.begin(), buffer_index.end(), 0);
        }

        bool get_difftest_error(void) const override {
            return difftest_error;
        }

        void next_cycle(void) override {
            next_instruction();
        }

        void next_instruction(void) override {
            for (auto cpu: this->cpus) {
                cpu->next_instruction();
            }
            std::optional<event_t<WORD_T>> events[static_cast<size_t>(event_type_t::n_event_type)] {};
            for (size_t cpu_index=0; cpu_index<this->cpus.size(); ++cpu_index) {
                // compare the new events in each buffer
                const event_buffer_t *buffer = this->cpus[cpu_index]->event_buffer;
                // If the event buffer of the CPU under test is not set
                // raise a difftest error and skip this CPU
                if (buffer == nullptr) {
                    difftest_error = true;
                    if (this->event_buffer != nullptr) {
                        this->event_buffer->push_back({.type=event_type_t::diff_error, .pc=this->get_pc(), .val1=static_cast<WORD_T>(event_type_t::diff_error), .val2=0});
                    }
                    continue;
                }
                for (size_t event_index=buffer_index[cpu_index]; event_index<buffer->lastindex(); ++event_index) {
                    event_t<WORD_T> event = (*buffer)[event_index];
                    size_t event_type = static_cast<size_t>(event.type);
                    if (events[event_type].has_value()) {
                        // it this type of event has been seen before
                        // compare it with the recorded one
                        if (event != events[event_type].value()) {
                            difftest_error = true;
                            // print out the error details
                            std::cout << "Difftest error!" << std::endl;
                            std::cout << events[event_type].value().to_string() << std::endl;
                            std::cout << event.to_string() << std::endl;
                            // log this difftest error into the event buffer
                            if (this->event_buffer != nullptr) {
                                this->event_buffer->push_back({
                                    .type=event_type_t::diff_error,
                                    .pc=this->get_pc(),
                                    .val1=static_cast<WORD_T>(event.type),
                                    .val2=this->pmem_read(this->get_pc(), libvio::width_t::dword).value_or(0)
                                });
                            }
                        }
                    } else {
                        // if this type event is the first time to be seen in this instruction
                        // record it
                        events[event_type] = event;
                        if (this->event_buffer != nullptr) {
                            this->event_buffer->push_back(event);
                        }
                    }
                }
                buffer_index[cpu_index] = buffer->lastindex();
            }
        }
};

}

#endif
