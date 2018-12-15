#pragma once
#include <renderer/buffer.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/cubemap.hpp>
#include <renderer/window.hpp>

namespace engine {
class SkyboxComponent {
public:
  // Creates an initialized Skybox with the given parameters
  SkyboxComponent(const renderer::Cubemap &cubemap);

  ~SkyboxComponent();

  // Skybox can't be copied
  SkyboxComponent(const SkyboxComponent &) = delete;
  SkyboxComponent &operator=(const SkyboxComponent &) = delete;

  // Skybox can be moved
  SkyboxComponent(SkyboxComponent &&rhs) = delete;
  SkyboxComponent &operator=(SkyboxComponent &&rhs) = delete;

  void draw(
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline);

protected:
  renderer::Cubemap m_cubemap;

  VkDescriptorSet m_materialDescriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
