#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fstl {

template <typename T> class array_proxy {
public:
  constexpr array_proxy(std::nullptr_t) : m_count(0), m_ptr(nullptr) {}

  array_proxy(T &ptr) : m_count(1), m_ptr(&ptr) {}

  array_proxy(uint32_t count, T *ptr) : m_count(count), m_ptr(ptr) {}

  template <size_t N>
  array_proxy(std::array<typename std::remove_const<T>::type, N> &data)
      : m_count(N), m_ptr(data.data()) {}

  template <size_t N>
  array_proxy(std::array<typename std::remove_const<T>::type, N> const &data)
      : m_count(N), m_ptr(data.data()) {}

  template <
      class Allocator = std::allocator<typename std::remove_const<T>::type>>
  array_proxy(std::vector<typename std::remove_const<T>::type, Allocator> &data)
      : m_count(static_cast<uint32_t>(data.size())), m_ptr(data.data()) {}

  template <
      class Allocator = std::allocator<typename std::remove_const<T>::type>>
  array_proxy(
      std::vector<typename std::remove_const<T>::type, Allocator> const &data)
      : m_count(static_cast<uint32_t>(data.size())), m_ptr(data.data()) {}

  array_proxy(std::initializer_list<T> const &data)
      : m_count(static_cast<uint32_t>(data.end() - data.begin())),
        m_ptr(data.begin()) {}

  const T *begin() const { return m_ptr; }

  const T *end() const { return m_ptr + m_count; }

  const T &front() const {
    assert(m_count && m_ptr);
    return *m_ptr;
  }

  const T &back() const {
    assert(m_count && m_ptr);
    return *(m_ptr + m_count - 1);
  }

  bool empty() const { return (m_count == 0); }

  uint32_t size() const { return m_count; }

  T *data() const { return m_ptr; }

private:
  uint32_t m_count;
  T *m_ptr;
};
} // namespace fstl
