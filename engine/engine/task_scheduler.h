#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <tinycthread.h>

extern _Thread_local uint32_t eg_worker_id;

typedef struct eg_task_t {
  struct eg_task_t *next;
  thrd_start_t routine;
  void *args;
} eg_task_t;

typedef struct eg_worker_t {
  uint32_t id;
  struct eg_task_scheduler_t *scheduler;
  bool working;
  mtx_t mutex;
  thrd_t thread;
} eg_worker_t;

typedef struct eg_task_scheduler_t {
  uint32_t num_workers;
  eg_worker_t *workers;
  mtx_t mutex;
  cnd_t wait_cond;
  cnd_t done_cond;
  eg_task_t *task;
  bool stop;
} eg_task_scheduler_t;

void eg_scheduler_init(eg_task_scheduler_t *scheduler, uint32_t num_workers);

void eg_scheduler_add_task(
    eg_task_scheduler_t *scheduler, thrd_start_t routine, void *args);

void eg_scheduler_destroy(eg_task_scheduler_t *scheduler);
