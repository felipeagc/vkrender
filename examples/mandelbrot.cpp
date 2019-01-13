#include <engine/engine.hpp>
#include <ftl/logging.hpp>
#include <ftl/vector.hpp>
#include <renderer/renderer.hpp>

int main() {
  re_window_t window;
  re_window_init(&window, "Mandelbrot set", 1600, 900);

  window.clear_color = {0.15, 0.15, 0.15, 1.0};

  uint32_t width, height;
  re_window_get_size(&window, &width, &height);

  re_canvas_t canvas;
  re_canvas_init(&canvas, width, height);

  // @note: Using this scope to destroy stuff in order for now
  {
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

    while (!window.should_close) {
      block.time += window.delta_time;

      SDL_Event event;
      while (re_window_poll_event(&window, &event)) {
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
          window.should_close = true;
          break;
        }
      }

      auto lock = shaderWatcher.lockPipeline();

      glm::dvec2 offset{0.0};
      double speed = 0.3f * window.delta_time;

      if (re_window_is_scancode_pressed(&window, SDL_SCANCODE_D)) {
        offset.x += speed;
      }
      if (re_window_is_scancode_pressed(&window, SDL_SCANCODE_A)) {
        offset.x -= speed;
      }
      if (re_window_is_scancode_pressed(&window, SDL_SCANCODE_W)) {
        offset.y -= speed;
      }
      if (re_window_is_scancode_pressed(&window, SDL_SCANCODE_S)) {
        offset.y += speed;
      }

      block.camPos += offset;

      re_window_begin_frame(&window);

      auto command_buffer = re_window_get_current_command_buffer(&window);

      // Render target pass
      {
        re_canvas_begin(&canvas, command_buffer);
        re_canvas_end(&canvas, command_buffer);
      }

      // Window pass
      {
        re_window_begin_render_pass(&window);

        vkCmdPushConstants(
            command_buffer,
            shaderWatcher.pipeline.layout,
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(block),
            &block);

        re_canvas_draw(&canvas, command_buffer, &shaderWatcher.pipeline);

        re_window_end_render_pass(&window);
      }

      re_window_end_frame(&window);
    }
  }

  re_canvas_destroy(&canvas);

  re_window_destroy(&window);

  re_context_destroy(&g_ctx);

  return 0;
}
