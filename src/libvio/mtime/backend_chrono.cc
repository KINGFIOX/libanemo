#include <chrono>
#include <cstdint>
#include <libvio/backend.hh>
#include <libvio/mtime.hh>

namespace libvio {

mtime_backend_chrono::mtime_backend_chrono(void) {
  mtime_offset = std::chrono::high_resolution_clock::now();
}

uint64_t mtime_backend_chrono::request(uint64_t req) {
  if (req == reqval::mtimecmp_l) {
    return mtimecmp & 0x00000000ffffffff;
  }
  if (req == reqval::mtimecmp_h) {
    return mtimecmp >> 32;
  }
  if (req == (reqval::mtimecmp_h | reqval::mtimecmp_l)) {
    return mtimecmp;
  }
  // get current time
  auto now = std::chrono::high_resolution_clock::now();
  auto duration = now - mtime_offset;
  uint64_t microseconds =
      std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  if (req == reqval::mtime_l) {
    return microseconds & 0x00000000ffffffff;
  }
  if (req == reqval::mtime_h) {
    return microseconds >> 32;
  }
  if (req == (reqval::mtime_h | reqval::mtime_l)) {
    return microseconds;
  }
  return 0;
}

void mtime_backend_chrono::put(uint64_t req, uint64_t data) {
  if (req == reqval::mtimecmp_l) {
    mtimecmp = (mtimecmp & 0xffffffff00000000) | (data & 0x00000000ffffffff);
    return;
  }
  if (req == reqval::mtimecmp_h) {
    mtimecmp = (data << 32) | (mtimecmp & 0x00000000ffffffff);
    return;
  }
  if (req == (reqval::mtimecmp_h | reqval::mtimecmp_l)) {
    mtimecmp = data;
    return;
  }
  // update mtime offset when trying to write to mtime
  // so that if mtime is read just afterwards
  // it will return the value written
  auto now = std::chrono::high_resolution_clock::now();
  uint64_t new_mtime = 0;
  if (req & reqval::mtime_l) {
    new_mtime |= data & 0x00000000ffffffff;
  }
  if (req & reqval::mtime_h) {
    new_mtime |= uint64_t(data) << 32;
  }
  mtime_offset = now - std::chrono::microseconds{new_mtime};
}

bool libvio::mtime_backend_chrono::poll(uint64_t req) { return true; }

bool libvio::mtime_backend_chrono::check(uint64_t req) { return true; }

} // namespace libvio