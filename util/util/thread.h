#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#define UT_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__)
#define UT_THREAD_LOCAL __thread
#endif

typedef void *(*ut_routine_t)(void *);

#if defined(_WIN32)
// @todo: implement windows version
#else

#include <pthread.h>

// Pthreads version
typedef pthread_t ut_thread_t;

typedef pthread_mutex_t ut_mutex_t;

typedef pthread_cond_t ut_cond_t;
#endif

void ut_thread_init(ut_thread_t *thread, ut_routine_t routine, void *arg);
void ut_thread_join(ut_thread_t thread, void **retval);

void ut_mutex_init(ut_mutex_t *mutex);
void ut_mutex_destroy(ut_mutex_t *mutex);
void ut_mutex_lock(ut_mutex_t *mutex);
void ut_mutex_unlock(ut_mutex_t *mutex);

void ut_cond_init(ut_cond_t *cond);
void ut_cond_destroy(ut_cond_t *cond);
void ut_cond_signal(ut_cond_t *cond);
void ut_cond_broadcast(ut_cond_t *cond);
void ut_cond_wait(ut_cond_t *cond, ut_mutex_t *mutex);

#ifdef __cplusplus
}
#endif
