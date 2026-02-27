#ifndef LIBVIO_CONSOLE_HH
#define LIBVIO_CONSOLE_HH

#include <cstddef>
#include <cstdint>
#include <istream>
#include <libvio/backend.hh>
#include <libvio/frontend.hh>
#include <libvio/ringbuffer.hh>
#include <optional>
#include <ostream>

namespace libvio {

namespace reqval {
inline static constexpr uint64_t console_rx = 1 << 0; ///< reading from rx
inline static constexpr uint64_t console_tx = 1 << 1; ///< writing to tx
inline static constexpr uint64_t console_prescaler =
    1 << 2; ///< getting or setting prescaler
} // namespace reqval

/**
 * @brief `io_frontend` implementation for console device
 *
 * This class follows the behavior of the uart emulator in NEMU.
 */
class console_frontend : public io_frontend {
public:
  ioreq_t resolve_read(uint64_t offset, width_t width) const override;
  ioreq_t resolve_write(uint64_t offset, width_t width,
                        uint64_t data) const override;
  uint64_t ioctl_get(uint64_t req) override;
  void ioctl_set(uint64_t req, uint64_t value) override;
};

/**
 * @brief Console I/O backend using C++ iostream
 *
 * This backend implements console input/output using standard C++ streams.
 */
class console_backend_iostream : public io_backend {
public:
  console_backend_iostream(std::istream &is, std::ostream &os);
  uint64_t request(uint64_t req) override;
  bool poll(uint64_t req) override;
  bool check(uint64_t req) override;
  void put(uint64_t req, uint64_t data) override;

private:
  std::istream &istream;
  std::ostream &ostream;
  std::optional<uint64_t> input_data = {};
};

} // namespace libvio

#endif
