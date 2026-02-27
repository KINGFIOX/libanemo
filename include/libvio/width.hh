/**
 * @file width.hh
 * @brief Provides width-related operations for integer types
 */

#ifndef LIBVIO_WIDTH_HH
#define LIBVIO_WIDTH_HH

#include <cstdint>
#include <string>

namespace libvio {

/**
 * @enum width_t
 * @brief Enumeration representing different data widths
 */
enum class width_t {
  byte = 1, ///< 1-byte width (8 bits)
  half = 2, ///< 2-byte width (16 bits)
  word = 4, ///< 4-byte width (32 bits)
  dword = 8 ///< 8-byte width (64 bits)
};

/**
 * @brief Truncates a value to the specified width by zeroing upper bits
 * @tparam WORD_T The word type (uint32_t or uint64_t)
 * @param value The input value to truncate
 * @param width The target width to truncate to
 * @return The truncated value with upper bits zeroed
 */
template <typename WORD_T>
constexpr WORD_T zero_truncate(WORD_T value, width_t width);

/**
 * @brief Specialization of zero_truncate for uint32_t
 * @param value The 32-bit input value to truncate
 * @param width The target width to truncate to
 * @return The truncated 32-bit value with upper bits zeroed
 */
template <>
constexpr uint32_t zero_truncate<uint32_t>(uint32_t value, width_t width) {
  return value & ((1ull << (8 * static_cast<uint32_t>(width))) - 1);
}

/**
 * @brief Specialization of zero_truncate for uint64_t
 * @param value The 64-bit input value to truncate
 * @param width The target width to truncate to
 * @return The truncated 64-bit value with upper bits zeroed
 */
template <>
constexpr uint64_t zero_truncate<uint64_t>(uint64_t value, width_t width) {
  return value & ((1ull << (8 * static_cast<uint64_t>(width))) - 1);
}

/**
 * @brief Sign-extends a value to the full width of the type
 * @tparam WORD_T The word type (uint32_t or uint64_t)
 * @param value The input value to sign-extend
 * @param width The original width of the input value
 * @return The sign-extended value
 */
template <typename WORD_T>
constexpr WORD_T sign_extend(WORD_T value, width_t width);

/**
 * @brief Specialization of sign_extend for uint32_t
 * @param value The 32-bit input value to sign-extend
 * @param width The original width of the input value
 * @return The sign-extended 32-bit value
 */
template <>
constexpr uint32_t sign_extend<uint32_t>(uint32_t value, width_t width) {
  switch (width) {
  case width_t::byte:
    return uint32_t(int32_t(int8_t(value)));
  case width_t::half:
    return uint32_t(int32_t(int16_t(value)));
  default:
    return value;
  }
}

/**
 * @brief Specialization of sign_extend for uint64_t
 * @param value The 64-bit input value to sign-extend
 * @param width The original width of the input value
 * @return The sign-extended 64-bit value
 */
template <>
constexpr uint64_t sign_extend<uint64_t>(uint64_t value, width_t width) {
  switch (width) {
  case width_t::byte:
    return uint64_t(int64_t(int8_t(value)));
  case width_t::half:
    return uint64_t(int64_t(int16_t(value)));
  case width_t::word:
    return uint64_t(int64_t(int32_t(value)));
  default:
    return value;
  }
}

} // namespace libvio

namespace std {

/**
 * @brief Converts a width_t enum value to its string representation
 * @param width The width enum value to convert
 * @return String representation of the width
 */
inline string to_string(libvio::width_t width) noexcept {
  switch (width) {
  case libvio::width_t::byte:
    return "byte";
  case libvio::width_t::half:
    return "half";
  case libvio::width_t::word:
    return "word";
  case libvio::width_t::dword:
    return "dword";
  default:
    return "unknown";
  }
}

} // namespace std

#endif
