#pragma once

#include <cstdio>
#include <cstring>

namespace ftl {

inline void vprintf(const char *prefix, const char *format, va_list args) {
  static char buf[512];
  strcpy(buf, "");
  strcat(buf, prefix);
  strcat(buf, format);
  strcat(buf, "\n");
  ::vprintf(buf, args);
}

inline void info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ftl::vprintf("[INFO] ", format, args);
  va_end(args);
}

inline void fatal(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ftl::vprintf("[INFO] ", format, args);
  va_end(args);
}

inline void error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ftl::vprintf("[INFO] ", format, args);
  va_end(args);
}

inline void warn(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ftl::vprintf("[INFO] ", format, args);
  va_end(args);
}

inline void debug(const char *format, ...) {
#ifndef NDEBUG
  va_list args;
  va_start(args, format);
  ftl::vprintf("[INFO] ", format, args);
  va_end(args);
#endif
}

} // namespace ftl
