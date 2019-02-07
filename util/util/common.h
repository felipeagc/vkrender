#pragma once

#if defined(_MSC_VER)
#define UT_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__)
#define UT_THREAD_LOCAL __thread
#endif
