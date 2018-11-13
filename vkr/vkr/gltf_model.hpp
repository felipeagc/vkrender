#pragma once

#include "buffer.hpp"
#include "commandbuffer.hpp"
#include "vertex_format.hpp"
#include "texture.hpp"
#include "window.hpp"
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace tinygltf {
class Model;
}

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

    struct MaterialUniform {
      glm::vec4 baseColorFactor;
    } ubo;

    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers;
    std::array<void *, MAX_FRAMES_IN_FLIGHT> mappings;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

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
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers;
    std::array<void *, MAX_FRAMES_IN_FLIGHT> mappings;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    struct MeshUniform {
      glm::mat4 model;
    } ubo;

    void updateUniform(int frameIndex);

    Mesh() {}
    Mesh(const glm::mat4 &matrix);
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

    void update(GltfModel &model, int frameIndex);
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

  GltfModel(Window &window, const std::string &path, bool flipUVs = false);
  ~GltfModel();
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

private:
  std::vector<Node> nodes;
  std::vector<Mesh> meshes;
  std::vector<Texture> textures;
  std::vector<Material> materials;

  glm::vec3 pos = {0.0, 0.0, 0.0};
  glm::vec3 scale = {1.0, 1.0, 1.0};
  glm::vec3 rotation = {0.0, 0.0, 0.0};

  Buffer vertexBuffer;
  Buffer indexBuffer;

  void loadMaterials(tinygltf::Model &model);

  void loadTextures(tinygltf::Model &model);

  void loadNode(
      int parentNodeIndex,
      int nodeIndex,
      const tinygltf::Model &model,
      std::vector<uint32_t> &indices,
      std::vector<Vertex> &vertices,
      bool flipUVs);

  void getNodeDimensions(Node &node, glm::vec3 &min, glm::vec3 &max);

  void getSceneDimensions();

  void drawNode(Node &node, Window &window, GraphicsPipeline &pipeline);
};
} // namespace vkr
