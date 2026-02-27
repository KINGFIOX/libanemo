#include <cstddef>
#include <cstdint>
#include <iostream>
#include <libvio/bus.hh>
#include <string>

namespace libvio {

mmio_device_def::mmio_device_def(io_frontend *frontend, io_backend *backend,
                                 uint64_t addr_begin, uint64_t byte_span)
    : frontend(frontend), backend(backend), addr_begin(addr_begin),
      byte_span(byte_span) {
  this->frontend->backend = this->backend.get();
}

using io_device = std::tuple<io_frontend *, io_backend *, uint64_t, uint64_t>;
io_dispatcher::io_dispatcher(std::initializer_list<io_device> device_list,
                             size_t buffer_size)
    : read_request_buffer(buffer_size), write_request_buffer(buffer_size) {
  devices.reserve(device_list.size());
  for (auto [front, back, addr, size] : device_list) {
    devices.emplace_back(mmio_device_def{front, back, addr, size});
  }
}

std::optional<uint64_t>
io_dispatcher::request_read(uint64_t addr, width_t width, size_t req_no) {
  if (req_no < read_request_buffer.firstindex()) {
    std::cerr << "libvio: Read buffer underflow." << std::endl;
    return {};
  } else if (req_no < read_request_buffer.lastindex()) {
    auto [cached_addr, cached_width, cached_data] = read_request_buffer[req_no];
    if (cached_addr == addr && cached_width == width) {
      return cached_data;
    } else {
      std::cerr << "libvio: Read request mismatch." << std::endl;
      std::cerr << "Cached request: addr=" << std::hex << cached_addr
                << ", width=" << std::to_string(cached_width);
      if (cached_data.has_value()) {
        std::cerr << ", data=" << std::hex << cached_data.value() << std::endl;
      } else {
        std::cerr << ", data=nullopt" << std::endl;
      }
      std::cerr << "New request: addr=" << std::hex << addr
                << ", width=" << std::to_string(width) << std::endl;
      return {};
    }
  } else if (req_no == read_request_buffer.lastindex()) {
    std::optional<uint64_t> req_data = {};
    for (auto &dev : devices) {
      if (addr >= dev.addr_begin && addr < dev.addr_begin + dev.byte_span) {
        req_data = dev.frontend->read(addr - dev.addr_begin, width);
        break;
      }
    }
    read_request_buffer.push_back({addr, width, req_data});
    return req_data;
  } else {
    std::cerr << "libvio: Read buffer overflow." << std::endl;
    return {};
  }
}

bool io_dispatcher::request_write(uint64_t addr, width_t width, size_t req_no,
                                  uint64_t data) {
  if (req_no < write_request_buffer.firstindex()) {
    std::cerr << "libvio: Write buffer underflow." << std::endl;
    return {};
  } else if (req_no < write_request_buffer.lastindex()) {
    auto [cached_addr, cached_width, cached_data, cached_result] =
        write_request_buffer[req_no];
    if (cached_addr == addr && cached_width == width && cached_data == data) {
      return cached_result;
    } else {
      std::cerr << "libvio: Write request mismatch." << std::endl;
      std::cerr << "Cached request: addr=" << std::hex << cached_addr
                << ", width=" << std::to_string(cached_width)
                << ", data=" << std::hex << cached_data
                << ", result=" << cached_result << std::endl;
      std::cerr << "New request: addr=" << std::hex << addr
                << ", width=" << std::to_string(width) << ", data=" << std::hex
                << data << std::endl;
      return {};
    }
  } else if (req_no == write_request_buffer.lastindex()) {
    bool result = false;
    for (auto &dev : devices) {
      if (addr >= dev.addr_begin && addr < dev.addr_begin + dev.byte_span) {
        result = dev.frontend->write(addr - dev.addr_begin, width, data);
        break;
      }
    }
    write_request_buffer.push_back({addr, width, data, result});
    return result;
  } else {
    std::cerr << "libvio: Write buffer overflow." << std::endl;
    return {};
  }
}

mmio_agent *io_dispatcher::new_agent(void) {
  agents.emplace_back(std::unique_ptr<mmio_agent>{new mmio_agent});
  agents.back()->dispatcher = this;
  return agents.back().get();
}

std::optional<uint64_t> mmio_agent::read(uint64_t addr, width_t width) {
  // check if there are already read requests in this cycle
  const auto &buffer = dispatcher->read_request_buffer;
  for (size_t i = old_read_count; i < read_count; ++i) {
    // In some cases, `read` and `write` can be called multiple times by the
    // similator in a cycle In this case, ignore all subsequent requests. But if
    // the same address is accessed in different ways in the same cycle. It
    // would be an error.
    if (addr == std::get<0>(buffer[i])) {
      if (width == std::get<1>(buffer[i])) {
        return std::get<2>(buffer[i]);
      } else {
        std::cerr << "libvio: Read request to the same address with different "
                     "widths in the same cycle."
                  << std::endl;
        std::cerr << "Adderss: 0x" << std::hex << addr << std::endl;
        return {};
      }
    }
  }
  return dispatcher->request_read(addr, width, read_count++);
}

bool mmio_agent::write(uint64_t addr, width_t width, uint64_t data) {
  // check if there are already write requests in this cycle
  const auto &buffer = dispatcher->write_request_buffer;
  for (size_t i = old_write_count; i < write_count; ++i) {
    // In some cases, `read` and `write` can be called multiple times by the
    // similator in a cycle In this case, ignore all subsequent requests. But if
    // the same address is accessed in different ways in the same cycle. It
    // would be an error.
    if (addr == std::get<0>(buffer[i])) {
      if (width == std::get<1>(buffer[i]) && addr == std::get<2>(buffer[i])) {
        return std::get<3>(buffer[i]);
      } else {
        std::cerr << "libvio: Write request to the same address with different "
                     "width or data in the same cycle."
                  << std::endl;
        std::cerr << "Adderss: 0x" << std::hex << addr << std::endl;
        return false;
      }
    }
  }
  return dispatcher->request_write(addr, width, write_count++, data);
}

void mmio_agent::next_cycle(void) {
  old_read_count = read_count;
  old_write_count = write_count;
}

} // namespace libvio