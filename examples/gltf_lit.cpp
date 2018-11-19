#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <vkr/buffer.hpp>
#include <vkr/camera.hpp>
#include <vkr/context.hpp>
#include <vkr/gltf_model.hpp>
#include <vkr/graphics_pipeline.hpp>
#include <vkr/imgui_utils.hpp>
#include <vkr/lighting.hpp>
#include <vkr/shader.hpp>
#include <vkr/texture.hpp>
#include <vkr/util.hpp>
#include <vkr/window.hpp>

int main() {
  vkr::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_4_BIT);

  window.clearColor = {0.52, 0.80, 0.92, 1.0};

  vkr::Shader modelShader{
      "../shaders/model_lit.vert",
      "../shaders/model_lit.frag",
  };

  vkr::GraphicsPipeline modelPipeline{
      window,
      modelShader,
      vkr::GltfModel::getVertexFormat(),
      vkr::ctx::descriptorManager.getDefaultSetLayouts(),
  };

  vkr::Camera camera({3.0, 3.0, 3.0});

  vkr::LightManager lightManager({
      vkr::Light{glm::vec4(3.0, 3.0, 3.0, 1.0), glm::vec4(1.0, 0.0, 0.0, 1.0)},
      vkr::Light{glm::vec4(-3.0, -3.0, -3.0, 1.0),
                 glm::vec4(0.0, 1.0, 0.0, 1.0)},
  });

  vkr::GltfModel helmet{window, "../assets/DamagedHelmet.glb", true};
  helmet.setPosition({2.0, 0.0, 0.0});
  vkr::GltfModel boombox{window, "../assets/BoomBox.glb"};
  boombox.setPosition({-2.0, 0.0, 0.0});
  boombox.setScale(glm::vec3{1.0, 1.0, 1.0} * 100.0f);

  float time = 0.0;

  auto draw = [&]() {
    time += window.getDelta();

    SDL_Event event = window.pollEvent();
    switch (event.type) {
    case SDL_WINDOWEVENT:
      switch (event.window.type) {
      case SDL_WINDOWEVENT_RESIZED:
        window.updateSize();
        break;
      }
      break;
    case SDL_QUIT:
      fstl::log::info("Goodbye");
      window.setShouldClose(true);
      break;
    }

    ImGui::Begin("Camera");

    static float camPos[] = {3.0, 3.0, 3.0};

    ImGui::SliderFloat3("Camera position", camPos, -10.0f, 10.0f);
    camera.setPos({camPos[0], camPos[1], camPos[2]});

    ImGui::End();

    camera.lookAt((helmet.getPosition() + boombox.getPosition()) / 2.0f);
    camera.update(window);

    ImGui::Begin("Lights");

    for (uint32_t i = 0; i < lightManager.getLightCount(); i++) {
      ImGui::PushID(i);
      ImGui::Text("Light %d", i);

      vkr::Light *light = &lightManager.getLights()[i];

      float pos[3] = {
          light->pos.x,
          light->pos.y,
          light->pos.z,
      };
      ImGui::DragFloat3("Position", pos, 1.0f, -10.0f, 10.0f);
      light->pos = {pos[0], pos[1], pos[2], 1.0f};

      float color[4] = {
          light->color.x,
          light->color.y,
          light->color.z,
          light->color.w,
      };
      ImGui::ColorEdit4("Color", color);
      light->color = {color[0], color[1], color[2], color[3]};

      ImGui::Separator();
      ImGui::PopID();
    }

    ImGui::End();

    vkr::imgui::statsWindow(window);

    // Draw stuff

    helmet.setRotation({0.0, time * 100.0, 0.0});
    boombox.setRotation({0.0, time * 100.0, 0.0});
    lightManager.bind(window, modelPipeline);

    camera.bind(window, modelPipeline);
    helmet.draw(window, modelPipeline);
    boombox.draw(window, modelPipeline);
  };

  while (!window.getShouldClose()) {
    window.present(draw);
    lightManager.update();
  }

  lightManager.destroy();
  camera.destroy();
  helmet.destroy();
  boombox.destroy();
  modelPipeline.destroy();
  modelShader.destroy();
  window.destroy();
  vkr::ctx::destroy();

  return 0;
}
