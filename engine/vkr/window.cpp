#include "window.hpp"
#include <SDL2/SDL_vulkan.h>

using namespace vkr;

Window::Window(const char *title, uint32_t width, uint32_t height) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

  this->window = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
}

Window::~Window() {
  SDL_DestroyWindow(this->window);
  SDL_Quit();
}

SDL_Event Window::pollEvent() {
  SDL_Event event;
  SDL_PollEvent(&event);
  return event;
}

std::vector<const char *> Window::getVulkanExtensions() const {
  uint32_t sdlExtensionCount = 0;
  SDL_Vulkan_GetInstanceExtensions(this->window, &sdlExtensionCount, nullptr);
  std::vector<const char *> sdlExtensions(sdlExtensionCount);
  SDL_Vulkan_GetInstanceExtensions(
      this->window, &sdlExtensionCount, sdlExtensions.data());
  return sdlExtensions;
}

vk::SurfaceKHR Window::createVulkanSurface(vk::Instance instance) const {
  vk::SurfaceKHR surface;
  if (!SDL_Vulkan_CreateSurface(
          window,
          static_cast<VkInstance>(instance),
          reinterpret_cast<VkSurfaceKHR *>(&surface))) {
    throw std::runtime_error("Failed to create window surface");
  }
  return surface;
}
