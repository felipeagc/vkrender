#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "thread.h"
#include <stdbool.h>
#include <stdint.h>

extern UT_THREAD_LOCAL uint32_t ut_worker_id;

typedef struct ut_task {
  struct ut_task *next;
  ut_routine_t routine;
  void *args;
} ut_task_t;

typedef struct {
  uint32_t id;
  struct ut_task_scheduler *scheduler;
  bool working;
  ut_mutex_t mutex;
  ut_thread_t thread;
} ut_worker_t;

typedef struct ut_task_scheduler {
  uint32_t num_workers;
  ut_worker_t *workers;
  ut_mutex_t mutex;
  ut_cond_t wait_cond;
  ut_cond_t done_cond;
  ut_task_t *task;
  bool stop;
} ut_task_scheduler_t;

void ut_scheduler_init(ut_task_scheduler_t *scheduler, uint32_t num_workers);

void ut_scheduler_add_task(
    ut_task_scheduler_t *scheduler, ut_routine_t routine, void *args);

void ut_scheduler_destroy(ut_task_scheduler_t *scheduler);

#ifdef __cplusplus
}
#endif
