#pragma once

#include <tinycthread.h>
#include <stdbool.h>
#include <stdint.h>

extern _Thread_local uint32_t re_worker_id;

typedef struct re_task_t {
  struct re_task_t *next;
  thrd_start_t routine;
  void *args;
} re_task_t;

typedef struct {
  uint32_t id;
  struct re_task_scheduler *scheduler;
  bool working;
  mtx_t mutex;
  thrd_t thread;
} re_worker_t;

typedef struct re_task_scheduler {
  uint32_t num_workers;
  re_worker_t *workers;
  mtx_t mutex;
  cnd_t wait_cond;
  cnd_t done_cond;
  re_task_t *task;
  bool stop;
} re_task_scheduler_t;

void re_scheduler_init(re_task_scheduler_t *scheduler, uint32_t num_workers);

void re_scheduler_add_task(
    re_task_scheduler_t *scheduler, thrd_start_t routine, void *args);

void re_scheduler_destroy(re_task_scheduler_t *scheduler);
