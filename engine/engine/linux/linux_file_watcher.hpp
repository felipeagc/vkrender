#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <functional>

namespace engine {
class FileWatcher {
public:
  FileWatcher();
  ~FileWatcher();

  void addFile(const std::string &filename);
  void removeFile(const std::string &filename);

  void startWatching();
  void stopWatching();

  std::function<void(const std::string &)> onModify = nullptr;

private:
  struct WatchDescriptor {
    std::string filename;
    std::chrono::steady_clock::time_point lastUpdated;
  };

  int m_fd;
  std::unordered_map<int, WatchDescriptor> m_table;
  std::mutex m_tableMutex;
  std::thread m_watcherThread;
  std::atomic_bool m_running{false};
};
} // namespace engine
