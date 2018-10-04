#pragma once

#include <vulkan/vulkan.hpp>

namespace {
  template<typename T>
  using ArrayProxy = vk::ArrayProxy<T>;
}
