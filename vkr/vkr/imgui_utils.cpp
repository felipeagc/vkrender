#include "imgui_utils.hpp"
#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <imgui/imgui.h>

namespace vkr {
namespace imgui {
void statsWindow(Window &window) {
  ImGui::Begin("Stats");

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(ctx::physicalDevice, &properties);

  ImGui::Text("Physical device: %s", properties.deviceName);
  ImGui::Text("Delta time: %.3f s", window.getDelta());
  ImGui::Text("Framerate: %.2f", 1.0f / window.getDelta());

  ImGui::End();
}

} // namespace imgui
} // namespace vkr
