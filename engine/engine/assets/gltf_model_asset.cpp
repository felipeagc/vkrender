#include "gltf_model_asset.hpp"
#include "../scene.hpp"
#include <renderer/context.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>
#include <tiny_gltf.h>

using namespace engine;

template <>
void engine::loadAsset<GltfModelAsset>(
    const sdf::AssetBlock &asset, AssetManager &assetManager) {
  std::string path;
  bool flipUVs = false;

  for (auto &prop : asset.properties) {
    if (strcmp(prop.name, "path") == 0) {
      prop.get_string(path);
    } else if (strcmp(prop.name, "flip_uvs") == 0) {
      flipUVs = true;
    }
  }

  char str[128] = "";
  sprintf(str, "Loading GltfModel: %s", path.c_str());
  auto start = timeBegin(str);

  auto &a = assetManager.loadAssetIntoIndex<GltfModelAsset>(
      asset.index, path, flipUVs);
  a.identifier = asset.name;

  sprintf(str, "Finished loading: %s", path.c_str());
  timeEnd(start, str);
}

void GltfModelAsset::Material::load(const GltfModelAsset &model) {
  auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.material;

  for (uint32_t i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    this->descriptorSets[i] = setLayout.allocate();

    VkDescriptorImageInfo albedoDescriptorInfo = {};
    VkDescriptorImageInfo normalDescriptorInfo = {};
    VkDescriptorImageInfo metallicRoughnessDescriptorInfo = {};
    VkDescriptorImageInfo occlusionDescriptorInfo = {};
    VkDescriptorImageInfo emissiveDescriptorInfo = {};

    // Albedo
    if (this->baseColorTextureIndex != -1) {
      auto &albedoTexture = model.m_textures[this->baseColorTextureIndex];
      albedoDescriptorInfo = albedoTexture.getDescriptorInfo();
    } else {
      albedoDescriptorInfo = renderer::ctx().m_whiteTexture.getDescriptorInfo();
    }

    if (this->normalTextureIndex != -1) {
      auto &normalTexture = model.m_textures[this->normalTextureIndex];
      normalDescriptorInfo = normalTexture.getDescriptorInfo();
    } else {
      normalDescriptorInfo = renderer::ctx().m_whiteTexture.getDescriptorInfo();
    }

    // Metallic/roughness
    if (this->metallicRoughnessTextureIndex != -1) {
      auto &metallicRoughnessTexture =
          model.m_textures[this->metallicRoughnessTextureIndex];
      metallicRoughnessDescriptorInfo =
          metallicRoughnessTexture.getDescriptorInfo();
    } else {
      metallicRoughnessDescriptorInfo =
          renderer::ctx().m_whiteTexture.getDescriptorInfo();
    }

    // Occlusion
    if (this->occlusionTextureIndex != -1) {
      auto &occlusionTexture = model.m_textures[this->occlusionTextureIndex];
      occlusionDescriptorInfo = occlusionTexture.getDescriptorInfo();
    } else {
      occlusionDescriptorInfo =
          renderer::ctx().m_whiteTexture.getDescriptorInfo();
    }

    // Emissive
    if (this->emissiveTextureIndex != -1) {
      auto &emissiveTexture = model.m_textures[this->emissiveTextureIndex];
      emissiveDescriptorInfo = emissiveTexture.getDescriptorInfo();
    } else {
      emissiveDescriptorInfo =
          renderer::ctx().m_blackTexture.getDescriptorInfo();
    }

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
            this->descriptorSets[i],                   // dstSet
            1,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &normalDescriptorInfo,                     // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->descriptorSets[i],                   // dstSet
            2,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &metallicRoughnessDescriptorInfo,          // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->descriptorSets[i],                   // dstSet
            3,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &occlusionDescriptorInfo,                  // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->descriptorSets[i],                   // dstSet
            4,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &emissiveDescriptorInfo,                   // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device,
        ARRAYSIZE(descriptorWrites),
        descriptorWrites,
        0,
        nullptr);
  }
}

void GltfModelAsset::Primitive::setDimensions(glm::vec3 min, glm::vec3 max) {
  dimensions.min = min;
  dimensions.max = max;
  dimensions.size = max - min;
  dimensions.center = (min + max) / 2.0f;
  dimensions.radius = glm::distance(min, max) / 2.0f;
}

GltfModelAsset::Primitive::Primitive(
    uint32_t firstIndex, uint32_t indexCount, int materialIndex)
    : firstIndex(firstIndex),
      indexCount(indexCount),
      materialIndex(materialIndex) {}

