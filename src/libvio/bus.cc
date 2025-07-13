#include <libvio/bus.hh>

namespace libvio {

mmio_device_def::mmio_device_def(io_frontend *frontend, io_backend *backend, uint64_t addr_begin, uint64_t byte_span) :frontend(frontend), backend(backend), addr_begin(addr_begin), byte_span(byte_span) {
    this->frontend->backend = this->backend.get();
}


bus::bus(std::initializer_list<std::tuple<io_frontend*, io_backend*, uint64_t, uint64_t>> device_list) {
    devices.reserve(device_list.size());
    for (auto def: device_list) {
        devices.emplace_back(mmio_device_def{std::get<0>(def), std::get<1>(def), std::get<2>(def), std::get<3>(def)});
    }
}

std::optional<uint64_t> bus::read(uint64_t addr, width_t width) {
    for (auto &dev: devices) {
        if (addr>=dev.addr_begin && addr<dev.addr_begin+dev.byte_span) {
            return dev.frontend->read(addr-dev.addr_begin, width);
        }
    }
    return {};
}

bool bus::write(uint64_t addr, width_t width, uint64_t data) {
    for (auto &dev: devices) {
        if (addr>=dev.addr_begin && addr<dev.addr_begin+dev.byte_span) {
            return dev.frontend->write(addr-dev.addr_begin, width, data);
        }
    }
    return false;
}

void bus::next_cycle(void) {
    for (auto &dev: devices) {
        dev.frontend->next_cycle();
    }
}

}