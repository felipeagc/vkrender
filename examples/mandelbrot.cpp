#include <engine/engine.hpp>
#include <ftl/logging.hpp>
#include <ftl/vector.hpp>
#include <renderer/renderer.hpp>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 800, 600);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  re_canvas_t canvas;
  re_canvas_init(&canvas, window.getWidth(), window.getHeight());

  // Create shaders & pipelines
  engine::ShaderWatcher shaderWatcher(
      &window.render_target,
      "../shaders/mandelbrot.vert",
      "../shaders/mandelbrot.frag",
      eg_fullscreen_pipeline_parameters());

  shaderWatcher.startWatching();

  struct {
    double time = 0.0;
    glm::dvec2 camPos{0.0, 0.0};
  } block;

  while (!window.getShouldClose()) {
    block.time += window.getDelta();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      switch (event.type) {
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          re_canvas_resize(
              &canvas,
              (uint32_t)event.window.data1,
              (uint32_t)event.window.data2);
        }
        break;
      case SDL_QUIT:
        window.setShouldClose(true);
        break;
      }
    }

    shaderWatcher.lockPipeline();

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

    // Render target pass
    {
      re_canvas_begin(&canvas, window.getCurrentCommandBuffer());
      re_canvas_end(&canvas, window.getCurrentCommandBuffer());
    }

    // Window pass
    {
      window.beginRenderPass();

      vkCmdPushConstants(
          window.getCurrentCommandBuffer(),
          shaderWatcher.pipeline.layout,
          VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
          0,
          sizeof(block),
          &block);

      re_canvas_draw(
          &canvas, window.getCurrentCommandBuffer(), &shaderWatcher.pipeline);

      window.endRenderPass();
    }

    window.endFrame();
  }

  re_canvas_destroy(&canvas);

  return 0;
}
