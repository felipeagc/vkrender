#include "linux_file_watcher.hpp"
#include <ftl/logging.hpp>
#include <sys/inotify.h>
#include <unistd.h>

using namespace engine;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

FileWatcher::FileWatcher() { m_fd = inotify_init1(IN_NONBLOCK); }

FileWatcher::~FileWatcher() {
  std::scoped_lock<std::mutex> lock(m_tableMutex);

  this->stopWatching();

  for (auto &[wd, desc] : m_table) {
    inotify_rm_watch(m_fd, wd);
  }

  close(m_fd);
}

void FileWatcher::addFile(const char *filename) {
  std::scoped_lock<std::mutex> lock(m_tableMutex);

  int wd = inotify_add_watch(m_fd, filename, IN_ALL_EVENTS);
  m_table[wd] = WatchDescriptor{filename, std::chrono::steady_clock::now()};
}

void FileWatcher::removeFile(const char *filename) {
  std::scoped_lock<std::mutex> lock(m_tableMutex);

  for (auto &[wd, desc] : m_table) {
    if (filename == desc.filename) {
      inotify_rm_watch(m_fd, wd);
      m_table.erase(wd);
      return;
    }
  }
}

void FileWatcher::startWatching() {
  m_running.store(true);
  m_watcherThread = std::thread([&]() {
    while (m_running.load()) {
      char buf[EVENT_BUF_LEN]
          __attribute__((aligned(__alignof__(struct inotify_event))));

      int len = read(m_fd, buf, EVENT_BUF_LEN);

      if (len < 0) {
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(250ms);
        continue;
      }

      int i = 0;

      while (i < len) {
        struct inotify_event *event = (struct inotify_event *)&buf[i];

        if (event->mask & IN_IGNORED) {
          std::scoped_lock<std::mutex> lock(m_tableMutex);

          std::string filename = m_table[event->wd].filename;
          m_table.erase(event->wd);

          inotify_rm_watch(m_fd, event->wd);
          int wd = inotify_add_watch(m_fd, filename.c_str(), IN_ALL_EVENTS);

          m_table[wd] =
              WatchDescriptor{filename, std::chrono::steady_clock::now()};
        }

        if (event->mask & IN_MOVE_SELF || event->mask & IN_MODIFY ||
            event->mask & IN_CREATE) {
          if (this->onModify != nullptr) {
            std::scoped_lock<std::mutex> lock(m_tableMutex);
            auto now = std::chrono::steady_clock::now();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now - m_table[event->wd].lastUpdated)
                              .count();
            if (millis > 250) {
              this->onModify(m_table[event->wd].filename);
              m_table[event->wd].lastUpdated = now;
            }
          }
        }

        i += EVENT_SIZE + event->len;
      }
    }
  });
}

void FileWatcher::stopWatching() {
  m_running.store(false);
  if (m_watcherThread.joinable()) {
    m_watcherThread.join();
  }
}
