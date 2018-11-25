#include "gltf_model.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"
#include <algorithm>
#include <fstl/logging.hpp>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <tiny_gltf.h>

using namespace vkr;

GltfModel::Material::Material(
    const GltfModel &model, int albedoTextureIndex, glm::vec4 baseColorFactor)
    : albedoTextureIndex(albedoTextureIndex) {
  auto [descriptorPool, descriptorSetLayout] =
      ctx().m_descriptorManager[DESC_MATERIAL];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  this->ubo.baseColorFactor = baseColorFactor;

  VkDescriptorSetAllocateInfo allocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      *descriptorPool,
      1,
      descriptorSetLayout,
  };

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VK_CHECK(vkAllocateDescriptorSets(
        ctx().m_device, &allocateInfo, &this->descriptorSets[i]));

    buffer::createUniformBuffer(
        sizeof(MaterialUniform),
        &this->uniformBuffers.buffers[i],
        &this->uniformBuffers.allocations[i]);

    // CombinedImageSampler
    auto &texture = model.m_textures[this->albedoTextureIndex];
    auto albedoDescriptorInfo = texture.getDescriptorInfo();

    // UniformBuffer
    buffer::mapMemory(this->uniformBuffers.allocations[i], &this->mappings[i]);
    memcpy(this->mappings[i], &this->ubo, sizeof(MaterialUniform));

    VkDescriptorBufferInfo bufferInfo = {
        this->uniformBuffers.buffers[i], 0, sizeof(MaterialUniform)};

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->descriptorSets[i],                   // dstSet
            0,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &albedoDescriptorInfo,                     // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->descriptorSets[i],           // dstSet
            1,                                 // dstBinding
            0,                                 // dstArrayElement
            1,                                 // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            nullptr,                           // pImageInfo
            &bufferInfo,                       // pBufferInfo
            nullptr,                           // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        ctx().m_device,
        ARRAYSIZE(descriptorWrites),
        descriptorWrites,
        0,
        nullptr);
  }
}

void GltfModel::Primitive::setDimensions(glm::vec3 min, glm::vec3 max) {
  dimensions.min = min;
  dimensions.max = max;
  dimensions.size = max - min;
  dimensions.center = (min + max) / 2.0f;
  dimensions.radius = glm::distance(min, max) / 2.0f;
}

GltfModel::Primitive::Primitive(
    uint32_t firstIndex, uint32_t indexCount, int materialIndex)
    : firstIndex(firstIndex),
      indexCount(indexCount),
      materialIndex(materialIndex) {}

// void GltfModel::Mesh::updateUniform(int frameIndex) {
//   memcpy(this->mappings[frameIndex], &this->ubo, sizeof(MeshUniform));
// }

glm::mat4 GltfModel::Node::localMatrix() {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
         glm::scale(glm::mat4(1.0f), scale) * matrix;
}

glm::mat4 GltfModel::Node::getMatrix(GltfModel &model) {
  glm::mat4 m = localMatrix();
  int p = this->parentIndex;
  while (p != -1) {
    m = model.m_nodes[this->parentIndex].localMatrix() * m;
    p = model.m_nodes[this->parentIndex].parentIndex;
  }

  return m;
}

