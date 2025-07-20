#ifndef LIBSDB_COMMANDLINE_HH
#define LIBSDB_COMMANDLINE_HH

#include <vector>
#include <string>
#include <optional>
#include <ostream>
#include <streambuf>
#include <memory>

namespace libsdb {

/**
 * @brief Represents a command with optional piping capability.
 *
 * This structure holds a command that can be executed, with an optional
 * pipe command that can process the output of the primary command.
 */
struct command_t {
    /// @brief The primary command. It is run on libsdb.
    std::string sdb_command;

    /// @brief The arguments of the primary command.
    std::vector<std::string> args;

    /// @brief An optional pipe command that processes the output of the primary command. It is passed to `popen`.
    /// If this has a value, the output of `sdb_command` will be piped to this command.
    std::optional<std::string> pipe_command;
};

/**
 * @brief Tokenizes a command string while respecting quotes and escape characters.
 *
 * This function splits a command string into tokens using spaces as delimiters, 
 * with the following special handling:
 * - Double quotes (") toggle quoting mode (spaces inside quotes are preserved)
 * - Backslash (\\) escapes the next character (treating it as literal)
 * - Tokens are separated by unquoted spaces
 *
 * @param command The input string to tokenize.
 * @return std::optional<std::vector<std::string>> List of tokens extracted from the command, nullopt if syntax error.
 */
std::optional<std::vector<std::string>> tokenize_command(std::string command);

/**
 * @brief Parses tokenized command into structured format with optional pipe command.
 * 
 * The command must follow one of these valid structures:
 * 1. [sdb_command_tokens]          → No pipe command
 * 2. [sdb_command_tokens] | [pipe_cmd] → With pipe command
 * 
 * Rules:
 * - The pipe symbol "|" must appear at most once.
 * - If present, "|" must be followed by exactly one token (the pipe command).
 * - The libsdb command part (before pipe) must be non-empty.
 * 
 * @param tokens Tokenized command strings from tokenize_command().
 * @return std::optional<command_t> Structured command if valid, otherwise std::nullopt.
 */
std::optional<command_t> parse_command(const std::vector<std::string> &tokens);

/**
 * @brief A streambuf implementation that writes to a pipe opened via popen().
 * 
 * This class provides a buffer for writing to a process via popen(). It handles
 * buffer management and synchronization with the pipe.
 */
class popen_output_buffer : public std::streambuf {
    private:
        static constexpr size_t buffer_size = 1024;  ///< Size of the internal buffer
        char buffer[buffer_size];                   ///< Internal character buffer
        FILE* pipe;                                 ///< File pointer to the pipe
    protected:
        int sync() override;
        int_type overflow(int_type ch) override;
    public:
        explicit popen_output_buffer(const std::string& command);
        ~popen_output_buffer() override;
};

/**
 * @brief An output stream that writes to a process via popen().
 * 
 * Provides an ostream interface for writing to a subprocess command.
 */
class popen_ostream : public std::ostream {
    private:
        std::unique_ptr<popen_output_buffer> buffer;  ///< The underlying buffer
    public:
        explicit popen_ostream(const std::string& command);
        friend popen_ostream& operator>>(std::ostream& os, const std::string& command);
};

}

#endif
