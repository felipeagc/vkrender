#ifndef FSTD_UTIL_H
#define FSTD_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#if defined(_MSC_VER)
#define ALIGNAS(x) __declspec(align(x))
#elif defined(__clang__)
#define ALIGNAS(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
#define ALIGNAS(x) __attribute__((aligned(x)))
#endif

#ifdef __cplusplus
}
#endif

#endif
