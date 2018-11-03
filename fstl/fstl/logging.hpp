#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>

namespace fstl {
namespace log {

template <typename... Args>
void info(std::string_view str, const Args &... args) {
  fmt::print(std::cout, fmt::format("[INFO] {}\n", str), args...);
}

template <typename... Args>
void fatal(std::string_view str, const Args &... args) {
  fmt::print(std::cerr, fmt::format("[FATAL] {}\n", str), args...);
}

template <typename... Args>
void error(std::string_view str, const Args &... args) {
  fmt::print(std::cerr, fmt::format("[ERROR] {}\n", str), args...);
}

template <typename... Args>
void warn(std::string_view str, const Args &... args) {
  fmt::print(std::cout, fmt::format("[WARNING] {}\n", str), args...);
}

template <typename... Args>
void debug(std::string_view str, const Args &... args) {
#ifndef NDEBUG
  fmt::print(std::cout, fmt::format("[DEBUG] {}\n", str), args...);
#endif
}

} // namespace log
} // namespace fstl
