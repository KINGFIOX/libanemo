#ifndef LIBVIO_RINGBUFFER_HH
#define LIBVIO_RINGBUFFER_HH

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace libvio {

template <typename T> class ringbuffer;

template <typename T> class ringbuffer_iterator;

template <typename T> class ringbuffer_const_iterator;

/**
 * @brief A circular buffer (ring-buffer) implementation with efficient access
 * patterns.
 *
 * This class implements a fixed-size circular buffer that allows efficient
 * addition and removal of elements at both ends. The buffer maintains its
 * elements in a contiguous storage, with indices wrapping around at the
 * boundaries.
 *
 * @note This implementation intentionally omits push_front/pop_front
 * operations. When multiple consumers access the buffer concurrently, each
 * consumer should maintain its own front index.
 *
 * @tparam T The type of elements stored in the buffer. Must be
 * copy-constructible and assignable.
 */
template <typename T> class ringbuffer {

private:
  T *buffer;          ///< Pointer to the underlying storage array
  size_t max_size;    ///< Maximum capacity of the buffer (never changes after
                      ///< construction)
  size_t first_index; ///< Index of the first valid element in the buffer
  size_t last_index;  ///< Index of the next available position in the buffer

public:
  using iterator = ringbuffer_iterator<T>;
  using const_iterator = ringbuffer_const_iterator<T>;

  /**
   * @brief Constructs a ringbuffer with the specified capacity.
   *
   * @param n The number of elements that can be stored in the buffer.
   */
  ringbuffer(size_t n) {
    buffer = new T[n];
    max_size = n;
    first_index = 0;
    last_index = 0;
  }

  ~ringbuffer() { delete[] buffer; }

  ringbuffer(const ringbuffer &other) {
    buffer = new T[other.max_size];
    max_size = other.max_size;
    first_index = other.first_index;
    last_index = other.last_index;
    std::copy(other.buffer, other.buffer + max_size, buffer);
  }

  ringbuffer(ringbuffer &&other) {
    buffer = other.buffer;
    max_size = other.max_size;
    first_index = other.first_index;
    last_index = other.last_index;
    other.buffer = nullptr;
    other.max_size = 0;
    other.first_index = 0;
    other.last_index = 0;
  }

  ringbuffer<T> &operator=(const ringbuffer<T> &other) {
    if (this != &other) {
      delete[] buffer;
      buffer = new T[other.max_size];
      max_size = other.max_size;
      first_index = other.first_index;
      last_index = other.last_index;
      std::copy(other.buffer, other.buffer + max_size, buffer);
    }
    return *this;
  }

  /**
   * @brief Gets the index of the first valid element in the buffer.
   *
   * This index can be used to track the logical start of the buffer.
   *
   * @return The first index
   */
  size_t firstindex() const { return first_index; }

  /**
   * @brief Gets the index of the next available position in the buffer.
   *
   * This index marks the position where the next element will be added.
   *
   * @return The last index
   */
  size_t lastindex() const { return last_index; }

  T &operator[](size_t index) { return buffer[index % max_size]; }

  const T &operator[](size_t index) const { return buffer[index % max_size]; }

  /**
   * @brief Adds an element to the end of the buffer.
   *
   * If the buffer is full, the oldest element (at the front) is overwritten.
   *
   * @param value The element to add
   */
  void push_back(const T &value) {
    buffer[last_index % max_size] = value;
    ++last_index;
    first_index = (last_index - first_index) > max_size ? last_index - max_size
                                                        : first_index;
  }

  /**
   * @brief Removes the last element from the buffer.
   *
   * If the buffer is empty, this operation has no effect.
   */
  void pop_back(void) {
    last_index = last_index > first_index ? last_index - 1 : first_index;
  }

  /*
  `push_front` and `pop_front` are intentinally not provided.
  The same ringbuffer will be read by multiple consumers in some cases.
  Each consumer should maintain its own front index rather than rely on the
  state of the ringbuffer.
  */

  /**
   * @brief Gets the maximum number of elements the buffer can hold.
   *
   * @return The buffer capacity
   */
  constexpr size_t capacity() const { return max_size; }

  /**
   * @brief Gets the current number of elements in the buffer.
   *
   * @return The number of elements (last_index - first_index)
   */
  size_t size() const { return last_index - first_index; }

  /**
   * @brief Checks if the buffer contains no elements.
   *
   * @return true if size() == 0, false otherwise
   */
  bool empty() const { return first_index == last_index; }

  /**
   * @brief Returns an iterator to the first element in the buffer.
   *
   * @return An iterator pointing to the first element
   */
  iterator begin() { return iterator(buffer, max_size, first_index); }

  /**
   * @brief Returns an iterator to the element following the last element.
   *
   * @return An iterator pointing to the past-the-end element
   */
  iterator end() { return iterator(buffer, max_size, last_index); }

  /**
   * @brief Returns a const iterator to the first element in the buffer.
   *
   * @return A const iterator pointing to the first element
   */
  const_iterator begin() const {
    return const_iterator(buffer, max_size, first_index);
  }

  /**
   * @brief Returns a const iterator to the element following the last element.
   *
   * @return A const iterator pointing to the past-the-end element
   */
  const_iterator end() const {
    return const_iterator(buffer, max_size, last_index);
  }

  /**
   * @brief Returns a const iterator to the first element in the buffer.
   *
   * @return A const iterator pointing to the first element
   */
  const_iterator cbegin() const { return begin(); }

  /**
   * @brief Returns a const iterator to the element following the last element.
   *
   * @return A const iterator pointing to the past-the-end element
   */
  const_iterator cend() const { return end(); }
};

template <typename T> class ringbuffer_iterator {
  friend class ringbuffer_const_iterator<T>;

private:
  T *buffer;
  size_t max_size;
  size_t index;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T *;
  using reference = T &;

  ringbuffer_iterator(T *buffer, size_t max_size, size_t index)
      : buffer(buffer), max_size(max_size), index(index) {}

  // Increment/decrement
  ringbuffer_iterator &operator++() {
    ++index;
    return *this;
  }
  ringbuffer_iterator operator++(int) {
    ringbuffer_iterator tmp = *this;
    ++index;
    return tmp;
  }
  ringbuffer_iterator &operator--() {
    --index;
    return *this;
  }
  ringbuffer_iterator operator--(int) {
    ringbuffer_iterator tmp = *this;
    --index;
    return tmp;
  }

  // Arithmetic
  ringbuffer_iterator operator+(difference_type n) const {
    return ringbuffer_iterator(buffer, max_size, index + n);
  }
  ringbuffer_iterator operator-(difference_type n) const {
    return ringbuffer_iterator(buffer, max_size, index - n);
  }
  ringbuffer_iterator &operator+=(difference_type n) {
    index += n;
    return *this;
  }
  ringbuffer_iterator &operator-=(difference_type n) {
    index -= n;
    return *this;
  }
  friend ringbuffer_iterator operator+(difference_type n,
                                       const ringbuffer_iterator &it) {
    return it + n;
  }

  // Difference
  difference_type operator-(const ringbuffer_iterator &other) const {
    return static_cast<difference_type>(index) -
           static_cast<difference_type>(other.index);
  }

  // Access
  T &operator*() const { return buffer[index % max_size]; }
  T &operator[](difference_type n) const { return *(*this + n); }

  // Comparison
  bool operator==(const ringbuffer_iterator &other) const {
    return index == other.index && buffer == other.buffer;
  }
  bool operator!=(const ringbuffer_iterator &other) const {
    return !(*this == other);
  }
  bool operator<(const ringbuffer_iterator &other) const {
    return index < other.index;
  }
  bool operator<=(const ringbuffer_iterator &other) const {
    return index <= other.index;
  }
  bool operator>(const ringbuffer_iterator &other) const {
    return index > other.index;
  }
  bool operator>=(const ringbuffer_iterator &other) const {
    return index >= other.index;
  }
};

template <typename T> class ringbuffer_const_iterator {
  friend class ringbuffer_iterator<T>;

private:
  const T *buffer;
  size_t max_size;
  size_t index;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = const T;
  using difference_type = std::ptrdiff_t;
  using pointer = const T *;
  using reference = const T &;

  ringbuffer_const_iterator(const ringbuffer_iterator<T> &other)
      : buffer(other.buffer), max_size(other.max_size), index(other.index) {}
  ringbuffer_const_iterator(const T *buffer, size_t max_size, size_t index)
      : buffer(buffer), max_size(max_size), index(index) {}

  // Increment/decrement
  ringbuffer_const_iterator &operator++() {
    ++index;
    return *this;
  }
  ringbuffer_const_iterator operator++(int) {
    ringbuffer_const_iterator tmp = *this;
    ++index;
    return tmp;
  }
  ringbuffer_const_iterator &operator--() {
    --index;
    return *this;
  }
  ringbuffer_const_iterator operator--(int) {
    ringbuffer_const_iterator tmp = *this;
    --index;
    return tmp;
  }

  // Arithmetic
  ringbuffer_const_iterator operator+(difference_type n) const {
    return ringbuffer_const_iterator(buffer, max_size, index + n);
  }
  ringbuffer_const_iterator operator-(difference_type n) const {
    return ringbuffer_const_iterator(buffer, max_size, index - n);
  }
  ringbuffer_const_iterator &operator+=(difference_type n) {
    index += n;
    return *this;
  }
  ringbuffer_const_iterator &operator-=(difference_type n) {
    index -= n;
    return *this;
  }
  friend ringbuffer_const_iterator
  operator+(difference_type n, const ringbuffer_const_iterator &it) {
    return it + n;
  }

  // Difference
  difference_type operator-(const ringbuffer_const_iterator &other) const {
    return static_cast<difference_type>(index) -
           static_cast<difference_type>(other.index);
  }

  // Access
  const T &operator*() const { return buffer[index % max_size]; }
  const T &operator[](difference_type n) const { return *(*this + n); }

  // Comparison
  bool operator==(const ringbuffer_const_iterator &other) const {
    return index == other.index && buffer == other.buffer;
  }
  bool operator!=(const ringbuffer_const_iterator &other) const {
    return !(*this == other);
  }
  bool operator<(const ringbuffer_const_iterator &other) const {
    return index < other.index;
  }
  bool operator<=(const ringbuffer_const_iterator &other) const {
    return index <= other.index;
  }
  bool operator>(const ringbuffer_const_iterator &other) const {
    return index > other.index;
  }
  bool operator>=(const ringbuffer_const_iterator &other) const {
    return index >= other.index;
  }
};

} // namespace libvio

#endif
