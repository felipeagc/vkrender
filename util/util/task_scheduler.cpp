#include "task_scheduler.hpp"
#include <stdlib.h>

thread_local uint32_t ut_worker_id = 0;

static inline void task_init(ut_task_t *task, ut_routine_t routine, void* args) {
  task->routine = routine;
  task->args = args;
  task->next = NULL;
}

static inline void task_append(ut_task_t *task, ut_routine_t routine, void* args) {
  if (task->next != NULL) {
    return task_append(task->next, routine, args);
  } else {
    task->next = (ut_task_t *)malloc(sizeof(ut_task_t));
    task_init(task->next, routine, args);
  }
}

void *worker_routine(void *args) {
  ut_worker_t *worker = (ut_worker_t *)args;

  ut_worker_id = worker->id;

  ut_task_t *curr_task = NULL;

  while (1) {
    while (!worker->scheduler->stop && worker->scheduler->task == NULL) {
      pthread_cond_wait(
          &worker->scheduler->wait_cond, &worker->scheduler->mutex);
    }

    if (worker->scheduler->stop) {
      pthread_mutex_unlock(&worker->scheduler->mutex);
      return NULL;
    }

    curr_task = worker->scheduler->task;
    if (worker->scheduler->task != NULL) {
      worker->scheduler->task = worker->scheduler->task->next;
    }
    pthread_mutex_unlock(&worker->scheduler->mutex);

    if (curr_task != NULL) {
      curr_task->routine(curr_task->args);

      free(curr_task);
    }

    pthread_cond_signal(&worker->scheduler->done_cond);
  }

  return NULL;
}

static inline void
worker_init(ut_worker_t *worker, ut_task_scheduler_t *scheduler, uint32_t id) {
  worker->id = id;
  worker->scheduler = scheduler;
  worker->working = false;
  pthread_create(&worker->thread, NULL, worker_routine, worker);
  pthread_mutex_init(&worker->mutex, NULL);
}

static inline void worker_wait(ut_worker_t *worker) {
  pthread_join(worker->thread, NULL);
}

static inline void worker_destroy(ut_worker_t *worker) {
  pthread_mutex_destroy(&worker->mutex);
}

void ut_scheduler_init(ut_task_scheduler_t *scheduler, uint32_t num_workers) {
  scheduler->num_workers = num_workers;
  scheduler->workers = (ut_worker_t *)malloc(sizeof(ut_worker_t) * scheduler->num_workers);
  scheduler->task = NULL;
  scheduler->stop = false;
  pthread_cond_init(&scheduler->wait_cond, NULL);
  pthread_cond_init(&scheduler->done_cond, NULL);
  pthread_mutex_init(&scheduler->mutex, NULL);
  for (uint32_t i = 0; i < scheduler->num_workers; i++) {
    worker_init(&scheduler->workers[i], scheduler, i);
  }
}

void ut_scheduler_add_task(ut_task_scheduler_t *scheduler, ut_routine_t routine, void* args) {
  pthread_mutex_lock(&scheduler->mutex);
  if (scheduler->task == NULL) {
    scheduler->task = (ut_task_t *)malloc(sizeof(ut_task_t));
    task_init(scheduler->task, routine, args);
  } else {
    task_append(scheduler->task, routine, args);
  }
  pthread_mutex_unlock(&scheduler->mutex);

  pthread_cond_signal(&scheduler->wait_cond);
}

void ut_scheduler_destroy(ut_task_scheduler_t *scheduler) {
  while (1) {
    pthread_cond_wait(&scheduler->done_cond, &scheduler->mutex);
    if (scheduler->task == NULL) {
      scheduler->stop = true;
      pthread_mutex_unlock(&scheduler->mutex);
      break;
    }
    pthread_mutex_unlock(&scheduler->mutex);
  }

  pthread_cond_broadcast(&scheduler->wait_cond);

  for (uint32_t i = 0; i < scheduler->num_workers; i++) {
    worker_wait(&scheduler->workers[i]);
  }

  for (uint32_t i = 0; i < scheduler->num_workers; i++) {
    worker_destroy(&scheduler->workers[i]);
  }

  pthread_cond_destroy(&scheduler->wait_cond);
  pthread_cond_destroy(&scheduler->done_cond);
  pthread_mutex_destroy(&scheduler->mutex);

  free(scheduler->workers);
}
