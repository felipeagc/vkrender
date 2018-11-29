#pragma once

#include <array>
#include <cstring>
#include <exception>

namespace fstl {
template <typename T, size_t N = 8> class fixed_vector {
public:
  fixed_vector() {}

  fixed_vector(size_t count) { this->resize(count); }

  fixed_vector(size_t count, const T &value) {
    this->resize(count);

    for (size_t i = 0; i < count; i++) {
      m_heapVector[i] = value;
    }
  }

  fixed_vector(std::initializer_list<T> const &data) {
    size_t len = data.end() - data.begin();

    this->resize(len);
    m_size = len;

    size_t i = 0;
    for (auto p = data.begin(); p < data.end(); p++) {
      m_heapVector[i++] = *p;
    }
  }

  ~fixed_vector() {
    if (m_heapVector != m_array.data()) {
      delete[] m_heapVector;
    }
  }

  fixed_vector(const fixed_vector &other) {
    this->resize(other.m_size);
    m_size = other.m_size;

    for (size_t i = 0; i < other.m_size; i++) {
      m_heapVector[i] = other[i];
    }
  }

  fixed_vector &operator=(const fixed_vector &other) {
    this->resize(other.m_size);
    m_size = other.m_size;

    for (size_t i = 0; i < other.m_size; i++) {
      m_heapVector[i] = other[i];
    }

    return *this;
  }

  void push_back(const T &value) noexcept {
    if (m_size == m_capacity) {
      T *newStorage = new T[m_capacity * 2];

      for (size_t i = 0; i < m_capacity; i++) {
        newStorage[i] = m_heapVector[i];
      }

      m_capacity *= 2;

      // Delete heap vector if it's not the stack allocated array
      if (m_heapVector != m_array.data()) {
        delete[] m_heapVector;
      }

      m_heapVector = newStorage;
    }

    m_size++;
    m_heapVector[m_size - 1] = value;
  }

  void push_back(T &&value) noexcept {
    if (m_size == m_capacity) {
      T *newStorage = new T[m_capacity * 2];

      for (size_t i = 0; i < m_capacity; i++) {
        newStorage[i] = std::move(m_heapVector[i]);
      }

      m_capacity *= 2;

      // Delete heap vector if it's not the stack allocated array
      if (m_heapVector != m_array.data()) {
        delete[] m_heapVector;
      }

      m_heapVector = newStorage;
    }

    m_size++;
    m_heapVector[m_size - 1] = std::move(value);
  }

  T &operator[](size_t index) const noexcept { return m_heapVector[index]; }

  T &at(size_t index) const {
    if (index >= m_size) {
      throw std::runtime_error("Bad index");
    }
    return m_heapVector[index];
  }

  void resize(size_t newCapacity) noexcept {
    m_size = newCapacity;
    if (newCapacity <= N) {
      if (m_heapVector != m_array.data()) {
        for (size_t i = 0; i < N; i++) {
          m_array[i] = m_heapVector[i];
        }

        delete[] m_heapVector;

        m_heapVector = m_array.data();
        m_capacity = N;
      }
      return;
    }

    T *newStorage = new T[newCapacity];

    size_t minCapacity =
        newCapacity <= m_capacity ? newCapacity : m_capacity;

    for (size_t i = 0; i < minCapacity; i++) {
      newStorage[i] = m_heapVector[i];
    }

    for (size_t i = minCapacity; i < newCapacity; i++) {
      newStorage[i] = T{};
    }

    if (m_heapVector != m_array.data()) {
      delete[] m_heapVector;
    }

    m_heapVector = newStorage;
    m_capacity = newCapacity;
  }

  void clear() noexcept {
    this->resize(0);
  }

  size_t size() const noexcept { return m_size; }

  T *data() const noexcept { return m_heapVector; }

  T *begin() const noexcept { return &m_heapVector[0]; }

  T *end() const noexcept { return &m_heapVector[m_size]; }

protected:
  std::array<T, N> m_array{};
  size_t m_size{0};
  size_t m_capacity{N};
  T *m_heapVector{m_array.data()};
};
} // namespace fstl
