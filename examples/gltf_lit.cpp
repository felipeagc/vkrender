#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <fstl/unique.hpp>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <vkr/buffer.hpp>
#include <vkr/camera.hpp>
#include <vkr/commandbuffer.hpp>
#include <vkr/context.hpp>
#include <vkr/gltf_model.hpp>
#include <vkr/pipeline.hpp>
#include <vkr/texture.hpp>
#include <vkr/window.hpp>

class Lighting {
public:
  Lighting(glm::vec3 lightColor, glm::vec3 lightPos)
      : uniformBuffer({
            sizeof(LightingUniform),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vkr::MemoryUsageFlagBits::eCpuToGpu,
            vkr::MemoryPropertyFlagBits::eHostVisible |
                vkr::MemoryPropertyFlagBits::eHostCoherent,
        }) {
    auto [descriptorPool, descriptorSetLayout] =
        vkr::Context::getDescriptorManager()[vkr::DESC_LIGHTING];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    this->descriptorSet =
        descriptorPool->allocateDescriptorSets({*descriptorSetLayout})[0];

    this->ubo.lightColor = glm::vec4(lightColor, 1.0f);
    this->ubo.lightPos = glm::vec4(lightPos, 1.0f);

    // TODO: store this uniform buffer on the GPU using staging buffer
    this->uniformBuffer.mapMemory(&this->mapped);
    memcpy(this->mapped, &this->ubo, sizeof(LightingUniform));

    auto bufferInfo = vk::DescriptorBufferInfo{
        this->uniformBuffer.getHandle(), 0, sizeof(LightingUniform)};

    vkr::Context::getDevice().updateDescriptorSets(
        {
            vk::WriteDescriptorSet{
                this->descriptorSet,                 // dstSet
                0,                                   // dstBinding
                0,                                   // dstArrayElement
                1,                                   // descriptorCount
                vkr::DescriptorType::eUniformBuffer, // descriptorType
                nullptr,                             // pImageInfo
                &bufferInfo,                         // pBufferInfo
                nullptr,                             // pTexelBufferView
            },
        },
        {});
  }

  ~Lighting() {
    if (this->uniformBuffer) {
      this->uniformBuffer.destroy();
    }
    auto descriptorPool =
        vkr::Context::getDescriptorManager().getPool(vkr::DESC_LIGHTING);

    assert(descriptorPool != nullptr);

    vkr::Context::getDevice().freeDescriptorSets(
        *descriptorPool, this->descriptorSet);
  }

  void bind(vkr::Window &window, vkr::GraphicsPipeline &pipeline) {
    window.getCurrentCommandBuffer().bindDescriptorSets(
        vkr::PipelineBindPoint::eGraphics,
        pipeline.getLayout(),
        3, // firstSet
        this->descriptorSet,
        {});
  }

private:
  struct LightingUniform {
    glm::vec4 lightColor;
    glm::vec4 lightPos;
  } ubo;

  vkr::Buffer uniformBuffer;
  void *mapped;
  vkr::DescriptorSet descriptorSet;
};

int main() {
  vkr::Window window("GLTF models");

  window.setMSAASamples(vkr::SampleCount::e4);

  window.clearColor = {0.52, 0.80, 0.92, 1.0};

  fstl::unique<vkr::Shader> modelShader{
      "../shaders/model_lit.vert",
      "../shaders/model_lit.frag",
  };

  fstl::unique<vkr::GraphicsPipeline> modelPipeline{
      window,
      *modelShader,
      vkr::GltfModel::getVertexFormat(),
      vkr::Context::getDescriptorManager().getDefaultSetLayouts(),
  };

  vkr::Camera camera({0.0, 0.0, 0.0});

  Lighting lighting({0.95, 0.80, 0.52}, {3.0, 3.0, 3.0});

  fstl::unique<vkr::GltfModel> helmet{
      window, "../assets/DamagedHelmet.glb", true};
  helmet->setPosition({2.0, 0.0, 0.0});
  fstl::unique<vkr::GltfModel> boombox{window, "../assets/BoomBox.glb"};
  boombox->setPosition({-2.0, 0.0, 0.0});
  boombox->setScale(glm::vec3{1.0, 1.0, 1.0} * 100.0f);

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
    camera.lookAt((helmet->getPosition() + boombox->getPosition()) / 2.0f);

    camera.update(window);

    helmet->setRotation({0.0, time * 100.0, 0.0});
    boombox->setRotation({0.0, time * 100.0, 0.0});
    lighting.bind(window, *modelPipeline);

    camera.bind(window, *modelPipeline);
    helmet->draw(window, *modelPipeline);
    boombox->draw(window, *modelPipeline);

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

  return 0;
}
