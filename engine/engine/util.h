#pragma once

#include <stdio.h>

#define _EG_LOG_INTERNAL(prefix, ...)                                          \
  do {                                                                         \
    printf(prefix __VA_ARGS__);                                                \
    puts("");                                                                  \
  } while (0);

#ifndef NDEBUG
#define EG_LOG_DEBUG(...) _EG_LOG_INTERNAL("[Engine-debug] ", __VA_ARGS__);
#else
#define EG_LOG_DEBUG(...)
#endif

#define EG_LOG_INFO(...) _EG_LOG_INTERNAL("[Engine-info] ", __VA_ARGS__);
#define EG_LOG_WARN(...) _EG_LOG_INTERNAL("[Engine-warn] ", __VA_ARGS__);
#define EG_LOG_ERROR(...) _EG_LOG_INTERNAL("[Engine-error] ", __VA_ARGS__);
#define EG_LOG_FATAL(...) _EG_LOG_INTERNAL("[Engine-fatal] ", __VA_ARGS__);
