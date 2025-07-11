#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <libcpu/rv64i_cpu_system_ptrace.hh>
#include <iostream>
#include <libvio/width.hh>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/ucontext.h>

using namespace libvio;
using namespace libcpu;
using namespace rv64i;

rv64i_cpu_system_ptrace::rv64i_cpu_system_ptrace(const char *program_path, char *const argv[], char *const envp[]) {
    pid_t pid = fork();
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
        execve(program_path, argv, envp);
    } else {
        guest_pid = pid;
        int child_status;
        waitpid(guest_pid, &child_status, 0);
        if (WIFSTOPPED(child_status)) {
            std::cout << "execve" << std::endl;
        }
        // Clear the pending SIGTRAP by continuing with no signal
        // ptrace(PTRACE_CONT, guest_pid, nullptr, (void*)(uintptr_t)0);
        // Wait for the child to stop again at the first instruction
        // waitpid(guest_pid, &child_status, 0);
    }
}

uint64_t rv64i_cpu_system_ptrace::get_pc() const {
    return guest_state[0];
}

const uint64_t *rv64i_cpu_system_ptrace::get_gpr(void) const {
    return nullptr;
}

uint64_t rv64i_cpu_system_ptrace::get_gpr(uint8_t gpr_addr) const {
    if (gpr_addr==0 || gpr_addr>31) {
        return 0;
    } else {
        return guest_state[gpr_addr];
    }
}

void rv64i_cpu_system_ptrace::next_cycle(void) {
    next_instruction();
}

void rv64i_cpu_system_ptrace::next_instruction(void) {
    int child_status;
    ptrace(PTRACE_SINGLESTEP, guest_pid, nullptr, nullptr);
    waitpid(guest_pid, &child_status, 0);
    ptrace(PTRACE_GETREGS, guest_pid, nullptr, guest_state);
    assert(guest_state[32]==0);
    if (WIFEXITED(child_status)) {
        guest_stopped = true;
    }
}

bool rv64i_cpu_system_ptrace::stopped(void) const {
    return guest_stopped;
}

std::optional<uint64_t> rv64i_cpu_system_ptrace::pmem_read(uint64_t addr, libvio::width_t width) const {
    uint64_t addr_high = addr & (~0x3);
    uint64_t addr_low = addr & 0x3;
    uint64_t data = ptrace(PTRACE_PEEKTEXT, guest_pid, addr, NULL);
    if (errno == 0) {
        data = data>>(8*addr_low);
        return zero_truncate<uint64_t>(data, width);
    } else {
        return {};
    }
}

rv64i_cpu_system_ptrace::~rv64i_cpu_system_ptrace() {
    ptrace(PTRACE_KILL, guest_pid, nullptr, nullptr);
}