GltfModelAsset::Mesh::Mesh(glm::mat4 matrix) {
  this->ubo.matrix = matrix;

  auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.mesh;

  for (uint32_t i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    this->descriptorSets[i] = setLayout.allocate();

    re_buffer_init_uniform(&this->uniformBuffers[i], sizeof(MeshUniform));

    // UniformBuffer
    re_buffer_map_memory(&this->uniformBuffers[i], &this->mappings[i]);
    memcpy(this->mappings[i], &this->ubo, sizeof(MeshUniform));

    VkDescriptorBufferInfo bufferInfo = {
        this->uniformBuffers[i].buffer, 0, sizeof(MeshUniform)};

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->descriptorSets[i],           // dstSet
            0,                                 // dstBinding
            0,                                 // dstArrayElement
            1,                                 // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            nullptr,                           // pImageInfo
            &bufferInfo,                       // pBufferInfo
            nullptr,                           // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device,
        ARRAYSIZE(descriptorWrites),
        descriptorWrites,
        0,
        nullptr);
  }
}

glm::mat4 GltfModelAsset::Node::localMatrix() {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
         glm::scale(glm::mat4(1.0f), scale) * this->matrix;
}

glm::mat4 GltfModelAsset::Node::getMatrix(GltfModelAsset &model) {
  glm::mat4 m = localMatrix();
  int p = this->parentIndex;
  while (p != -1) {
    m = model.m_nodes[p].localMatrix() * m;
    p = model.m_nodes[p].parentIndex;
  }
  return m;
}

void GltfModelAsset::Node::update(GltfModelAsset &model, uint32_t frameIndex) {
  if (this->meshIndex != -1) {
    Mesh &mesh = model.m_meshes[this->meshIndex];
    mesh.ubo.matrix = this->getMatrix(model);
    memcpy(mesh.mappings[frameIndex], &mesh.ubo, sizeof(Mesh::MeshUniform));
  }

  for (int childIndex : this->childrenIndices) {
    // this if maybe isn't needed
    if (childIndex != -1) {
      model.m_nodes[childIndex].update(model, frameIndex);
    }
  }
}

GltfModelAsset::GltfModelAsset(const std::string &path, bool flipUVs) {
  this->identifier = path;

  std::string dir = path.substr(0, path.find_last_of('/') + 1);

  tinygltf::TinyGLTF loader;
  tinygltf::Model model;

  std::string err, warn;

  // TODO: check file extension and load accordingly
  std::string gltfExt = path.substr(path.find_last_of('.'), path.size());
  bool ret;
  if (std::strncmp(gltfExt.c_str(), ".glb", gltfExt.length()) == 0) {
    ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
  } else {
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
  }

  if (!warn.empty()) {
    ftl::warn("GLTF: %s", warn.c_str());
  }

  if (!err.empty()) {
    ftl::error("GLTF: %s", err.c_str());
  }

  if (!ret) {
    throw std::runtime_error("Failed to parse GLTF model");
  }

  loadTextures(model);
  loadMaterials(model);

  std::vector<uint32_t> indices;
  std::vector<renderer::StandardVertex> vertices;

  tinygltf::Scene &scene = model.scenes[model.defaultScene];

  m_nodes.resize(model.nodes.size());
  m_meshes.resize(model.meshes.size());

  for (size_t i = 0; i < scene.nodes.size(); i++) {
    this->loadNode(-1, scene.nodes[i], model, indices, vertices, flipUVs);
  }

  for (size_t i = 0; i < scene.nodes.size(); i++) {
    for (uint32_t frameIndex = 0; frameIndex < renderer::MAX_FRAMES_IN_FLIGHT;
         frameIndex++) {
      m_nodes[i].update(*this, frameIndex);
    }
  }

  size_t vertexBufferSize = vertices.size() * sizeof(renderer::StandardVertex);
  // TODO: index buffer size could be larger than it needs to be
  // due to other index types
  size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  assert((vertexBufferSize > 0) && (indexBufferSize > 0));

  re_buffer_t staging_buffer;
  re_buffer_init_staging(
      &staging_buffer,
      vertexBufferSize > indexBufferSize ? vertexBufferSize : indexBufferSize);

  void *staging_pointer;
  re_buffer_map_memory(&staging_buffer, &staging_pointer);

  re_buffer_init_vertex(&m_vertexBuffer, vertexBufferSize);
  re_buffer_init_index(&m_indexBuffer, indexBufferSize);

  memcpy(staging_pointer, vertices.data(), vertexBufferSize);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &m_vertexBuffer, vertexBufferSize);

  memcpy(staging_pointer, indices.data(), indexBufferSize);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &m_indexBuffer, indexBufferSize);

  re_buffer_unmap_memory(&staging_buffer);
  re_buffer_destroy(&staging_buffer);

  this->getSceneDimensions();
}

GltfModelAsset::~GltfModelAsset() {
  re_buffer_destroy(&m_vertexBuffer);
  re_buffer_destroy(&m_indexBuffer);

  for (auto &mesh : m_meshes) {
    for (auto &uniformBuffer : mesh.uniformBuffers) {
      re_buffer_unmap_memory(&uniformBuffer);
      re_buffer_destroy(&uniformBuffer);
    }

    auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.material;
    for (auto &set : mesh.descriptorSets) {
      setLayout.free(set);
    }
  }

  for (auto &material : m_materials) {
    auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.material;
    for (auto &set : material.descriptorSets) {
      setLayout.free(set);
    }
  }

  m_materials.clear();

  for (auto &texture : m_textures) {
    if (texture)
      texture.destroy();
  }

  m_textures.clear();
}

