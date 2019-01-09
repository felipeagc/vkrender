#include <engine/engine.hpp>
#include <ftl/vector.hpp>
#include <ftl/logging.hpp>
#include <renderer/renderer.hpp>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 800, 600);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  renderer::Canvas renderTarget(window.getWidth(), window.getHeight());

  // Create shaders & pipelines
  engine::ShaderWatcher<renderer::FullscreenPipeline> shaderWatcher(
      window, "../shaders/mandelbrot.vert", "../shaders/mandelbrot.frag");

  shaderWatcher.startWatching();

  struct {
    double time = 0.0;
    glm::dvec2 camPos{0.0, 0.0};
  } block;

  auto pipelineLayout =
      renderer::ctx().m_resourceManager.m_providers.fullscreen.pipelineLayout;

  while (!window.getShouldClose()) {
    block.time += window.getDelta();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      switch (event.type) {
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          renderTarget.resize(
              static_cast<uint32_t>(event.window.data1),
              static_cast<uint32_t>(event.window.data2));
        }
        break;
      case SDL_QUIT:
        window.setShouldClose(true);
        break;
      }
    }

    glm::dvec2 offset{0.0};
    double speed = 0.3f * window.getDelta();

    if (window.isScancodePressed(renderer::Scancode::eD)) {
      offset.x += speed;
    }
    if (window.isScancodePressed(renderer::Scancode::eA)) {
      offset.x -= speed;
    }
    if (window.isScancodePressed(renderer::Scancode::eW)) {
      offset.y -= speed;
    }
    if (window.isScancodePressed(renderer::Scancode::eS)) {
      offset.y += speed;
    }

    block.camPos += offset;

    window.beginFrame();

    shaderWatcher.lockPipeline();

    // Render target pass
    {
      renderTarget.beginRenderPass(window);
      renderTarget.endRenderPass(window);
    }

    // Window pass
    {
      window.beginRenderPass();

      vkCmdPushConstants(
          window.getCurrentCommandBuffer(),
          pipelineLayout,
          VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
          0,
          sizeof(block),
          &block);

      renderTarget.draw(window, shaderWatcher.pipeline());

      window.endRenderPass();
    }

    window.endFrame();
  }

  return 0;
}
