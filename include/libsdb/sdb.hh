#ifndef LIBSDB_SDB_HH
#define LIBSDB_SDB_HH

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <ios>
#include <iostream>
#include <libcpu/event.hh>
#include <ostream>
#include <vector>
#include <string>
#include <libsdb/commandline.hh>
#include <libsdb/expression.hh>
#include <libcpu/abstract_cpu.hh>

namespace libsdb {

/**
 * @class sdb
 * @brief Template class for a simple debugger command-line interface for a simulated CPU.
 * @tparam WORD_T The word type of the target CPU (e.g., uint32_t).
 */
template <typename WORD_T>
class sdb {
public:
    /**
     * @struct command_def_t
     * @brief Command definition structure.
     */
    using command_def_t = struct {
        void (*func)(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os); /**< Command function pointer */
        const char* const* names; /**< Null-terminated array of command name aliases */
        const char* help; /**< Help text for the command */
    };

    /**
     * @struct watchpoint_t
     * @brief Watchpoint definition structure.
     */
    using watchpoint_t = struct {
        std::string str; /**< String representation of the watch expression */
        std::vector<token_t> expr; /**< Parsed post-fix expression for evaluation */
        std::optional<WORD_T> old_value; /**< Previous value for change detection */
    };

    // Command function declarations
    static void cmd_help(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_quit(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_continue(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_step(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_status(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_examine(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_watch(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_break(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_eval(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_trace(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);
    static void cmd_reset(std::vector<std::string> args, sdb<WORD_T>* sdb_inst, std::ostream& os);

    /**
     * @var commands
     * @brief Static command definition table.
     * @details Contains all debugger commands with their handlers, aliases, and help text.
     */
    static inline command_def_t commands[] = {
        {cmd_help, (const char* const[]){"help", "h", nullptr}, 
            "help: Show help for commands\n"
            "Usage:\n"
            "  help [command]"},
        {cmd_quit, (const char* const[]){"quit", "q", nullptr}, 
            "quit: Exit the debugger\n"
            "Usage:\n"
            "  quit"},
        {cmd_continue, (const char* const[]){"continue", "c", nullptr}, 
            "continue: Continue execution until breakpoint, watchpoint, or program end\n"
            "Usage:\n"
            "  continue"},
        {cmd_step, (const char* const[]){"step", "s", "si", nullptr}, 
            "step: Execute one or more instructions\n"
            "Usage:\n"
            "  step [n=1]"},
        {cmd_status, (const char* const[]){"status", "st", "regs", "r", nullptr}, 
            "status: Show current PC and general purpose registers\n"
            "Usage:\n"
            "  status"},
        {cmd_examine, (const char* const[]){"examine", "x", nullptr}, 
            "examine: Dump memory\n"
            "Usage:\n"
            "   examine <base> <length> <word_sz>\n"
            "  <base>     - Starting address (expression)\n"
            "  <length>   - Number of words to display (expression)\n"
            "  <word_sz>  - Word size in bytes (1, 2, 4, or 8)"},
        {cmd_watch, (const char* const[]){"watch", "w", nullptr}, 
            "watch: Manage watchpoints\n"
            "Usage:\n"
            "  watch <expr> - Set a watchpoint on an expression\n"
            "  watch ls     - List all watchpoints\n"
            "  watch rm <n> - Remove watchpoint by index\n"
            "Arguments:\n"
            "  <expr> - Expression to monitor\n"
            "  <n>    - Index of watchpoint to remove"},
        {cmd_break, (const char* const[]){"break", "b", "br", nullptr}, 
            "break: Manage breakpoints\nUsage:\n"
            "  break <addr>      - Set breakpoint at address\n"
            "  break ls          - List all breakpoints\n"
            "  break rm <n>      - Remove breakpoint by index\n"
            "  break trap on|off - Enable/disable trap breakpoints\n"
            "Arguments:\n"
            "  <addr> - Address expression for breakpoint\n"
            "  <n>    - Index of breakpoint to remove\n"
            "  on|off - Enable or disable trap breakpoints"},
        {cmd_eval, (const char* const[]){"evaluate", "eval", "e", "expr", nullptr},
            "eval: Evaluate an expression\n"
            "Usage:\n"
            "  evaluate <expression>"
        },
        {cmd_trace, (const char* const[]){"trace", "t", "log", "events", nullptr},
            "trace: show event logs\nUsage:\n"
            "  trace [instr] [mem] [func] [trap]"
        },
        {cmd_reset, (const char* const[]){"reset", "rst", nullptr},
            "reset: reset the cpu\n"
            "Usage:\n"
            "  reset <init_pc>\n"
            "Note:\n"
            "  This will not reset the content of the memory."
        }
    };

    libcpu::abstract_cpu<WORD_T>* cpu = nullptr; /**< Pointer to CPU instance being debugged */

    /**
     * @brief Check if debugger is in stopped state
     * @return true if execution is stopped (at breakpoint), false otherwise
     */
    virtual bool stopped(void) const;

    /**
     * @brief Show help for a specific command
     * @param cmd Command definition to show help for
     * @param os Output stream for help text
     */
    static void show_command_help(const command_def_t& cmd, std::ostream& os);

    /**
     * @brief Show help for a command by name
     * @param name Command name to look up
     * @param os Output stream for help text
     */
    static void show_command_help(const char* name, std::ostream& os);

    /**
     * @brief Execute a command from string input
     * @param cmd Command string to parse and execute
     */
    virtual void execute_command(std::string cmd);

    /**
     * @brief Execute a pre-parsed command
     * @param cmd Command token to execute
     */
    virtual void execute_command(command_t cmd);

    /**
     * @brief Get the command prompt of SDB, may change according to the state.
     * @return A constant string containing the command prompt.
     */
    virtual const char *get_prompt(void) const;

protected:
    bool is_stopped = false; /**< Internal stopped state flag */

    std::vector<WORD_T> breakpoints = {}; /**< Active breakpoint addresses */
    std::vector<watchpoint_t> watchpoints = {}; /**< Active watchpoints */
    bool breakpoint_on_trap = false; /**< Whether to break on CPU traps */

    /**
     * @brief Check if current PC matches any breakpoint
     * @param os Output stream for notifications
     * @return true if breakpoint hit, false otherwise
     */
    bool check_breakpoints(std::ostream& os);

    /**
     * @brief Check all watchpoints for value changes
     * @param os Output stream for notifications
     * @return true if any watchpoint triggered, false otherwise
     */
    bool check_watchpoints(std::ostream& os);

    /**
     * @brief Check if CPU is in trap state
     * @param os Output stream for notifications
     * @return true if in trap state and traps are enabled, false otherwise
     */
    bool check_trap(std::ostream& os);

    /**
     * @brief Execute single-step operations
     * @param n Number of steps to execute
     * @param os Output stream for step notifications
     */
    void execute_steps(size_t n, std::ostream& os);
};

template <typename WORD_T>
bool sdb<WORD_T>::stopped(void) const {
    return is_stopped;
}

template <typename WORD_T>
void sdb<WORD_T>::show_command_help(const command_def_t &def, std::ostream &os) {
    os << def.help << std::endl;
    if (def.names[1] != nullptr) {
        os << "Alias:" << std::endl;
        os << "  ";
        for (size_t i=1; def.names[i]!=nullptr; ++i) {
            os << ' ' << def.names[i];
        }
        os << std::endl;
    }
}

template <typename WORD_T>
void sdb<WORD_T>::show_command_help(const char *name, std::ostream &os) {
    for (const command_def_t &def: commands) {
        for (size_t i=0; def.names[i]!=nullptr; ++i) {
            if (std::strcmp(def.names[i], name) == 0){
                show_command_help(def, os); 
                return;
            }
        }
    }
}

template <typename WORD_T>
void sdb<WORD_T>::execute_command(std::string cmd){
    std::optional<std::vector<std::string>> tokens_opt = tokenize_command(cmd);
    if (!tokens_opt.has_value()) {
        std::cerr << "libsdb: command syntax error." << std::endl;
        return;
    }
    std::optional<command_t> cmd_opt = parse_command(tokens_opt.value());
    if (!cmd_opt.has_value()) {
        std::cerr << "libsdb: command syntax error." << std::endl;
        std::cerr << "Must be one of:\n<command> [arg]...\n<command> [arg]... | <pipe_command>" << std::endl;
        return;
    }
    execute_command(cmd_opt.value());
}

template <typename WORD_T>
void sdb<WORD_T>::execute_command(command_t cmd) {
    if (cpu == nullptr) {
        return;
    }
    for (const command_def_t &def: commands) {
        for (size_t i=0; def.names[i]!=nullptr; ++i) {
            if (std::strcmp(def.names[i], cmd.sdb_command.c_str()) == 0){
                if (cmd.pipe_command.has_value()) {
                    popen_ostream os{cmd.pipe_command.value()};
                    def.func(cmd.args, this, os);
                } else {
                    def.func(cmd.args, this, std::cout);
                }
                return;
            }
        }
    }
    std::cerr << "libsdb: Command not found." << std::endl;
}

// Command function implementations
template <typename WORD_T>
void sdb<WORD_T>::cmd_help(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (args.empty()) {
        for (const auto &cmd: commands) {
            show_command_help(cmd, os);
            os << std::endl;
        }
    } else {
        show_command_help(args[0].c_str(), os);
    }
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_quit(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (!args.empty()) {
        show_command_help("quit", os);
    }
    sdb_inst->is_stopped = true;
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_continue(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (!args.empty()) {
        show_command_help("continue", os);
        return;
    }
    sdb_inst->execute_steps(SIZE_MAX, os);
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_step(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    size_t n;
    if (args.size() == 0) {
        n = 1;
    } else {
        std::string expr_str;
        for (const auto& arg: args) {
            expr_str += arg + " ";
        }
        auto n_opt = evaluate_expression(expr_str, sdb_inst->cpu);
        if (n_opt.has_value()) {
            n = n_opt.value();
        } else {
            os << "libsdb: Invalid expression in arguments." << std::endl;
            return;
        }
    }
    sdb_inst->execute_steps(n, os);
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_status(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (!args.empty()) {
        show_command_help("status", os);
        return;
    }
    
    os << "  pc=0x" << std::hex << sdb_inst->cpu->get_pc() << "\n";
    for (uint8_t i=0; i<sdb_inst->cpu->n_gpr(); ++i) {
        os << std::right << std::setw(4) << std::setfill(' ') << sdb_inst->cpu->gpr_name(i) << "=0x" 
           << std::hex << std::setw(sizeof(WORD_T)*2) << std::setfill('0') << sdb_inst->cpu->get_gpr(i) << ' ';
        if (i%8 == 7) {
            os << std::endl;
        }
    }
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_examine(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (args.size() != 3) {
        show_command_help("examine", os);
        return;
    }
    
    // evaluate each argument
    std::optional<WORD_T> base_opt = evaluate_expression(args[0], sdb_inst->cpu);
    std::optional<WORD_T> length_opt = evaluate_expression(args[1], sdb_inst->cpu);
    std::optional<WORD_T> word_sz_opt = evaluate_expression(args[2], sdb_inst->cpu);
    if (!base_opt.has_value() || !length_opt.has_value() || !word_sz_opt.has_value()) {
        os << "libsdb: Invalid expression in arguments." << std::endl;
        return;
    }
    WORD_T base = base_opt.value();
    WORD_T length = length_opt.value();
    WORD_T word_sz = word_sz_opt.value();

    if (word_sz!=1 && word_sz!=2 && word_sz!=4 && word_sz!=8) {
        os << "libsdb: Invalid word size (must be 1, 2, 4, or 8)\n";
        return;
    }
    
    for (WORD_T addr=base; addr<base+length*word_sz; addr+=word_sz) {
        if (addr%16 == 0) {
            os << "0x" << std::hex << addr << ":";
        }
        auto val = sdb_inst->cpu->vmem_peek(addr, static_cast<libvio::width_t>(word_sz));
        if (val.has_value()) {
            os << " " << std::setfill('0') << std::setw(word_sz * 2) 
               << std::hex << val.value();
        } else {
            os << " ?";
        }
        if ((addr+word_sz)%16 == 0) {
            os << "\n";
        }
    }
    if ((base+length*word_sz)%16 != 0) {
        os << "\n";
    }
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_watch(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (args.empty()) {
        show_command_help("watch", os);
        return;
    }
    
    if (args[0] == "ls") {
        // List watchpoints
        if (sdb_inst->watchpoints.empty()) {
            os << "No watchpoints set\n";
            return;
        }
        
        for (size_t i = 0; i < sdb_inst->watchpoints.size(); i++) {
            os << "[" << i << "] " << sdb_inst->watchpoints[i].str;
            if (sdb_inst->watchpoints[i].old_value.has_value()) {
                os << " = 0x" << std::hex << sdb_inst->watchpoints[i].old_value.value();
            }
            os << "\n";
        }
    } else if (args[0] == "rm") {
        // Remove watchpoint
        if (args.size() < 2) {
            os << "Missing watchpoint index\n";
            return;
        }
        
        size_t idx = std::stoul(args[1]);
        if (idx >= sdb_inst->watchpoints.size()) {
            os << "Invalid watchpoint index\n";
            return;
        }
        sdb_inst->watchpoints.erase(sdb_inst->watchpoints.begin()+idx);
        os << "Removed watchpoint " << idx << "\n";
    } else {
        // Set watchpoint
        std::string expr_str;
        for (const auto& arg: args) {
            expr_str += arg + " ";
        }
        
        auto tokens = tokenize_expression(expr_str);
        auto parsed = parse_expression(tokens);
        if (!parsed.has_value()) {
            os << "Invalid expression\n";
            return;
        }
        
        watchpoint_t wp;
        wp.str = expr_str;
        wp.expr = parsed.value();
        specialize_expression(wp.expr, *(sdb_inst->cpu));
        auto val = evaluate_expression(wp.expr, sdb_inst->cpu);
        if (val.has_value()) {
            wp.old_value = val.value();
        }
        
        sdb_inst->watchpoints.push_back(wp);
        os << "Watchpoint [" << sdb_inst->watchpoints.size() - 1 << "] set: " 
           << expr_str << " = 0x" << std::hex << val.value_or(0) << "\n";
    }
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_break(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (args.empty()) {
        show_command_help("break", os);
    }
    
    if (args[0] == "ls") {
        // List breakpoints
        if (sdb_inst->breakpoints.empty()) {
            os << "No breakpoints set\n";
            return;
        }
        
        for (size_t i = 0; i < sdb_inst->breakpoints.size(); i++) {
            os << "[" << i << "] 0x" << std::hex << sdb_inst->breakpoints[i] << "\n";
        }
    } else if (args[0] == "rm") {
        // Remove breakpoint by index
        if (args.size() < 2) {
            os << "Missing breakpoint index\n";
            return;
        }

        size_t idx = std::stoul(args[1]);
        if (idx >= sdb_inst->breakpoints.size()) {
            os << "Invalid breakpoint index\n";
            return;
        }
        WORD_T addr = sdb_inst->breakpoints[idx];
        sdb_inst->breakpoints.erase(sdb_inst->breakpoints.begin() + idx);
        os << "Removed breakpoint [" << idx << "] at 0x" << std::hex << addr << "\n";
    } else if (args[0] == "trap") {
        // Manage trap breakpoints
        if (args.size() < 2) {
            os << "Missing argument\n";
            return;
        }
        
        if (args[1] == "on") {
            sdb_inst->breakpoint_on_trap = true;
            os << "Break on trap enabled\n";
        } else if (args[1] == "off") {
            sdb_inst->breakpoint_on_trap = false;
            os << "Break on trap disabled\n";
        } else {
            os << "Invalid argument (must be 'on' or 'off')\n";
        }
    } else {
        // Set breakpoint at address
        std::string expr_str;
        for (const auto& arg : args) {
            expr_str += arg + " ";
        }
        auto addr = evaluate_expression(expr_str, sdb_inst->cpu);
        if (!addr.has_value()) {
            os << "libsdb: Invalid expression in arguments." << std::endl;
            return;
        }
        
        // Check if breakpoint already exists
        auto it = std::find(sdb_inst->breakpoints.begin(), sdb_inst->breakpoints.end(), addr.value());
        if (it != sdb_inst->breakpoints.end()) {
            os << "Breakpoint already exists at 0x" << std::hex << addr.value() << "\n";
            return;
        }
        
        sdb_inst->breakpoints.push_back(addr.value());
        os << "Breakpoint [" << sdb_inst->breakpoints.size() - 1 << "] set at 0x" 
           << std::hex << addr.value() << "\n";
    }
}

template <typename WORD_T>
bool sdb<WORD_T>::check_breakpoints(std::ostream &os) {
    WORD_T pc = cpu->get_pc();
    for (const WORD_T &bp : breakpoints) {
        if (bp == pc) {
            os << "Breakpoint at 0x" << std::hex << pc << std::endl;
            return true;
        }
    }
    return false;
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_eval(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (args.size() == 0) {
        show_command_help("evaluate", os);
        return;
    }
    std::string expr_str;
    for (const auto& arg : args) {
        expr_str += arg + " ";
    }
    std::optional<WORD_T> val_opt = evaluate_expression(expr_str, sdb_inst->cpu);
    if (!val_opt.has_value()) {
        os << "libsdb: Invalid expression in arguments." << std::endl;
        return;
    }

    WORD_T val = val_opt.value();
    const size_t total_bits = sizeof(WORD_T) * 8;

    std::string formatted_bin;
    for (size_t i=0; i<total_bits; ++i) {
        if (i>0 && i%8==0) {
            formatted_bin = ' ' + formatted_bin;
        }
        formatted_bin = ((val>>i)&1 ? '1' : '0') + formatted_bin;
    }
    os << "Binary: " << formatted_bin << std::endl;

    const int oct_width = (total_bits+2) / 3;
    os << "Octal: " 
       << std::setw(oct_width) << std::setfill('0') 
       << std::oct << val << std::endl;

    os << "Decimal: " << std::dec << val << std::endl;

    const int hex_width = sizeof(WORD_T) * 2;
    os << "Hexadecimal: " 
       << std::setw(hex_width) << std::setfill('0') 
       << std::hex << val << std::endl;
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_trace(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    bool instr, mem, func, trap;
    if (args.size() == 0) {
        instr = mem = func = trap = true;
    } else {
        instr = mem = func = trap = false;
        for (auto s: args) {
            if (s == "instr") {
                instr = true;
            } else if (s == "mem") {
                mem = true;
            } else if (s == "func") {
                func = true;
            } else if (s == "trap") {
                trap = true;
            } else {
                show_command_help("trace", os);
                return;
            }
        }
    }
    if (sdb_inst->cpu->event_buffer == nullptr) {
        os << "Event buffer is null, tracing disabled." << std::endl;
        return;
    }
    for (libcpu::event_t<WORD_T> e: *(sdb_inst->cpu->event_buffer)) {
        switch (e.type) {
            case libcpu::event_type_t::issue:
            case libcpu::event_type_t::reg_write:
                if (instr) {
                    os << e.to_string() << std::endl;
                }
                break;
            case libcpu::event_type_t::load:
            case libcpu::event_type_t::store:
                if (mem) {
                    os << e.to_string() << std::endl;
                }
                break;
            case libcpu::event_type_t::call:
            case libcpu::event_type_t::call_ret:
                if (func) {
                    os << e.to_string() << std::endl;
                }
                break;
            case libcpu::event_type_t::trap:
            case libcpu::event_type_t::trap_ret:
                if (trap) {
                    os << e.to_string() << std::endl;
                }
                break;
            default:
                break;
        }
    }
}

template <typename WORD_T>
void sdb<WORD_T>::cmd_reset(std::vector<std::string> args, sdb<WORD_T> *sdb_inst, std::ostream &os) {
    if (args.size() == 0) {
        show_command_help("reset", os);
    } else {
        std::string expr_str;
        for (const auto& arg: args) {
            expr_str += arg + " ";
        }
        auto init_pc_opt = evaluate_expression(expr_str, sdb_inst->cpu);
        if (init_pc_opt.has_value()) {
            WORD_T init_pc = init_pc_opt.value();
            sdb_inst->cpu->reset(init_pc);
        } else {
            os << "libsdb: Invalid expression in arguments." << std::endl;
            return;
        }
    }
}

template <typename WORD_T>
bool sdb<WORD_T>::check_watchpoints(std::ostream &os) {
    for (auto &wp : watchpoints) {
        auto new_val = evaluate_expression(wp.expr, cpu);
        if (new_val.has_value() && wp.old_value.has_value() && wp.old_value.value()!= new_val.value()) {
            os << "Watchpoint " << wp.str << " changed: old = ";
            if (wp.old_value.has_value()) {
                os << "0x" << std::hex << wp.old_value.value();
            } else {
                os << "<uninitialized>";
            }
            os << ", new = 0x" << std::hex << new_val.value() << std::endl;
            wp.old_value = new_val;
            return true;
        }
    }
    return false;
}

template <typename WORD_T>
bool sdb<WORD_T>::check_trap(std::ostream &os) {
    if (breakpoint_on_trap && cpu->get_trap().has_value()) {
        os << "Trap encountered: cause=0x" <<
           std::hex << cpu->get_trap().value() << std::endl;
        return true;
    }
    return false;
}

template <typename WORD_T>
void sdb<WORD_T>::execute_steps(size_t n, std::ostream &os) {
    for (size_t i = 0; i < n; i++) {
        if (cpu->stopped()) {
            os << "CPU stopped" << std::endl;
            break;
        }
        cpu->next_instruction();
        if (check_breakpoints(os) || check_watchpoints(os) || check_trap(os)) {
            break;
        }
    }
}

template <typename WORD_T>
const char* sdb<WORD_T>::get_prompt(void) const {
    return "sdb> ";
}

}

#endif