GltfModel::GltfModel(const std::string &path) {
  bool flipUVs = false;

  std::string ext = path.substr(path.find_last_of('.'), path.size());

  std::string dir = path.substr(0, path.find_last_of('/') + 1);

  // Load json asset descriptor
  std::string gltfPath;
  if (std::strncmp(ext.c_str(), ".json", ext.length()) == 0) {
    std::ifstream file(path);
    if (file.fail()) {
      throw std::runtime_error("Failed to load asset json file");
    }

    rapidjson::IStreamWrapper isw(file);

    rapidjson::Document doc;
    doc.ParseStream(isw);

    assert(doc["path"].IsString());

    gltfPath = dir + doc["path"].GetString();

    if (doc.HasMember("parameters") && doc["parameters"].HasMember("flipUVs") &&
        doc["parameters"]["flipUVs"].IsBool()) {
      flipUVs = doc["parameters"]["flipUVs"].GetBool();
    }

    file.close();
  } else {
    gltfPath = path;
  }

  tinygltf::TinyGLTF loader;
  tinygltf::Model model;

  std::string err, warn;

  // TODO: check file extension and load accordingly
  std::string gltfExt =
      gltfPath.substr(gltfPath.find_last_of('.'), path.size());
  bool ret;
  if (std::strncmp(gltfExt.c_str(), ".glb", gltfExt.length()) == 0) {
    ret = loader.LoadBinaryFromFile(&model, &err, &warn, gltfPath);
  } else {
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltfPath);
  }

  if (!warn.empty()) {
    fstl::log::warn("GLTF: {}", warn);
  }

  if (!err.empty()) {
    fstl::log::error("GLTF: {}", err);
  }

  if (!ret) {
    throw std::runtime_error("Failed to parse GLTF model");
  }

  loadTextures(model);
  loadMaterials(model);

  std::vector<uint32_t> indices;
  std::vector<StandardVertex> vertices;

  tinygltf::Scene &scene = model.scenes[model.defaultScene];

  m_nodes.resize(model.nodes.size());
  m_meshes.resize(model.meshes.size());

  for (size_t i = 0; i < scene.nodes.size(); i++) {
    this->loadNode(-1, scene.nodes[i], model, indices, vertices, flipUVs);
  }

  size_t vertexBufferSize = vertices.size() * sizeof(StandardVertex);
  // TODO: index buffer size could be larger than it needs to be
  // due to other index types
  size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  assert((vertexBufferSize > 0) && (indexBufferSize > 0));

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  buffer::createStagingBuffer(
      std::max(vertexBufferSize, indexBufferSize),
      &stagingBuffer,
      &stagingAllocation);

  void *stagingMemoryPointer;
  buffer::mapMemory(stagingAllocation, &stagingMemoryPointer);

  buffer::createVertexBuffer(
      vertexBufferSize, &m_vertexBuffer, &m_vertexAllocation);

  buffer::createIndexBuffer(
      indexBufferSize, &m_indexBuffer, &m_indexAllocation);

  memcpy(stagingMemoryPointer, vertices.data(), vertexBufferSize);
  buffer::bufferTransfer(stagingBuffer, m_vertexBuffer, vertexBufferSize);

  memcpy(stagingMemoryPointer, indices.data(), indexBufferSize);
  buffer::bufferTransfer(stagingBuffer, m_indexBuffer, indexBufferSize);

  buffer::unmapMemory(stagingAllocation);
  buffer::destroy(stagingBuffer, stagingAllocation);
}

GltfModel::operator bool() const {
  return (m_vertexBuffer != VK_NULL_HANDLE);
}

void GltfModel::destroy() {
  if (!*this) {
    return;
  }

  buffer::destroy(m_vertexBuffer, m_vertexAllocation);
  buffer::destroy(m_indexBuffer, m_indexAllocation);

  m_vertexBuffer = VK_NULL_HANDLE;
  m_indexBuffer = VK_NULL_HANDLE;

  for (auto &material : m_materials) {
    for (size_t i = 0; i < ARRAYSIZE(material.uniformBuffers.buffers); i++) {
      buffer::unmapMemory(material.uniformBuffers.allocations[i]);
      buffer::destroy(
          material.uniformBuffers.buffers[i],
          material.uniformBuffers.allocations[i]);
    }

    auto descriptorPool = ctx().m_descriptorManager.getPool(DESC_MATERIAL);

    assert(descriptorPool != nullptr);

    vkFreeDescriptorSets(
        ctx().m_device,
        *descriptorPool,
        material.descriptorSets.size(),
        material.descriptorSets.data());
  }

  m_materials.clear();

  for (auto &texture : m_textures) {
    if (texture)
      texture.destroy();
  }

  m_textures.clear();
}

void GltfModel::loadMaterials(tinygltf::Model &model) {
  m_materials.resize(model.materials.size());
  for (size_t i = 0; i < model.materials.size(); i++) {
    auto &mat = model.materials[i];

    int albedoTextureIndex = -1;
    glm::vec4 baseColorFactor = glm::vec4(1.0, 1.0, 1.0, 1.0);

    if (mat.values.find("baseColorTexture") != mat.values.end()) {
      albedoTextureIndex =
          model.textures[mat.values["baseColorTexture"].TextureIndex()].source;
    }

    if (mat.values.find("baseColorFactor") != mat.values.end()) {
      baseColorFactor =
          glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
    }

    m_materials[i] = Material{*this, albedoTextureIndex, baseColorFactor};
  }
}

