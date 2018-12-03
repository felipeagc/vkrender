#include "file_watcher.hpp"

#ifdef __linux__
#include "linux/linux_file_watcher.cpp"
#endif
