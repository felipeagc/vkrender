#pragma once

#include "string.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace ftl {

// @todo: allow for larger, dynamic buffers

template <size_t S = 16384> struct string_builder {
  string_builder() {}

  void append(const char *s) {
    for (size_t i = 0; i < strlen(s); i++) {
      m_buf[m_length++] = s[i];
    }
    m_buf[m_length] = '\0';
  }

  void append(const string &s) {
    strcpy(m_buf, s.c_str());
    m_buf[m_length] = '\0';
  }

  // length does not include null terminator
  void copy_to(char *dest, size_t length) const noexcept {
    strncpy(dest, m_buf, length + 1);
  }

  void copy_to(ftl::string &s) const noexcept {
    s.reserve(m_length);
    s = m_buf;
  }

  ftl::string build_string() const noexcept {
    ftl::string s;
    this->copy_to(s);
    return s;
  }

  void clear() {
    m_buf[0] = '\0';
    m_length = 0;
  }

  bool is_empty() const noexcept { return m_length == 0; }

  size_t length() const noexcept { return m_length; }

  char operator[](size_t pos) const noexcept { return m_buf[pos]; }

private:
  size_t m_length = 0;
  char m_buf[S] = "";
};

} // namespace ftl
