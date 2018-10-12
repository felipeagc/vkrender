#pragma once

#include "buffer.hpp"
#include "commandbuffer.hpp"
#include "pipeline.hpp"
#include "texture.hpp"
#include "window.hpp"
#include <string>
#include <tiny_gltf.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vkr {
class GltfModel {
public:
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
  };

  struct Material {
    int albedoTextureIndex = -1;
    vk::DescriptorSet descriptorSet;

    // Call this after texture index is established
    void init(GltfModel &model);
  };

  struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount = 0;
    int materialIndex = -1;

    struct Dimensions {
      glm::vec3 min = glm::vec3(FLT_MAX);
      glm::vec3 max = glm::vec3(-FLT_MAX);
      glm::vec3 size;
      glm::vec3 center;
      float radius;
    } dimensions;

    void setDimensions(glm::vec3 min, glm::vec3 max);

    Primitive(uint32_t firstIndex, uint32_t indexCount, int materialIndex);
  };

  struct Mesh {
    std::vector<Primitive> primitives;
    Buffer uniformBuffer;
    void *mapped;
    vk::DescriptorBufferInfo bufferInfo;
    vk::DescriptorSet descriptorSet;

    struct ModelUniform {
      glm::mat4 model;
    } modelUniform;

    Mesh(){};
    Mesh(glm::mat4 matrix);
  };

  struct Node {
    int parentIndex = -1;
    int index = -1;
    std::vector<int> childrenIndices;
    glm::mat4 matrix;
    std::string name;
    int meshIndex = -1;
    glm::vec3 translation{};
    glm::vec3 scale{1.0f};
    glm::quat rotation{};

    glm::mat4 localMatrix();
    glm::mat4 getMatrix(GltfModel &model);

    void update(GltfModel &model);
  };

  struct Dimensions {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
    glm::vec3 size;
    glm::vec3 center;
    float radius;
  } dimensions;

  static VertexFormat getVertexFormat() {
    return VertexFormatBuilder()
        .addBinding(0, sizeof(GltfModel::Vertex), vk::VertexInputRate::eVertex)
        .addAttribute(
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(GltfModel::Vertex, pos))
        .addAttribute(
            1,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(GltfModel::Vertex, normal))
        .addAttribute(
            2, 0, vk::Format::eR32G32Sfloat, offsetof(GltfModel::Vertex, uv))
        .build();
  }

  GltfModel(const std::string &path);
  ~GltfModel();
  GltfModel(const GltfModel &other) = default;
  GltfModel &operator=(GltfModel &other) = default;

  void draw(vkr::CommandBuffer &commandBuffer, vkr::GraphicsPipeline &pipeline);

  void destroy();

private:
  std::vector<Node> nodes;
  std::vector<Mesh> meshes;
  std::vector<Texture> textures;
  std::vector<Material> materials;

  Buffer vertexBuffer;
  Buffer indexBuffer;
  uint32_t indexCount;

  void loadMaterials(tinygltf::Model &model);

  void loadTextures(tinygltf::Model &model);

  void loadNode(
      int parentNodeIndex,
      int nodeIndex,
      const tinygltf::Model &model,
      std::vector<uint32_t> &indices,
      std::vector<Vertex> &vertices);

  void getNodeDimensions(Node &node, glm::vec3 &min, glm::vec3 &max);

  void getSceneDimensions();

  void drawNode(
      Node &node,
      vkr::CommandBuffer &commandBuffer,
      vkr::GraphicsPipeline &pipeline);
};
} // namespace vkr
