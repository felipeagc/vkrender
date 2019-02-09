#pragma once

#define VKM_USE_SSE

#if defined(_MSC_VER)
#define VKM_ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
#define VKM_ALIGN(x) __attribute__ ((aligned(x)))
#endif

