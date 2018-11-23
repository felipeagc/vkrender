#pragma once

#include "buffer.hpp"
#include "pipeline.hpp"
#include "texture.hpp"
#include "window.hpp"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace tinygltf {
class Model;
}

namespace vkr {
struct GraphicsPipeline;

// Represents a glTF model asset
// Can be treated as a handle to the resource
class GltfModel {
  friend struct GltfModelInstance;

public:
  struct Material {
    int albedoTextureIndex = -1;

    struct MaterialUniform {
      glm::vec4 baseColorFactor;
    } ubo;

    buffer::Buffers<MAX_FRAMES_IN_FLIGHT> uniformBuffers;
    std::array<void *, MAX_FRAMES_IN_FLIGHT> mappings;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

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

  // TODO: instead of constructor use load functions

  GltfModel(const std::string &path, bool flipUVs = false);
  ~GltfModel(){};
  GltfModel(const GltfModel &other) = default;
  GltfModel &operator=(GltfModel &other) = default;

  void draw(Window &window, GraphicsPipeline &pipeline);

  void setPosition(glm::vec3 pos);
  glm::vec3 getPosition() const;

  void setRotation(glm::vec3 rotation);
  glm::vec3 getRotation() const;

  void setScale(glm::vec3 scale);
  glm::vec3 getScale() const;

  void destroy();

protected:
  glm::vec3 pos_ = {0.0, 0.0, 0.0};
  glm::vec3 scale_ = {1.0, 1.0, 1.0};
  glm::vec3 rotation_ = {0.0, 0.0, 0.0};

  struct ModelUniform {
    glm::mat4 model{1.0};
  } ubo;

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> uniformBuffers_;
  std::array<void *, MAX_FRAMES_IN_FLIGHT> mappings_;
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

  // Run every frame to update the uniform transform matrix with
  // updated position, scale and rotations
  void updateUniforms(int frameIndex);

  void drawNode(Node &node, Window &window, GraphicsPipeline &pipeline);

  std::vector<Node> nodes_;
  std::vector<Mesh> meshes_;
  std::vector<Texture> textures_;
  std::vector<Material> materials_;

  VkBuffer vertexBuffer_;
  VmaAllocation vertexAllocation_;
  VkBuffer indexBuffer_;
  VmaAllocation indexAllocation_;

  void loadMaterials(tinygltf::Model &model);

  void loadTextures(tinygltf::Model &model);

  void loadNode(
      int parentNodeIndex,
      int nodeIndex,
      const tinygltf::Model &model,
      std::vector<uint32_t> &indices,
      std::vector<StandardVertex> &vertices,
      bool flipUVs);

  void getNodeDimensions(Node &node, glm::vec3 &min, glm::vec3 &max);

  void getSceneDimensions();
};
} // namespace vkr
