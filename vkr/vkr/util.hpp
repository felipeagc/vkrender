#pragma once

#include <cassert>
#include <vulkan/vulkan.h>

#define VK_CHECK(exp)                                                          \
  do {                                                                         \
    VkResult result = exp;                                                     \
    assert(result == VK_SUCCESS);                                              \
  } while (0)

#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))