void GltfModel::loadTextures(tinygltf::Model &model) {
  m_textures.resize(model.images.size());
  for (size_t i = 0; i < model.images.size(); i++) {
    if (model.images[i].component != 4) {
      // TODO: support RGB images
      throw std::runtime_error("Only 4-component images are supported.");
    }

    m_textures[i] =
        Texture{model.images[i].image,
                static_cast<uint32_t>(model.images[i].width),
                static_cast<uint32_t>(model.images[i].height)};
  }
}

void GltfModel::loadNode(
    int parentIndex,
    int nodeIndex,
    const tinygltf::Model &model,
    std::vector<uint32_t> &indices,
    std::vector<StandardVertex> &vertices,
    bool flipUVs) {
  const tinygltf::Node &node = model.nodes[nodeIndex];

  Node &newNode = m_nodes[nodeIndex];
  newNode.index = nodeIndex;
  newNode.parentIndex = parentIndex;
  newNode.name = node.name;
  newNode.matrix = glm::mat4(1.0f);

  // Generate local node matrix
  if (node.translation.size() == 3) {
    newNode.translation = glm::make_vec3(node.translation.data());
  }
  if (node.rotation.size() == 4) {
    glm::quat q = glm::make_quat(node.rotation.data());
    newNode.rotation = glm::mat4(q);
  }
  if (node.scale.size() == 3) {
    newNode.scale = glm::make_vec3(node.scale.data());
  }
  if (node.matrix.size() == 16) {
    newNode.matrix = glm::make_mat4x4(node.matrix.data());
  };

  // Node with children
  if (node.children.size() > 0) {
    for (size_t i = 0; i < node.children.size(); i++) {
      loadNode(nodeIndex, node.children[i], model, indices, vertices, flipUVs);
    }
  }

  if (node.mesh > -1) {
    const tinygltf::Mesh mesh = model.meshes[node.mesh];
    Mesh &newMesh = m_meshes[node.mesh];

    for (size_t j = 0; j < mesh.primitives.size(); j++) {
      const tinygltf::Primitive &primitive = mesh.primitives[j];

      if (primitive.indices < 0) {
        continue;
      }

      uint32_t indexStart = static_cast<uint32_t>(indices.size());
      uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
      uint32_t indexCount = 0;
      glm::vec3 posMin{};
      glm::vec3 posMax{};

      // Vertices
      {
        const float *bufferPos = nullptr;
        const float *bufferNormals = nullptr;
        const float *bufferTexCoords = nullptr;

        assert(
            primitive.attributes.find("POSITION") !=
            primitive.attributes.end());

        const tinygltf::Accessor &posAccessor =
            model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView &posView =
            model.bufferViews[posAccessor.bufferView];
        bufferPos = reinterpret_cast<const float *>(
            &(model.buffers[posView.buffer]
                  .data[posAccessor.byteOffset + posView.byteOffset]));
        posMin = glm::vec3(
            posAccessor.minValues[0],
            posAccessor.minValues[1],
            posAccessor.minValues[2]);
        posMax = glm::vec3(
            posAccessor.maxValues[0],
            posAccessor.maxValues[1],
            posAccessor.maxValues[2]);

        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
          const tinygltf::Accessor &normAccessor =
              model.accessors[primitive.attributes.find("NORMAL")->second];

          const tinygltf::BufferView &normView =
              model.bufferViews[normAccessor.bufferView];

          bufferNormals = reinterpret_cast<const float *>(
              &(model.buffers[normView.buffer]
                    .data[normAccessor.byteOffset + normView.byteOffset]));
        }

        if (primitive.attributes.find("TEXCOORD_0") !=
            primitive.attributes.end()) {
          const tinygltf::Accessor &uvAccessor =
              model.accessors[primitive.attributes.find("TEXCOORD_0")->second];

          const tinygltf::BufferView &uvView =
              model.bufferViews[uvAccessor.bufferView];

          bufferTexCoords = reinterpret_cast<const float *>(
              &(model.buffers[uvView.buffer]
                    .data[uvAccessor.byteOffset + uvView.byteOffset]));
        }

        for (size_t v = 0; v < posAccessor.count; v++) {
          StandardVertex vert{};

          // TODO: fix rendundancies and see if it works?
          // TODO: this is a mess.
          vert.pos = glm::vec3(
              newNode.getMatrix(*this) *
              glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0));
          vert.normal = glm::normalize(
              bufferNormals ? glm::mat3(glm::transpose(
                                  glm::inverse(newNode.getMatrix(*this)))) *
                                  glm::make_vec3(&bufferNormals[v * 3])
                            : glm::vec3(0.0f));
          vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2])
                                    : glm::vec2(0.0f);

          if (flipUVs) {
            vert.uv = glm::vec2(vert.uv.x, 1.0f - vert.uv.y);
          }

          vertices.push_back(vert);
        }
      }

      // Indices
      {
        const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
        const tinygltf::BufferView &bufferView =
            model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

        indexCount = static_cast<uint32_t>(accessor.count);

        switch (accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
          std::vector<uint32_t> buf(accessor.count);
          memcpy(
              buf.data(),
              &buffer.data[accessor.byteOffset + bufferView.byteOffset],
              accessor.count * sizeof(uint32_t));
          for (size_t index = 0; index < accessor.count; index++) {
            indices.push_back(buf[index] + vertexStart);
          }
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
          std::vector<uint16_t> buf(accessor.count);
          memcpy(
              buf.data(),
              &buffer.data[accessor.byteOffset + bufferView.byteOffset],
              accessor.count * sizeof(uint16_t));
          for (size_t index = 0; index < accessor.count; index++) {
            indices.push_back(buf[index] + vertexStart);
          }
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
          std::vector<uint8_t> buf(accessor.count);
          memcpy(
              buf.data(),
              &buffer.data[accessor.byteOffset + bufferView.byteOffset],
              accessor.count * sizeof(uint8_t));
          for (size_t index = 0; index < accessor.count; index++) {
            indices.push_back(buf[index] + vertexStart);
          }
          break;
        }
        default:
          fstl::log::error(
              "GLTF: Index component type {} not supported!",
              accessor.componentType);
          return;
        }
      }

      Primitive newPrimitive(indexStart, indexCount, primitive.material);
      newPrimitive.setDimensions(posMin, posMax);
      newMesh.primitives.push_back(newPrimitive);
    }

    newNode.meshIndex = node.mesh;
  }

  if (parentIndex != -1) {
    m_nodes[parentIndex].childrenIndices.push_back(nodeIndex);
  }
}

