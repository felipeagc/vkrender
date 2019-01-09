#pragma once

#include <cstring>
#include <initializer_list>
#include <stdexcept>

namespace ftl {

template <typename T, size_t N = 1> class vector {
public:
  vector() {}

  vector(size_t count) { this->resize(count); }

  vector(size_t count, const T &value) {
    this->internal_resize(count);

    for (size_t i = 0; i < count; i++) {
      m_vector[i] = value;
    }
  }

  vector(std::initializer_list<T> const &data) {
    size_t len = data.end() - data.begin();

    this->resize(len);
    m_size = len;

    size_t i = 0;
    for (auto p = data.begin(); p < data.end(); p++) {
      m_vector[i++] = *p;
    }
  }

  ~vector() {
    if (m_vector != m_stack_vector) {
      delete[] m_vector;
    }
  }

  vector(const vector &other) {
    this->resize(other.m_size);
    m_size = other.m_size;

    for (size_t i = 0; i < other.m_size; i++) {
      m_vector[i] = other[i];
    }
  }

  vector &operator=(const vector &other) {
    this->resize(other.m_size);
    m_size = other.m_size;

    for (size_t i = 0; i < other.m_size; i++) {
      m_vector[i] = other[i];
    }

    return *this;
  }

  vector(vector &&other) {
    if (m_stack_vector != m_vector) {
      delete[] m_vector;
    }

    if (other.m_vector != other.m_stack_vector) {
      m_vector = other.m_vector;
    } else {
      for (size_t i = 0; i < other.size(); i++) {
        m_stack_vector[i] = std::move(other.m_stack_vector[i]);
      }
    }

    m_capacity = other.m_capacity;
    m_size = other.m_size;
    other.m_vector = other.m_stack_vector;
    other.m_size = 0;
    other.m_capacity = N;
  }

  vector &operator=(vector &&other) {
    if (m_stack_vector != m_vector) {
      delete[] m_vector;
    }

    if (other.m_vector != other.m_stack_vector) {
      m_vector = other.m_vector;
    } else {
      for (size_t i = 0; i < other.size(); i++) {
        m_stack_vector[i] = std::move(other.m_stack_vector[i]);
      }
    }

    m_capacity = other.m_capacity;
    m_size = other.m_size;
    other.m_vector = other.m_stack_vector;
    other.m_size = 0;
    other.m_capacity = N;

    return *this;
  }

  void push_back(const T &value) noexcept {
    if (m_size == m_capacity) {
      T *new_storage = new T[m_capacity * 2];

      for (size_t i = 0; i < m_capacity; i++) {
        new_storage[i] = m_vector[i];
      }

      m_capacity *= 2;

      // Delete heap vector if it's not the stack allocated array
      if (m_vector != m_stack_vector) {
        delete[] m_vector;
      }

      m_vector = new_storage;
    }

    m_size++;
    m_vector[m_size - 1] = value;
  }

  void push_back(T &&value) noexcept {
    if (m_size == m_capacity) {
      T *new_storage = new T[m_capacity * 2];

      for (size_t i = 0; i < m_capacity; i++) {
        new_storage[i] = std::move(m_vector[i]);
      }

      m_capacity *= 2;

      // Delete heap vector if it's not the stack allocated array
      if (m_vector != m_stack_vector) {
        delete[] m_vector;
      }

      m_vector = new_storage;
    }

    m_size++;
    m_vector[m_size - 1] = std::move(value);
  }

  T &operator[](size_t index) const noexcept { return m_vector[index]; }

  T &at(size_t index) const {
    if (index >= m_size) {
      throw std::runtime_error("Bad index");
    }
    return m_vector[index];
  }

  void resize(size_t new_size) noexcept {
    size_t size_before = m_size;
    internal_resize(new_size);

    if (size_before < new_size) {
      for (size_t i = size_before; i < new_size; i++) {
        // Default initialize the members
        m_vector[i] = T{};
      }
    }
  }

  void clear() noexcept { this->resize(0); }

  size_t size() const noexcept { return m_size; }

  T *data() const noexcept { return m_vector; }

  T *begin() const noexcept { return &m_vector[0]; }

  T *end() const noexcept { return &m_vector[m_size]; }

  bool is_small() const noexcept { return m_vector == m_stack_vector; }

protected:
  // Does not initialize elements
  void internal_resize(size_t new_size) noexcept {
    m_size = new_size;
    if (new_size <= N) {
      if (m_vector != m_stack_vector) {
        for (size_t i = 0; i < N; i++) {
          m_stack_vector[i] = m_vector[i];
        }

        delete[] m_vector;

        m_vector = m_stack_vector;
        m_capacity = N;
      }
      return;
    }

    T *new_storage = new T[new_size];

    size_t min_capacity = new_size <= m_capacity ? new_size : m_capacity;

    for (size_t i = 0; i < min_capacity; i++) {
      new_storage[i] = m_vector[i];
    }

    for (size_t i = min_capacity; i < new_size; i++) {
      new_storage[i] = T{};
    }

    if (m_vector != m_stack_vector) {
      delete[] m_vector;
    }

    m_vector = new_storage;
    m_capacity = new_size;
  }

  T m_stack_vector[N];
  size_t m_size = 0;
  size_t m_capacity = N;
  T *m_vector = m_stack_vector;
};

template <typename T, size_t N = 8> using small_vector = vector<T, N>;
} // namespace ftl
