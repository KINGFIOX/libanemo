#ifndef LIBSDB_SDB_DIFFTEST_HH
#define LIBSDB_SDB_DIFFTEST_HH

#include <libsdb/sdb.hh>
#include <libcpu/difftest.hh>

namespace libsdb {

/**
 * @class sdb_difftest
 * @brief Extension of sdb with differential testing capabilities
 * @tparam WORD_T The word type of the target CPU (e.g., uint32_t)
 */
template <typename WORD_T>
class sdb_difftest: public sdb<WORD_T> {
    public:
        libcpu::abstract_difftest<WORD_T>* difftest = nullptr;  /**< Differential testing interface */

        using sdb<WORD_T>::execute_command;

        /**
        * @brief Execute a pre-parsed command (overrides base class)
        * @param cmd Command token to execute
        */
        virtual void execute_command(command_t cmd) override;

        virtual const char *get_prompt(void) const override;
};


template <typename WORD_T>
void sdb_difftest<WORD_T>::execute_command(command_t cmd) {
    if (this->difftest == nullptr) {
        std::cerr << "libsdb: `sdb_difftest.difftest` and `sdb_difftest.cpu` cannot be nullptr." << std::endl;
    } else if (cmd.sdb_command == "dut") {
        this->cpu = this->difftest->dut;
    } else if (cmd.sdb_command == "ref") {
        this->cpu = this->difftest->ref;
    } else if (cmd.sdb_command == "difftest") {
        this->cpu = this->difftest;
    } else {
        sdb<WORD_T>::execute_command(cmd);
    }
}

template <typename WORD_T>
const char* sdb_difftest<WORD_T>::get_prompt(void) const {
    if (this->difftest==nullptr || this->cpu==nullptr) {
        std::cerr << "libsdb: `sdb_difftest.difftest` and `sdb_difftest.cpu` cannot be nullptr." << std::endl;
        return "sdb|error> ";
    } else if (this->cpu == this->difftest) {
        return "sdb|difftest> ";
    } else if (this->cpu == this->difftest->dut) {
        return "sdb|dut> ";
    } else if (this->cpu == this->difftest->ref) {
        return "sdb|ref> ";
    } else {
        std::cerr << "libsdb: invalid value for`sdb_difftest.cpu`." << std::endl;
        return "sdb|error> ";
    }
}

} // namespace libsdb

#endif // LIBSDB_SDB_DIFFTEST_HH