void GltfModelAsset::loadMaterials(tinygltf::Model &model) {
  m_materials.resize(model.materials.size());
  for (size_t i = 0; i < model.materials.size(); i++) {
    auto &mat = model.materials[i];

    Material material;

    // Textures
    if (mat.values.find("baseColorTexture") != mat.values.end()) {
      material.baseColorTextureIndex =
          model.textures[mat.values["baseColorTexture"].TextureIndex()].source;
    }

    if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
      material.metallicRoughnessTextureIndex =
          model.textures[mat.values["metallicRoughnessTexture"].TextureIndex()]
              .source;
    }

    if (mat.additionalValues.find("normalTexture") !=
        mat.additionalValues.end()) {
      material.ubo.hasNormalTexture = 1.0f;
      material.normalTextureIndex =
          mat.additionalValues["normalTexture"].TextureIndex();
    }

    if (mat.additionalValues.find("emissiveTexture") !=
        mat.additionalValues.end()) {
      material.emissiveTextureIndex =
          mat.additionalValues["emissiveTexture"].TextureIndex();
    }

    if (mat.additionalValues.find("occlusionTexture") !=
        mat.additionalValues.end()) {
      material.occlusionTextureIndex =
          mat.additionalValues["occlusionTexture"].TextureIndex();
    }

    // Factors
    if (mat.values.find("baseColorFactor") != mat.values.end()) {
      material.ubo.baseColorFactor =
          glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
    }

    if (mat.values.find("metallicFactor") != mat.values.end()) {
      material.ubo.metallic =
          static_cast<float>(mat.values["metallicFactor"].Factor());
    }

    if (mat.values.find("roughnessFactor") != mat.values.end()) {
      material.ubo.roughness =
          static_cast<float>(mat.values["roughnessFactor"].Factor());
    }

    if (mat.additionalValues.find("emissiveFactor") !=
        mat.additionalValues.end()) {
      material.ubo.emissiveFactor = glm::vec4(
          glm::make_vec3(
              mat.additionalValues["emissiveFactor"].ColorFactor().data()),
          1.0);
    }

    material.load(*this);

    m_materials[i] = material;
  }
}

void GltfModelAsset::loadTextures(tinygltf::Model &model) {
  m_textures.resize(model.images.size());
  for (size_t i = 0; i < model.images.size(); i++) {
    if (model.images[i].component != 4) {
      // TODO: support RGB images
      throw std::runtime_error("Only 4-component images are supported.");
    }

    m_textures[i] =
        renderer::Texture{model.images[i].image,
                          static_cast<uint32_t>(model.images[i].width),
                          static_cast<uint32_t>(model.images[i].height)};
  }
}

void GltfModelAsset::loadNode(
    int parentIndex,
    int nodeIndex,
    const tinygltf::Model &model,
    std::vector<uint32_t> &indices,
    std::vector<renderer::StandardVertex> &vertices,
    bool flipUVs) {
  const tinygltf::Node &node = model.nodes[nodeIndex];

  Node &newNode = m_nodes[nodeIndex];
  newNode.index = nodeIndex;
  if (parentIndex != nodeIndex) {
    newNode.parentIndex = parentIndex;
  }
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
    newMesh = Mesh{newNode.matrix};

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
          renderer::StandardVertex vert{};

          vert.pos =
              glm::vec3(glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0));
          vert.normal = glm::normalize(
              bufferNormals ? glm::make_vec3(&bufferNormals[v * 3])
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
          ftl::error(
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

  if (newNode.parentIndex != -1) {
    m_nodes[newNode.parentIndex].childrenIndices.push_back(nodeIndex);
  }
}

void GltfModelAsset::getNodeDimensions(
    Node &node, glm::vec3 &min, glm::vec3 &max) {
  if (node.meshIndex != -1) {
    for (Primitive &primitive : m_meshes[node.meshIndex].primitives) {
      glm::vec4 locMin =
          node.getMatrix(*this) * glm::vec4(primitive.dimensions.min, 1.0f);
      glm::vec4 locMax =
          node.getMatrix(*this) * glm::vec4(primitive.dimensions.max, 1.0f);
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

void GltfModelAsset::getSceneDimensions() {
  dimensions.min = glm::vec3(FLT_MAX);
  dimensions.max = glm::vec3(-FLT_MAX);
  for (auto node : m_nodes) {
    getNodeDimensions(node, dimensions.min, dimensions.max);
  }
  dimensions.size = dimensions.max - dimensions.min;
  dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
  dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
}
