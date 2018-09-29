#include <iostream>
#include <vkr/window.hpp>
#include <vkr/context.hpp>

int main() {
  vkr::Window window("Hello");
  vkr::Context context(window);

  while (1) {
    SDL_Event event = window.pollEvent();
    switch(event.type) {
    case SDL_WINDOWEVENT:
      switch(event.window.type) {
      case SDL_WINDOWEVENT_RESIZED:
        context.updateSize(
            static_cast<uint32_t>(event.window.data1),
            static_cast<uint32_t>(event.window.data2));
        break;
      }
      break;
    case SDL_QUIT:
      std::cout << "Goodbye\n";
      return 0;
      break;
    }

    context.beginRenderPass();
    context.endRenderPass();
    context.present();
  }

  return 0;
}
