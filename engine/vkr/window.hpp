#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vulkan/vulkan.hpp>

namespace vkr {
class Window {
public:
  Window(const char *title, uint32_t width = 800, uint32_t height = 600);
  ~Window();
  Window(const Window &other) = delete;
  Window &operator=(Window other) = delete;

  SDL_Event pollEvent();

  uint32_t getWidth() const;
  uint32_t getHeight() const;

  std::vector<const char *> getVulkanExtensions() const;
  vk::SurfaceKHR createVulkanSurface(vk::Instance instance) const;

private:
  SDL_Window *window;
};
} // namespace vkr
