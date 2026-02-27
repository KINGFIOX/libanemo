#include <cstdint>
#include <libvio/frontend.hh>
#include <libvio/mtime.hh>

namespace libvio {

ioreq_t mtime_frontend::resolve_read(size_t offset, width_t width) const {
  if (width == width_t::dword) {
    if (offset == 0) {
      // reading mtime
      return {ioreq_type_t::read, reqval::mtime_h | reqval::mtime_l};
    } else if (offset == 8) {
      // reading mtimecmp
      return {ioreq_type_t::read, reqval::mtimecmp_h | reqval::mtimecmp_l};
      ;
    } else {
      return {ioreq_type_t::invalid, 0};
    }
  } else if (width == width_t::word) {
    if (offset == 0) {
      return {ioreq_type_t::read, reqval::mtime_l};
    } else if (offset == 4) {
      return {ioreq_type_t::read, reqval::mtime_h};
    } else if (offset == 8) {
      return {ioreq_type_t::read, reqval::mtimecmp_l};
    } else if (offset == 12) {
      return {ioreq_type_t::read, reqval::mtimecmp_l};
    } else {
      return {ioreq_type_t::invalid, 0};
    }
  } else {
    return {ioreq_type_t::invalid, 0};
  }
}

ioreq_t mtime_frontend::resolve_write(size_t offset, width_t width,
                                      uint64_t data) const {
  if (width == width_t::dword) {
    if (offset == 0) {
      // mtime
      return {ioreq_type_t::write, reqval::mtime_h | reqval::mtime_l};
    } else if (offset == 8) {
      // mtimecmp
      return {ioreq_type_t::write, reqval::mtimecmp_h | reqval::mtimecmp_l};
      ;
    } else {
      return {ioreq_type_t::invalid, 0};
    }
  } else if (width == width_t::word) {
    if (offset == 0) {
      return {ioreq_type_t::write, reqval::mtime_l};
    } else if (offset == 4) {
      return {ioreq_type_t::write, reqval::mtime_h};
    } else if (offset == 8) {
      return {ioreq_type_t::write, reqval::mtimecmp_l};
    } else if (offset == 12) {
      return {ioreq_type_t::write, reqval::mtimecmp_l};
    } else {
      return {ioreq_type_t::invalid, 0};
    }
  } else {
    return {ioreq_type_t::invalid, 0};
  }
}

uint64_t mtime_frontend::ioctl_get(uint64_t req) { return 0; }

void mtime_frontend::ioctl_set(uint64_t req, uint64_t data) { return; }

} // namespace libvio