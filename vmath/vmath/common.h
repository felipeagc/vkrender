#pragma once

#define VMATH_USE_SSE

#if defined(_MSC_VER)
#define VMATH_ALIGN(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define VMATH_ALIGN(x) __attribute__ ((aligned(x)))
#endif
#endif

