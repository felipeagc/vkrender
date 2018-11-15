#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <vkr/buffer.hpp>
#include <vkr/camera.hpp>
#include <vkr/context.hpp>
#include <vkr/gltf_model.hpp>
#include <vkr/graphics_pipeline.hpp>
#include <vkr/shader.hpp>
#include <vkr/texture.hpp>
#include <vkr/util.hpp>
#include <vkr/window.hpp>

class Lighting {
public:
  Lighting(glm::vec3 lightColor, glm::vec3 lightPos) {
    this->ubo.lightColor = glm::vec4(lightColor, 1.0f);
    this->ubo.lightPos = glm::vec4(lightPos, 1.0f);

    for (size_t i = 0; i < ARRAYSIZE(this->uniformBuffers.buffers); i++) {
      vkr::buffer::makeUniformBuffer(
          sizeof(LightingUniform),
          &this->uniformBuffers.buffers[i],
          &this->uniformBuffers.allocations[i]);
    }

    auto [descriptorPool, descriptorSetLayout] =
        vkr::ctx::descriptorManager[vkr::DESC_LIGHTING];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    for (int i = 0; i < vkr::MAX_FRAMES_IN_FLIGHT; i++) {
      vkAllocateDescriptorSets(
          vkr::ctx::device, &allocateInfo, &this->descriptorSets[i]);

      vkr::buffer::mapMemory(this->uniformBuffers.allocations[i], &this->mappings[i]);
      memcpy(this->mappings[i], &this->ubo, sizeof(LightingUniform));

      VkDescriptorBufferInfo bufferInfo = {
          this->uniformBuffers.buffers[i], 0, sizeof(LightingUniform)};

      VkWriteDescriptorSet descriptorWrite = {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          nullptr,
          this->descriptorSets[i],               // dstSet
          0,                                 // dstBinding
          0,                                 // dstArrayElement
          1,                                 // descriptorCount
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
          nullptr,                           // pImageInfo
          &bufferInfo,                       // pBufferInfo
          nullptr,                           // pTexelBufferView
      };

      vkUpdateDescriptorSets(vkr::ctx::device, 1, &descriptorWrite, 0, nullptr);
    }
  }

  void destroy() {
    for (size_t i = 0; i < ARRAYSIZE(this->uniformBuffers.buffers); i++) {
      vkr::buffer::unmapMemory(this->uniformBuffers.allocations[i]);
      vkr::buffer::destroy(
          this->uniformBuffers.buffers[i], this->uniformBuffers.allocations[i]);
    }

    auto descriptorPool =
        vkr::ctx::descriptorManager.getPool(vkr::DESC_LIGHTING);

    assert(descriptorPool != nullptr);

    vkFreeDescriptorSets(
        vkr::ctx::device,
        *descriptorPool,
        ARRAYSIZE(this->descriptorSets),
        this->descriptorSets);
  }

  void bind(vkr::Window &window, vkr::GraphicsPipeline &pipeline) {
    VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.getLayout(),
        3, // firstSet
        1,
        &this->descriptorSets[window.getCurrentFrameIndex()],
        0,
        nullptr);
  }

private:
  struct LightingUniform {
    glm::vec4 lightColor;
    glm::vec4 lightPos;
  } ubo;

  vkr::buffer::Buffers<vkr::MAX_FRAMES_IN_FLIGHT> uniformBuffers;
  void *mappings[vkr::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet descriptorSets[vkr::MAX_FRAMES_IN_FLIGHT];
};

int main() {
  vkr::Window window("GLTF models");

  window.setMSAASamples(VK_SAMPLE_COUNT_4_BIT);

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

  vkr::Camera camera({0.0, 0.0, 0.0});

  Lighting lighting({0.95, 0.80, 0.52}, {3.0, 3.0, 3.0});

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

    // float radius = 3.5f;
    // float camX = sin(time) * radius;
    // float camY = cos(time) * radius;
    // camera.setPos(glm::vec3{camX, radius / 1.5, camY});
    camera.lookAt((helmet.getPosition() + boombox.getPosition()) / 2.0f);

    camera.update(window);

    helmet.setRotation({0.0, time * 100.0, 0.0});
    boombox.setRotation({0.0, time * 100.0, 0.0});
    lighting.bind(window, modelPipeline);

    camera.bind(window, modelPipeline);
    helmet.draw(window, modelPipeline);
    boombox.draw(window, modelPipeline);

    ImGui::Begin("Camera");

    static float camPos[3] = {
        camera.getPos().x,
        camera.getPos().y,
        camera.getPos().z,
    };
    ImGui::SliderFloat3("Camera position", camPos, -10.0f, 10.0f);
    camera.setPos({camPos[0], camPos[1], camPos[2]});

    ImGui::End();
  };

  while (!window.getShouldClose()) {
    window.present(draw);
  }

  lighting.destroy();
  camera.destroy();
  helmet.destroy();
  boombox.destroy();
  modelPipeline.destroy();
  modelShader.destroy();
  window.destroy();
  vkr::ctx::destroy();

  return 0;
}
