#ifndef UT_LOG_H
#define UT_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

inline void ut_vprintf(const char *prefix, const char *format, va_list args) {
  char buf[512];
  strcpy(buf, "");
  strcat(buf, prefix);
  strcat(buf, format);
  strcat(buf, "\n");
  vprintf(buf, args);
}

inline void ut_log_info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ut_vprintf("[INFO] ", format, args);
  va_end(args);
}

inline void ut_log_fatal(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ut_vprintf("[FATAL] ", format, args);
  va_end(args);
}

inline void ut_log_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ut_vprintf("[ERROR] ", format, args);
  va_end(args);
}

inline void ut_log_warn(const char *format, ...) {
  va_list args;
  va_start(args, format);
  ut_vprintf("[WARN] ", format, args);
  va_end(args);
}

inline void ut_log_debug(const char *format, ...) {
#ifndef NDEBUG
  va_list args;
  va_start(args, format);
  ut_vprintf("[DEBUG] ", format, args);
  va_end(args);
#endif
}

#ifdef __cplusplus
}
#endif

#endif
