#pragma once

#include "../asset_manager.hpp"
#include <renderer/buffer.hpp>
#include <renderer/common.hpp>
#include <renderer/glm.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/resource_manager.hpp>
#include <renderer/texture.hpp>
#include <scene/scene.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace tinygltf {
class Model;
}

namespace engine {
// Represents a glTF model asset
// Can be treated as a handle to the resource

class GltfModelAsset : public Asset {
  friend class GltfModelComponent;

public:
  struct Material {
    int baseColorTextureIndex = -1;
    int metallicRoughnessTextureIndex = -1;

    int normalTextureIndex = -1;
    int emissiveTextureIndex = -1;
    int occlusionTextureIndex = -1;

    struct MaterialUniform {
      glm::vec4 baseColorFactor = glm::vec4(1.0);
      float metallic = 1.0;
      float roughness = 1.0;
      glm::vec4 emissiveFactor = glm::vec4(0.0);
      float hasNormalTexture = 0.0f;
    } ubo;

    renderer::ResourceSet descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

    void load(const GltfModelAsset &model);
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

    struct MeshUniform {
      glm::mat4 matrix;
    } ubo;

    renderer::Buffer uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];
    void *mappings[renderer::MAX_FRAMES_IN_FLIGHT];
    renderer::ResourceSet descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

    Mesh() {}
    Mesh(glm::mat4 matrix);
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
    glm::mat4 getMatrix(GltfModelAsset &model);

    // Call this after loading all nodes
    // And after updating animations
    void update(GltfModelAsset &model, uint32_t frameIndex);
  };

  struct Dimensions {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
    glm::vec3 size;
    glm::vec3 center;
    float radius;
  } dimensions;

  GltfModelAsset(){};
  GltfModelAsset(const std::string &path, bool flipUVs = false);

  ~GltfModelAsset();

  // GltfModelAsset cannot be copied
  GltfModelAsset(const GltfModelAsset &) = delete;
  GltfModelAsset &operator=(const GltfModelAsset &) = delete;

  // GltfModelAsset cannot be moved
  GltfModelAsset(GltfModelAsset &&) = delete;
  GltfModelAsset &operator=(GltfModelAsset &&) = delete;

  operator bool() const;

  // TODO: these vectors are bad for data locality
  std::vector<Node> m_nodes;
  std::vector<Mesh> m_meshes;
  std::vector<renderer::Texture> m_textures;
  std::vector<Material> m_materials;

protected:
  renderer::Buffer m_vertexBuffer;
  renderer::Buffer m_indexBuffer;

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
