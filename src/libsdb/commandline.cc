#include <libsdb/commandline.hh>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace libsdb {

std::optional<std::vector<std::string>> tokenize_command(std::string command) {
    bool quote = false;   // Flag for inside quoted section
    bool esc = false;     // Flag for escape character mode
    std::vector<std::string> tokens{};  // Result token storage
    std::string current_token = "";     // Current token being built
    for (auto c : command) {  // Process each character
        if (esc) {
            // Escape mode: add character literally and reset escape flag
            current_token += c;
            esc = false;
        } else {
            if (c == '\\') {
                // Start escape sequence (next character will be literal)
                esc = true;
            } else if (c == '"') {
                // Toggle quoting mode (don't add quote to token)
                quote = !quote;
            } else if (c == ' ') {
                if (quote) {
                    // In quotes: preserve space as part of token
                    current_token += c;
                } else if (!current_token.empty())  {
                    // Outside quotes: finalize current token
                    tokens.push_back(current_token);
                    current_token.clear();  // Reset for next token
                }
            } else {
                // Regular character: add to current token
                current_token += c;
            }
        }
    }
    // Add any remaining token content after processing all characters
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    // unclosed quote or trailing escape is an error
    if (quote || esc) {
        return {};
    } else {
        return tokens;
    }
}

std::optional<command_t> parse_command(const std::vector<std::string> &tokens) {
    auto pipe_index = std::find(tokens.begin(), tokens.end(), "|");
    std::vector<std::string> sdb_command_args{tokens.begin(), pipe_index};
    // There can be no pipe
    bool no_pipe = pipe_index==tokens.end();
    // If present, "|" must be followed by exactly one token
    bool valid_pipe = (tokens.end()-pipe_index)==2;
    // The args is optional, but the command itself cannot be missing
    bool valid_command = (pipe_index-tokens.begin()) != 0;
    std::string command;
    std::vector<std::string> args;
    std::optional<std::string> pipe_command = {};
    if (valid_command) {
        command = tokens[0];
        args = {std::next(tokens.begin()), pipe_index};
    }
    if (valid_pipe) {
        pipe_command = *std::next(pipe_index);
    }
    if (valid_command && (valid_pipe||no_pipe)) {
        return {{command, args, pipe_command}};
    } else {
        return {};
    }
}

int popen_output_buffer::sync() {
    if (pptr() > pbase()) {
        size_t size = pptr() - pbase();
        if (fwrite(pbase(), 1, size, pipe) != size) {
            return -1;  // Write error occurred
        }
        fflush(pipe);
        pbump(-static_cast<int>(size));  // Reset buffer pointers
    }
    return 0;
}

popen_output_buffer::int_type popen_output_buffer::overflow(int_type ch) {
    if (ch != traits_type::eof()) {
        *pptr() = ch;
        pbump(1);
        if (sync() != 0) {
            return traits_type::eof();
        }
    }
    return ch;
}

popen_output_buffer::popen_output_buffer(const std::string& command) 
    : pipe(popen(command.c_str(), "w")) {
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    setp(buffer, buffer + buffer_size - 1);  // Set buffer pointers
}

popen_output_buffer::~popen_output_buffer() {
    sync();  // Flush any remaining data
    if (pipe) {
        pclose(pipe);
    }
}

popen_ostream::popen_ostream(const std::string& command)
    : std::ostream(nullptr),
      buffer(std::make_unique<popen_output_buffer>(command)) {
    rdbuf(buffer.get());  // Set the underlying buffer
}

popen_ostream& operator>>(std::ostream& os, const std::string& command) {
    // Ignore input stream, create new stream with the command
    static popen_ostream popen_stream(command);
    return popen_stream;
}

}
