#include "thread_pool.hpp"

namespace renderer {
thread_local uint32_t threadID = 0;

Worker::~Worker() {
  if (m_thread.joinable()) {
    this->wait();
    m_jobsMutex.lock();
    m_destroying = true;
    m_cv.notify_one();
    m_jobsMutex.unlock();
    m_thread.join();
  }
}

void Worker::startThread() { m_thread = std::thread(&Worker::work, this); }

void Worker::work() {
  threadID = m_id;
  while (true) {
    Job job;
    {
      std::unique_lock<std::mutex> lock(m_jobsMutex);
      m_cv.wait(lock, [this]() { return !m_jobs.empty() || m_destroying; });
      if (m_destroying) {
        break;
      }

      job = m_jobs.front();
    }

    job();

    {
      std::unique_lock<std::mutex> lock(m_jobsMutex);
      m_jobs.pop();
      m_cv.notify_one();
    }
  }
}

void Worker::addJob(Job job) {
  std::scoped_lock lock(m_jobsMutex);
  m_jobs.push(job);
  m_cv.notify_one();
}

void Worker::wait() {
  std::unique_lock lock(m_jobsMutex);
  m_cv.wait(lock, [this]() { return m_jobs.empty(); });
}

ThreadPool::ThreadPool(uint32_t workerCount) {
  m_workers = std::vector<Worker>(workerCount);
  for (uint32_t i = 0; i < workerCount; i++) {
    m_workers[i].m_id = i;
    m_workers[i].startThread();
  }
}

void ThreadPool::wait() {
  for (auto &worker : m_workers) {
    worker.wait();
  }
}

void ThreadPool::addJob(Job job) {
  m_workers[m_currentThread].addJob(job);

  m_currentThread++;
  m_currentThread = m_currentThread % m_workers.size();
}

} // namespace renderer
