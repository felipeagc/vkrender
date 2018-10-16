#pragma once

#include <array>
#include <cstring>
#include <exception>

namespace vkr {
template <typename T, size_t N = 8> class SmallVec {
public:
  SmallVec() {}

  SmallVec(size_t count) { this->resize(count); }

  SmallVec(size_t count, const T &value) {
    this->resize(count);

    for (size_t i = 0; i < count; i++) {
      this->m_heapVector[i] = value;
    }
  }

  SmallVec(std::initializer_list<T> const &data) {
    size_t len = data.end() - data.begin();

    this->resize(len);
    this->m_size = len;

    size_t i = 0;
    for (auto p = data.begin(); p < data.end(); p++) {
      this->m_heapVector[i++] = *p;
    }
  }

  ~SmallVec() {
    if (m_heapVector != m_array.data()) {
      delete[] m_heapVector;
    }
  }

  SmallVec(const SmallVec &other) {
    this->resize(other.m_size);
    this->m_size = other.m_size;

    for (size_t i = 0; i < other.m_size; i++) {
      this->m_heapVector[i] = other[i];
    }
  }

  SmallVec &operator=(const SmallVec &other) {
    this->resize(other.m_size);
    this->m_size = other.m_size;

    for (size_t i = 0; i < other.m_size; i++) {
      this->m_heapVector[i] = other[i];
    }

    return *this;
  }

  void push_back(const T &value) noexcept {
    if (this->m_size == this->m_capacity) {
      T *newStorage = new T[this->m_capacity * 2];
      
      for (size_t i = 0; i < this->m_capacity; i++) {
        newStorage[i] = this->m_heapVector[i];
      }

      this->m_capacity *= 2;

      // Delete heap vector if it's not the stack allocated array
      if (this->m_heapVector != this->m_array.data()) {
        delete[] this->m_heapVector;
      }

      this->m_heapVector = newStorage;
    }

    this->m_size++;
    this->m_heapVector[this->m_size-1] = value;
  }

  T &operator[](size_t index) const noexcept { return m_heapVector[index]; }

  T &at(size_t index) const {
    if (index >= m_size) {
      throw std::runtime_error("Bad index");
    }
    return m_heapVector[index];
  }

  void resize(size_t newCapacity) noexcept {
    this->m_size = newCapacity;
    if (newCapacity <= N) {
      if (m_heapVector != m_array.data()) {
        for (size_t i = 0; i < N; i++) {
          m_array[i] = m_heapVector[i];
        }

        delete[] m_heapVector;

        m_heapVector = m_array.data();
        this->m_capacity = N;
      }
      return;
    }

    T *newStorage = new T[newCapacity];

    size_t minCapacity =
        newCapacity <= this->m_capacity ? newCapacity : this->m_capacity;

    for (size_t i = 0; i < minCapacity; i++) {
      newStorage[i] = this->m_heapVector[i];
    }

    for (size_t i = minCapacity; i < newCapacity; i++) {
      newStorage[i] = T{};
    }

    if (m_heapVector != m_array.data()) {
      delete[] m_heapVector;
    }

    m_heapVector = newStorage;
    this->m_capacity = newCapacity;
  }

  size_t size() const noexcept { return this->m_size; }

  T *data() const noexcept { return m_heapVector; }

  T *begin() const noexcept { return &m_heapVector[0]; }

  T *end() const noexcept { return &m_heapVector[this->m_size]; }

protected:
  std::array<T, N> m_array{};
  size_t m_size{0};
  size_t m_capacity{N};
  T *m_heapVector{m_array.data()};
};
} // namespace vkr
