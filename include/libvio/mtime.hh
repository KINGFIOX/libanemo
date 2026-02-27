/**
 * @file mtime.hh
 * @brief RISC-V system timer (mtime) frontend interface
 */
#ifndef LIBVIO_MTIME_HH
#define LIBVIO_MTIME_HH

#include <chrono>
#include <cstdint>
#include <libvio/backend.hh>
#include <libvio/frontend.hh>

namespace libvio {

namespace reqval {
inline static constexpr uint64_t mtime_l =
    1 << 0; ///< Reading lower part of mtime register
inline static constexpr uint64_t mtime_h =
    1 << 1; ///< Reading higher part of mtime register
inline static constexpr uint64_t mtimecmp_l =
    1 << 2; ///< Reading/writing lower part of mtimecmp register
inline static constexpr uint64_t mtimecmp_h =
    1 << 3; ///< Reading/writing higher part of mtimecmp register
} // namespace reqval

/**
 * @brief `io_frontend` implementation for RISC-V system timer (mtime/mtimecmp)
 *
 * This class handles the memory-mapped accesses to the mtime and mtimecmp
 * registers as defined in the RISC-V privileged specification.
 */
class mtime_frontend : public io_frontend {
public:
  ioreq_t resolve_read(uint64_t offset, width_t width) const override;
  ioreq_t resolve_write(uint64_t offset, width_t width,
                        uint64_t data) const override;
  uint64_t ioctl_get(uint64_t req) override;
  void ioctl_set(uint64_t req, uint64_t value) override;
};

/**
 * @brief Timer backend implementation using std::chrono
 *
 * This backend provides the actual timer functionality using C++'s chrono
 * library. It implements the mtime and mtimecmp functionality as specified in
 * RISC-V.
 */
class mtime_backend_chrono : public io_backend {
public:
  mtime_backend_chrono(void);
  uint64_t request(uint64_t req) override;
  bool poll(uint64_t req) override;
  bool check(uint64_t req) override;
  void put(uint64_t req, uint64_t data) override;

private:
  // 我叫达拉崩吧斑得贝迪卜多比鲁翁
  std::chrono::high_resolution_clock::time_point mtime_offset;
  uint64_t mtimecmp;
};

} // namespace libvio

#endif
