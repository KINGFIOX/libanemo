#ifndef LIBCPU_EVENT_HH
#define LIBCPU_EVENT_HH

#include <ios>
#include <string>
#include <sstream>
#include <iomanip>

/**
 * @file event.hh
 * @brief Definition of CPU event types for debugging and difftest
 */

namespace libcpu {

/**
 * @enum event_type_t
 * @brief Enumeration of different CPU event types
 * 
 * Each event type has specific meanings for its val1 and val2 fields
 */
enum class event_type_t {
    issue,       ///< Instruction issued - val1: instr_part1, val2: instr_part2
    reg_write,   ///< Register written - val1: rd_addr, val2: rd_data
    load,        ///< Memory load - val1: addr, val2: zero extended data
    store,       ///< Memory store - val1: addr, val2: zero extended data
    call,        ///< Function call - val1: target_addr, val2: stack_pointer
    call_ret,    ///< Function return - val1: target_addr, val2: stack_pointer
    trap,        ///< Trap handling - val1: mcause, val2: mtval
    trap_ret,    ///< Trap return - val1: target_addr, val2: mstatus
    diff_error,  ///< Difftest error - val1: event_type, val2: instr_part1
    n_event_type ///< A place holder indicating the number of event types
};

/**
 * @brief Convert an event_type_t to its string representation
 * @param type The event type to convert
 * @return String representation of the event type
 */
inline const char *event_type_to_str(event_type_t type) {
    switch (type) {
        case event_type_t::issue:      return "issue";
        case event_type_t::reg_write:  return "reg_write";
        case event_type_t::load:       return "load";
        case event_type_t::store:      return "store";
        case event_type_t::call:       return "call";
        case event_type_t::call_ret:   return "call_ret";
        case event_type_t::trap:       return "trap";
        case event_type_t::trap_ret:   return "trap_ret";
        case event_type_t::diff_error: return "diff_error";
        default:                       return "unknown";
    }
}

/**
 * @struct event_t
 * @brief Template structure representing a CPU event
 * @tparam WORD_T The word type used for event values (typically uint32_t or uint64_t)
 */
template <typename WORD_T>
struct event_t {
    event_type_t type; ///< Type of the event
    WORD_T pc;         ///< Program counter associated with the event
    WORD_T val1;       ///< First value (meaning depends on event type)
    WORD_T val2;       ///< Second value (meaning depends on event type)

    /**
     * @brief Equality comparison operator for event_t
     * @param other The event to compare with
     * @return true if all fields are equal, false otherwise
     */
    bool operator==(const event_t& other) const {
        return type == other.type && 
               pc == other.pc && 
               val1 == other.val1 && 
               val2 == other.val2;
    }
    /**
     * @brief Inequality comparison operator for event_t
     * @param other The event to compare with
     * @return true if any field is not equal, false otherwise
     */
    bool operator!=(const event_t& other) const {
        return !(*this == other);
    }

    /**
     * @brief Convert the event to a human-readable string
     * @return String representation of the event with appropriate field labels
     */
    std::string to_string() const {
        const char *label1 = "";
        const char *label2 = "";
        int label_width = 8; // Width for field labels
        
        // Determine labels based on event type
        switch (type) {
            case event_type_t::issue:       label1 = "instr1"; label2 = "instr2"; break;
            case event_type_t::reg_write:   label1 = "rd_addr"; label2 = "rd_data"; break;
            case event_type_t::load:        label1 = "addr"; label2 = "data"; break;
            case event_type_t::store:       label1 = "addr"; label2 = "data"; break;
            case event_type_t::call:        label1 = "target"; label2 = "sp"; break;
            case event_type_t::call_ret:    label1 = "target"; label2 = "sp"; break;
            case event_type_t::trap:        label1 = "mcause"; label2 = "mtval"; break;
            case event_type_t::trap_ret:    label1 = "target"; label2 = "mstatus"; break;
            case event_type_t::diff_error:  label1 = "err_type"; label2 = "instr"; break;
            default:                        label1 = "val1"; label2 = "val2"; break;
        }

        std::ostringstream oss;
        oss << std::left << std::setw(10) << std::setfill(' ') << event_type_to_str(type) 
            << " pc:0x" << std::hex << std::setw(sizeof(WORD_T)*2) << std::setfill('0') << pc << " "
            << std::left << std::setw(label_width) << std::setfill(' ')  << label1 << ":0x" << std::hex << std::right << std::setw(sizeof(WORD_T)*2) << std::setfill('0') << val1 << " "
            << std::left << std::setw(label_width) << std::setfill(' ')  << label2 << ":0x" << std::hex << std::right << std::setw(sizeof(WORD_T)*2) << std::setfill('0') << val2;

        return oss.str();
    }
};

}

#endif
