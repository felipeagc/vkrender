#include <iostream>
#include <vkr/window.hpp>
#include <vkr/context.hpp>

int main() {
  vkr::Window window("Hello");
  vkr::Context context(window);

  while (1) {
    SDL_Event event = window.pollEvent();
    switch(event.type) {
    case SDL_QUIT:
      std::cout << "Goodbye\n";
      return 0;
      break;
    }
  }

  return 0;
}
