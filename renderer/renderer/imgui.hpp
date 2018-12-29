#pragma once

#include "common.hpp"

union SDL_Event;

namespace renderer {
class Window;

class ImGuiRenderer {
public:
  ImGuiRenderer(Window &window);
  ~ImGuiRenderer();

  // ImguiRenderer cannot be copied
  ImGuiRenderer(const ImGuiRenderer &) = delete;
  ImGuiRenderer &operator=(const ImGuiRenderer &) = delete;

  // ImguiRenderer cannot be moved
  ImGuiRenderer(ImGuiRenderer &&) = delete;
  ImGuiRenderer &operator=(ImGuiRenderer &&) = delete;

  void begin();
  void end();

  void draw();

  void processEvent(SDL_Event *event);

private:
  Window &m_window;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  void createDescriptorPool();
  void destroyDescriptorPool();
};
} // namespace renderer
