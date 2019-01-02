#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace renderer {

// Thread ID used to index thread local stuff, like CommandPools.
// Starts from 0.
extern thread_local uint32_t threadID;

using Job = std::function<void()>;

class Worker {
  friend class ThreadPool;

public:
  Worker() {}
  ~Worker();

  void startThread();
  void work();
  void addJob(Job job);
  void wait();

protected:
  uint32_t m_id;
  std::thread m_thread;
  std::condition_variable m_cv;
  std::mutex m_jobsMutex;
  std::queue<Job> m_jobs;
  bool m_destroying = false;
};

class ThreadPool {
public:
  ThreadPool(uint32_t workerCount);
  ~ThreadPool() {}

  void wait();
  void addJob(Job job);

protected:
  std::vector<Worker> m_workers;
  uint32_t m_currentThread;
};
} // namespace renderer
