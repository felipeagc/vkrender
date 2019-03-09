#ifndef UT_LOG_H
#define UT_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

inline void ut_log_info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  printf("[INFO] ");
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

inline void ut_log_fatal(const char *format, ...) {
  va_list args;
  va_start(args, format);
  printf("[FATAL] ");
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

inline void ut_log_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  printf("[ERROR] ");
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

inline void ut_log_warn(const char *format, ...) {
  va_list args;
  va_start(args, format);
  printf("[WARN] ");
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

inline void ut_log_debug(const char *format, ...) {
#ifndef NDEBUG
  va_list args;
  va_start(args, format);
  printf("[DEBUG] ");
  vprintf(format, args);
  printf("\n");
  va_end(args);
#endif
}

#ifdef __cplusplus
}
#endif

#endif
