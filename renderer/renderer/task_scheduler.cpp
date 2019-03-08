#include "task_scheduler.hpp"
#include <stdlib.h>

_Thread_local uint32_t re_worker_id = 0;

static inline void
task_init(re_task_t *task, thrd_start_t routine, void *args) {
  task->routine = routine;
  task->args = args;
  task->next = NULL;
}

static inline void
task_append(re_task_t *task, thrd_start_t routine, void *args) {
  if (task->next != NULL) {
    return task_append(task->next, routine, args);
  } else {
    task->next = (re_task_t *)malloc(sizeof(re_task_t));
    task_init(task->next, routine, args);
  }
}

int worker_routine(void *args) {
  re_worker_t *worker = (re_worker_t *)args;

  re_worker_id = worker->id;

  re_task_t *curr_task = NULL;

  while (1) {
    while (!worker->scheduler->stop && worker->scheduler->task == NULL) {
      cnd_wait(&worker->scheduler->wait_cond, &worker->scheduler->mutex);
    }

    if (worker->scheduler->stop) {
      mtx_unlock(&worker->scheduler->mutex);
      return 0;
    }

    curr_task = worker->scheduler->task;
    if (worker->scheduler->task != NULL) {
      worker->scheduler->task = worker->scheduler->task->next;
    }
    mtx_unlock(&worker->scheduler->mutex);

    if (curr_task != NULL) {
      curr_task->routine(curr_task->args);

      free(curr_task);
    }

    cnd_signal(&worker->scheduler->done_cond);
  }

  return 0;
}

static inline void worker_init(
    re_worker_t *worker, re_task_scheduler_t *scheduler, uint32_t id) {
  worker->id = id;
  worker->scheduler = scheduler;
  worker->working = false;
  thrd_create(&worker->thread, worker_routine, worker);
  mtx_init(&worker->mutex, mtx_plain);
}

static inline void worker_wait(re_worker_t *worker) {
  thrd_join(worker->thread, NULL);
}

static inline void worker_destroy(re_worker_t *worker) {
  mtx_destroy(&worker->mutex);
}

void re_scheduler_init(re_task_scheduler_t *scheduler, uint32_t num_workers) {
  scheduler->num_workers = num_workers;
  scheduler->workers =
      (re_worker_t *)malloc(sizeof(re_worker_t) * scheduler->num_workers);
  scheduler->task = NULL;
  scheduler->stop = false;
  cnd_init(&scheduler->wait_cond);
  cnd_init(&scheduler->done_cond);
  mtx_init(&scheduler->mutex, mtx_plain);
  for (uint32_t i = 0; i < scheduler->num_workers; i++) {
    worker_init(&scheduler->workers[i], scheduler, i);
  }
}

void re_scheduler_add_task(
    re_task_scheduler_t *scheduler, thrd_start_t routine, void *args) {
  mtx_lock(&scheduler->mutex);
  if (scheduler->task == NULL) {
    scheduler->task = (re_task_t *)malloc(sizeof(re_task_t));
    task_init(scheduler->task, routine, args);
  } else {
    task_append(scheduler->task, routine, args);
  }
  mtx_unlock(&scheduler->mutex);

  cnd_signal(&scheduler->wait_cond);
}

void re_scheduler_destroy(re_task_scheduler_t *scheduler) {
  while (1) {
    cnd_wait(&scheduler->done_cond, &scheduler->mutex);
    if (scheduler->task == NULL) {
      scheduler->stop = true;
      mtx_unlock(&scheduler->mutex);
      break;
    }
    mtx_unlock(&scheduler->mutex);
  }

  cnd_broadcast(&scheduler->wait_cond);

  for (uint32_t i = 0; i < scheduler->num_workers; i++) {
    worker_wait(&scheduler->workers[i]);
  }

  for (uint32_t i = 0; i < scheduler->num_workers; i++) {
    worker_destroy(&scheduler->workers[i]);
  }

  cnd_destroy(&scheduler->wait_cond);
  cnd_destroy(&scheduler->done_cond);
  mtx_destroy(&scheduler->mutex);

  free(scheduler->workers);
}
