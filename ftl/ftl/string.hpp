#pragma once

#include <cstddef>
#include <cstring>

namespace ftl {

template <size_t S> struct basic_string {
  basic_string() { m_buf[0] = '\0'; }

  basic_string(const basic_string &other) {
    this->ensure_size(other.m_length);
    strncpy(m_buf, other.m_buf, other.m_length + 1);
    m_length = other.m_length;
  }
  basic_string &operator=(const basic_string &other) {
    this->ensure_size(other.m_length);
    strncpy(m_buf, other.m_buf, other.m_length + 1);
    m_length = other.m_length;
    return *this;
  }

  basic_string(basic_string &&other) {
    if (other.m_buf != other.m_smallbuf) {
      m_buf = other.m_buf;
    } else {
      strncpy(m_buf, other.m_buf, other.m_length + 1);
    }

    m_length = other.m_length;
    m_capacity = other.m_capacity;

    other.m_buf = other.m_smallbuf;
    other.m_length = 0;
    other.m_capacity = 0;
  }

  basic_string &operator=(basic_string &&other) {
    if (other.m_buf != other.m_smallbuf) {
      m_buf = other.m_buf;
    } else {
      strncpy(m_buf, other.m_buf, other.m_length + 1);
    }

    m_length = other.m_length;
    m_capacity = other.m_capacity;

    other.m_buf = other.m_smallbuf;
    other.m_length = 0;
    other.m_capacity = 0;

    return *this;
  }

  ~basic_string() {
    if (m_buf != m_smallbuf) {
      delete[] m_buf;
    }
  }

  basic_string(const char *s) {
    size_t length = strlen(s);
    this->ensure_size(length);
    strncpy(m_buf, s, length + 1);
    m_length = length;
  }

  basic_string &operator=(const char *s) {
    size_t length = strlen(s);
    this->ensure_size(length);
    strncpy(m_buf, s, length + 1);
    m_length = length;
    return *this;
  }

  bool operator==(const basic_string &s) { return strcmp(m_buf, s.m_buf) == 0; }

  bool operator==(const char *s) { return strcmp(m_buf, s) == 0; }

  size_t length() const noexcept { return m_length; }

  // Returns the character capacity
  size_t capacity() const noexcept { return m_capacity; }

  char *c_str() const noexcept { return m_buf; }

  bool is_small() const noexcept { return m_buf == m_smallbuf; }

  char *begin() noexcept { return &m_buf[0]; }

  char *end() noexcept { return &m_buf[m_length]; }

  void reserve(size_t length) { this->ensure_size(length); }

private:
  // length does not include null terminator
  void ensure_size(size_t length) {
    if (length >= S + 1) {
      // Heap
      if (m_buf != m_smallbuf) {
        // Already on heap
        if (length > m_capacity) {
          // Re allocate
          char *new_buf = new char[length + 1];
          strncpy(new_buf, m_buf, (length < m_length ? length : m_length) + 1);
          delete[] m_buf;
          m_buf = new_buf;
          m_capacity = length;
        }
      } else {
        // Move to heap
        m_buf = new char[length + 1];
        strncpy(m_buf, m_smallbuf, m_length + 1);
        m_capacity = length;
      }
    } else {
      // Stack
      if (m_buf != m_smallbuf) {
        // Move to stack
        strncpy(m_smallbuf, m_buf, S);
        delete[] m_buf;
        m_buf = m_smallbuf;
        m_capacity = S;
      } else {
        // Already on stack
      }
    }
  }

  size_t m_length = 0;

  // Maximum number of characters (not including null terminator)
  size_t m_capacity = S;

  char m_smallbuf[S + 1];
  char *m_buf = m_smallbuf;
};

using string = basic_string<0>;
using small_string = basic_string<128>;

} // namespace ftl