void GltfModel::getNodeDimensions(Node &node, glm::vec3 &min, glm::vec3 &max) {
  if (node.meshIndex != -1) {
    for (Primitive &primitive : m_meshes[node.meshIndex].primitives) {
      glm::vec4 locMin =
          glm::vec4(primitive.dimensions.min, 1.0f) * node.getMatrix(*this);
      glm::vec4 locMax =
          glm::vec4(primitive.dimensions.max, 1.0f) * node.getMatrix(*this);
      if (locMin.x < min.x) {
        min.x = locMin.x;
      }
      if (locMin.y < min.y) {
        min.y = locMin.y;
      }
      if (locMin.z < min.z) {
        min.z = locMin.z;
      }
      if (locMax.x > max.x) {
        max.x = locMax.x;
      }
      if (locMax.y > max.y) {
        max.y = locMax.y;
      }
      if (locMax.z > max.z) {
        max.z = locMax.z;
      }
    }
  }
  for (auto childIndex : node.childrenIndices) {
    getNodeDimensions(m_nodes[childIndex], min, max);
  }
}

void GltfModel::getSceneDimensions() {
  dimensions.min = glm::vec3(FLT_MAX);
  dimensions.max = glm::vec3(-FLT_MAX);
  for (auto node : m_nodes) {
    getNodeDimensions(node, dimensions.min, dimensions.max);
  }
  dimensions.size = dimensions.max - dimensions.min;
  dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
  dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
}
