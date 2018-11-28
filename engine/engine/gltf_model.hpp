#pragma once

#include <renderer/buffer.hpp>
#include <renderer/glm.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/texture.hpp>
#include <renderer/window.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace tinygltf {
class Model;
}

namespace engine {
// Represents a glTF model asset
// Can be treated as a handle to the resource

class GltfModel {
  friend class GltfModelComponent;

public:
  struct Material {
    int albedoTextureIndex = -1;

    struct MaterialUniform {
      glm::vec4 baseColorFactor;
    } ubo;

    renderer::buffer::Buffers<renderer::MAX_FRAMES_IN_FLIGHT> uniformBuffers;
    std::array<void *, renderer::MAX_FRAMES_IN_FLIGHT> mappings;
    std::array<VkDescriptorSet, renderer::MAX_FRAMES_IN_FLIGHT> descriptorSets;

    Material() {}
    Material(
        const GltfModel &model,
        int albedoTextureIndex,
        glm::vec4 baseColorFactor);
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
  };

  struct Node {
    int parentIndex = -1;
    int index = -1;
    std::vector<int> childrenIndices;
    glm::mat4 matrix{1.0f};
    std::string name;
    int meshIndex = -1;
    glm::vec3 translation{};
    glm::vec3 scale{1.0f};
    glm::quat rotation{};

    glm::mat4 localMatrix();
    glm::mat4 getMatrix(GltfModel &model);
  };

  struct Dimensions {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
    glm::vec3 size;
    glm::vec3 center;
    float radius;
  } dimensions;

  GltfModel(){};
  GltfModel(const std::string &path);

  operator bool() const;

  void destroy();

protected:
  std::vector<Node> m_nodes;
  std::vector<Mesh> m_meshes;
  std::vector<renderer::Texture> m_textures;
  std::vector<Material> m_materials;

  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VmaAllocation m_vertexAllocation = VK_NULL_HANDLE;
  VkBuffer m_indexBuffer = VK_NULL_HANDLE;
  VmaAllocation m_indexAllocation = VK_NULL_HANDLE;

  void loadMaterials(tinygltf::Model &model);

  void loadTextures(tinygltf::Model &model);

  void loadNode(
      int parentNodeIndex,
      int nodeIndex,
      const tinygltf::Model &model,
      std::vector<uint32_t> &indices,
      std::vector<renderer::StandardVertex> &vertices,
      bool flipUVs);

  void getNodeDimensions(Node &node, glm::vec3 &min, glm::vec3 &max);

  void getSceneDimensions();
};
} // namespace engine
