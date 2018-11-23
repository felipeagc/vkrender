#include "gltf_model_instance.hpp"
#include "context.hpp"
#include "util.hpp"

using namespace vkr;

GltfModelInstance::GltfModelInstance(const GltfModel &model) : model(model) {
  // Create uniform buffers and descriptors
  {
    auto [descriptorPool, descriptorSetLayout] =
        ctx::descriptorManager[DESC_MESH];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      buffer::makeUniformBuffer(
          sizeof(ModelUniform),
          &this->uniformBuffers.buffers[i],
          &this->uniformBuffers.allocations[i]);

      buffer::mapMemory(
          this->uniformBuffers.allocations[i], &this->mappings[i]);
      memcpy(this->mappings[i], &this->ubo, sizeof(ModelUniform));

      VkDescriptorBufferInfo bufferInfo = {
          this->uniformBuffers.buffers[i], 0, sizeof(ModelUniform)};

      VK_CHECK(vkAllocateDescriptorSets(
          ctx::device, &allocateInfo, &this->descriptorSets[i]));

      VkWriteDescriptorSet descriptorWrite = {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          nullptr,
          this->descriptorSets[i],          // dstSet
          0,                                 // dstBinding
          0,                                 // dstArrayElement
          1,                                 // descriptorCount
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
          nullptr,                           // pImageInfo
          &bufferInfo,                       // pBufferInfo
          nullptr,                           // pTexelBufferView
      };

      vkUpdateDescriptorSets(ctx::device, 1, &descriptorWrite, 0, nullptr);
    }
  }
}

void GltfModelInstance::destroy() {
  for (size_t i = 0; i < ARRAYSIZE(uniformBuffers.buffers); i++) {
    buffer::unmapMemory(uniformBuffers.allocations[i]);
    buffer::destroy(uniformBuffers.buffers[i], uniformBuffers.allocations[i]);
  }

  auto descriptorPool = ctx::descriptorManager.getPool(DESC_MESH);
  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      ctx::device,
      *descriptorPool,
      descriptorSets.size(),
      descriptorSets.data());
}

void GltfModelInstance::draw(Window &window, GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  this->updateUniforms(i);

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      commandBuffer, 0, 1, &this->model.vertexBuffer_, &offset);

  vkCmdBindIndexBuffer(
      commandBuffer, this->model.indexBuffer_, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.pipelineLayout,
      2, // firstSet
      1,
      &this->descriptorSets[i],
      0,
      nullptr);

  for (auto &node : this->model.nodes_) {
    drawNode(node, window, pipeline);
  }
}

void GltfModelInstance::updateUniforms(int frameIndex) {
  auto translation = glm::translate(glm::mat4(1.0f), this->pos);
  auto rotation = glm::rotate(
      glm::mat4(1.0f), glm::radians(this->rotation.x), {1.0, 0.0, 0.0});
  rotation =
      glm::rotate(rotation, glm::radians(this->rotation.y), {0.0, 1.0, 0.0});
  rotation =
      glm::rotate(rotation, glm::radians(this->rotation.z), {0.0, 0.0, 1.0});
  auto scaling = glm::scale(glm::mat4(1.0f), this->scale);

  this->ubo.model = translation * rotation * scaling;

  memcpy(this->mappings[frameIndex], &this->ubo, sizeof(ModelUniform));
}

void GltfModelInstance::drawNode(
    GltfModel::Node &node, Window &window, GraphicsPipeline &pipeline) {
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  if (node.meshIndex != -1) {
    for (GltfModel::Primitive &primitive :
         this->model.meshes_[node.meshIndex].primitives) {
      if (primitive.materialIndex != -1 && this->model.materials_.size() > 0) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.pipelineLayout,
            1, // firstSet
            1,
            &this->model.materials_[primitive.materialIndex].descriptorSets[i],
            0,
            nullptr);
      }

      vkCmdDrawIndexed(
          commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
    }
  }

  for (auto &childIndex : node.childrenIndices) {
    drawNode(this->model.nodes_[childIndex], window, pipeline);
  }
}
