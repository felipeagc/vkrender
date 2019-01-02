#include "gltf_model_system.hpp"

#include "../assets/shape_asset.hpp"
#include "../components/camera_component.hpp"
#include "../components/environment_component.hpp"
#include "../components/gltf_model_component.hpp"
#include "../components/transform_component.hpp"
#include <renderer/window.hpp>

using namespace engine;

GltfModelSystem::GltfModelSystem(AssetManager &assetManager) {
  m_boxAsset = assetManager
                   .loadAsset<engine::ShapeAsset>(
                       engine::BoxShape(glm::vec3(-1.0), glm::vec3(1.0)))
                   .index();
}

GltfModelSystem::~GltfModelSystem() {}

void GltfModelSystem::process(
    renderer::Window &window,
    AssetManager &assetManager,
    ecs::World &world,
    renderer::GraphicsPipeline &modelPipeline,
    renderer::GraphicsPipeline &boxPipeline) {
  // Draw models
  ecs::Entity camera = world.first<engine::CameraComponent>();
  ecs::Entity skybox = world.first<engine::EnvironmentComponent>();

  world.getComponent<engine::CameraComponent>(camera)->bind(
      window, modelPipeline);

  world.getComponent<engine::EnvironmentComponent>(skybox)->bind(
      window, modelPipeline, 4);

  world.each<engine::GltfModelComponent, engine::TransformComponent>(
      [&](ecs::Entity,
          engine::GltfModelComponent &model,
          engine::TransformComponent &transform) {
        model.draw(window, assetManager, modelPipeline, transform.getMatrix());
      });

  auto &box = assetManager.getAsset<ShapeAsset>(m_boxAsset);

  world.getComponent<engine::CameraComponent>(camera)->bind(
      window, boxPipeline);

  world.each<engine::GltfModelComponent, engine::TransformComponent>(
      [&](ecs::Entity,
          engine::GltfModelComponent &model,
          engine::TransformComponent &) {
        vkCmdBindPipeline(
            window.getCurrentCommandBuffer(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            boxPipeline.m_pipeline);

        auto &modelAsset =
            assetManager.getAsset<GltfModelAsset>(model.m_modelIndex);

        struct {
          alignas(16) glm::vec3 scale;
          alignas(16) glm::vec3 offset;
        } pushConstant;
        pushConstant.scale = (modelAsset.dimensions.size) / 2.0f;
        pushConstant.offset = modelAsset.dimensions.center;

        vkCmdPushConstants(
            window.getCurrentCommandBuffer(),
            boxPipeline.m_pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(pushConstant),
            &pushConstant);

        VkDeviceSize offset = 0;
        VkBuffer vertexBuffer = box.m_vertexBuffer.getHandle();
        vkCmdBindVertexBuffers(
            window.getCurrentCommandBuffer(), 0, 1, &vertexBuffer, &offset);

        vkCmdBindIndexBuffer(
            window.getCurrentCommandBuffer(),
            box.m_indexBuffer.getHandle(),
            0,
            VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(
            window.getCurrentCommandBuffer(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            boxPipeline.m_pipelineLayout,
            1, // firstSet
            1,
            model.m_descriptorSets[window.getCurrentFrameIndex()],
            0,
            nullptr);

        vkCmdDrawIndexed(
            window.getCurrentCommandBuffer(),
            box.m_shape.m_indices.size(),
            1,
            0,
            0,
            0);
      });
}
