#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
// @todo: implement windows version
#else

void ut_thread_init(ut_thread_t *thread, ut_routine_t routine, void *arg) {
  pthread_create(thread, NULL, routine, arg);
}

void ut_thread_join(ut_thread_t thread, void **retval) {
  pthread_join(thread, retval);
}

void ut_mutex_init(ut_mutex_t *mutex) { pthread_mutex_init(mutex, NULL); }

void ut_mutex_destroy(ut_mutex_t *mutex) { pthread_mutex_destroy(mutex); }

void ut_mutex_lock(ut_mutex_t *mutex) { pthread_mutex_lock(mutex); }

void ut_mutex_unlock(ut_mutex_t *mutex) { pthread_mutex_unlock(mutex); }

void ut_cond_init(ut_cond_t *cond) { pthread_cond_init(cond, NULL); }

void ut_cond_destroy(ut_cond_t *cond) { pthread_cond_destroy(cond); }

void ut_cond_signal(ut_cond_t *cond) { pthread_cond_signal(cond); }

void ut_cond_broadcast(ut_cond_t *cond) { pthread_cond_broadcast(cond); }

void ut_cond_wait(ut_cond_t *cond, ut_mutex_t *mutex) {
  pthread_cond_wait(cond, mutex);
}

#endif

#ifdef __cplusplus
}
#endif
