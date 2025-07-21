#include <cstdint>
#include <libvio/backend.hh>
#include <libvio/frontend.hh>
#include <libvio/width.hh>
#include <optional>
#include <iostream>
#include <ostream>

namespace libvio {

std::optional<uint64_t> io_frontend::read(uint64_t offset, width_t width) {
    std::optional<uint64_t> read_data;
    auto req = resolve_read(offset, width);
    switch (req.type) {
        case ioreq_type_t::read:
            read_data = backend->request(req.req);
            break;
        case ioreq_type_t::poll_in:
            read_data = backend->poll(req.req);
            break;
        case ioreq_type_t::poll_out:
            read_data = 1;
            break;
        case ioreq_type_t::ioctl_get:
            read_data = ioctl_get(req.req);
            break;
        case ioreq_type_t::ioctl_set:
        case ioreq_type_t::write:
            std::cerr << "MMIO read resolved as write request types." << std::endl;
        case ioreq_type_t::invalid:
            read_data = {};
            break;
    }
    // make sure the higher bits are set to zero
    if (read_data.has_value()) {
        read_data = zero_truncate<uint64_t>(read_data.value(), width);
    }
    return read_data;
}

bool io_frontend::write(uint64_t offset, width_t width, uint64_t data) {
    auto req = resolve_write(offset, width, data);
    write_data = data;
    switch (req.type) {
        case ioreq_type_t::write:
            backend->put(req.req, data);
            write_result = true;
            break;
        case ioreq_type_t::ioctl_set:
            ioctl_set(req.req, data);
            write_result = true;
            break;
        case ioreq_type_t::read:
        case ioreq_type_t::poll_in:
        case ioreq_type_t::poll_out:
        case ioreq_type_t::ioctl_get:
            std::cerr << "MMIO write resolved as read request types." << std::endl;
        case ioreq_type_t::invalid:
            write_result = false;
            break;
    }
    return write_result;
}

}