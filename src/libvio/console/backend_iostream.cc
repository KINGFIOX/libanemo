#include <cstdint>
#include <libvio/console.hh>

namespace libvio {

console_backend_iostream::console_backend_iostream(std::istream &is, std::ostream &os): istream(is), ostream(os) {}

uint64_t console_backend_iostream::request(uint64_t req) {
    if (req != reqval::console_rx) {
        return 0;
    }

    if (input_data.has_value()) {
        uint64_t data = input_data.value();
        input_data = {};
        return data;
    } else {
        return istream.get();
    }
}

bool console_backend_iostream::poll(uint64_t req) {
    if (req != reqval::console_rx) {
        return true;
    }

    if (input_data.has_value()) {
        return true;
    } else {
        input_data = istream.get();
        return true;
    }
}

bool console_backend_iostream::check(uint64_t req) {
    if (req != reqval::console_rx) {
        return true;
    }
    
    return input_data.has_value();
}

void console_backend_iostream::put(uint64_t req, uint64_t data) {
    if (req == reqval::console_tx) {
        ostream << static_cast<char>(data);
    }
}

}