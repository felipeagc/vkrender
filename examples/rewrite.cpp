#include <renderer/renderer.hpp>
#include <imgui/imgui.h>

int main() {
  re_window_t window;
  re_window_init(&window, "Re-write", 1600, 900);

  re_imgui_t imgui;
  re_imgui_init(&imgui, &window);

  window.clear_color = {1.0, 1.0, 1.0, 1.0};

  while (!window.should_close) {
    SDL_Event event;
    while (re_window_poll_event(&window, &event)) {
      re_imgui_process_event(&imgui, &event);

      switch (event.type) {
      case SDL_QUIT:
        window.should_close = true;
        break;
      }
    }

    re_imgui_begin(&imgui);

    if (ImGui::Begin("Hello!")) {
      ImGui::End();
    }

    re_imgui_end(&imgui);

    re_window_begin_frame(&window);

    re_window_begin_render_pass(&window);

    re_imgui_draw(&imgui);

    re_window_end_render_pass(&window);

    re_window_end_frame(&window);
  }

  re_imgui_destroy(&imgui);
  re_window_destroy(&window);
  re_context_destroy(&g_ctx);

  return 0;
}
