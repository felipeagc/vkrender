#pragma once

#include <vulkan/vulkan.hpp>

namespace vkr {
  template<typename T>
  using ArrayProxy = vk::ArrayProxy<T>;
}
