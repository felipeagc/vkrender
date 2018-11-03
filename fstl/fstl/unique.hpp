#pragma once

#include <utility>

namespace fstl {

// Use this wrapper to call .destroy() on destruction
template <class T> class unique {
public:
  unique(T &&t) : t(std::move(t)) {}

  template <typename... Args> unique(Args &&... args) : t(args...) {}

  ~unique() { t.destroy(); }

  unique(const unique &other) = delete;
  unique(unique &&other) noexcept = delete;

  unique &operator=(const unique &other) = delete;
  unique &operator=(const unique &&other) noexcept = delete;

  operator bool() { return t; }

  T *operator->() { return &t; }

  T const *operator->() const { return &t; }

  T &operator*() { return t; }

  T const &operator*() const { return t; }

  const T &get() const { return t; }

  T &get() { return t; }

private:
  T t;
};
} // namespace fstl
