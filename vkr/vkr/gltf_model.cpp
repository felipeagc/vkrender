#include "gltf_model.hpp"
#include "context.hpp"
#include <fstl/logging.hpp>
#include <fstl/unique.hpp>
#include <tiny_gltf.h>

using namespace vkr;

GltfModel::Material::Material(
    const GltfModel &model, int albedoTextureIndex, glm::vec4 baseColorFactor)
    : albedoTextureIndex(albedoTextureIndex) {
  auto [descriptorPool, descriptorSetLayout] =
      Context::getDescriptorManager()[DESC_MATERIAL];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  this->ubo.baseColorFactor = baseColorFactor;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->descriptorSets[i] =
        descriptorPool->allocateDescriptorSets({*descriptorSetLayout})[0];

    this->uniformBuffers[i] = Buffer{
        sizeof(MaterialUniform),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vkr::MemoryUsageFlagBits::eCpuToGpu,
        vkr::MemoryPropertyFlagBits::eHostVisible |
            vkr::MemoryPropertyFlagBits::eHostCoherent,
    };

    // CombinedImageSampler
    auto &texture = model.textures[this->albedoTextureIndex];
    auto albedoDescriptorInfo = texture.getDescriptorInfo();

    // UniformBuffer
    this->uniformBuffers[i].mapMemory(&this->mappings[i]);
    memcpy(this->mappings[i], &this->ubo, sizeof(MaterialUniform));
    auto bufferInfo = vk::DescriptorBufferInfo{
        this->uniformBuffers[i].getHandle(), 0, sizeof(MaterialUniform)};

    Context::getDevice().updateDescriptorSets(
        {
            vk::WriteDescriptorSet{
                this->descriptorSets[i],                    // dstSet
                0,                                          // dstBinding
                0,                                          // dstArrayElement
                1,                                          // descriptorCount
                vkr::DescriptorType::eCombinedImageSampler, // descriptorType
                &albedoDescriptorInfo,                      // pImageInfo
                nullptr,                                    // pBufferInfo
                nullptr,                                    // pTexelBufferView
            },
            vk::WriteDescriptorSet{
                this->descriptorSets[i],             // dstSet
                1,                                   // dstBinding
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

GltfModel::Mesh::Mesh(const glm::mat4 &matrix) {
  this->ubo.model = matrix;

  auto [descriptorPool, descriptorSetLayout] =
      Context::getDescriptorManager()[DESC_MESH];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->uniformBuffers[i] = Buffer{
        sizeof(MeshUniform),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vkr::MemoryUsageFlagBits::eCpuToGpu,
        vkr::MemoryPropertyFlagBits::eHostVisible |
            vkr::MemoryPropertyFlagBits::eHostCoherent,
    };

    this->uniformBuffers[i].mapMemory(&this->mappings[i]);

    memcpy(this->mappings[i], &this->ubo, sizeof(MeshUniform));

    auto bufferInfo = vk::DescriptorBufferInfo{
        this->uniformBuffers[i].getHandle(), 0, sizeof(MeshUniform)};

    this->descriptorSets[i] =
        descriptorPool->allocateDescriptorSets(1, *descriptorSetLayout)[0];

    vkr::Context::getDevice().updateDescriptorSets(
        {vk::WriteDescriptorSet{
            this->descriptorSets[i],             // dstSet
            0,                                   // dstBinding
            0,                                   // dstArrayElement
            1,                                   // descriptorCount
            vkr::DescriptorType::eUniformBuffer, // descriptorType
            nullptr,                             // pImageInfo
            &bufferInfo,                         // pBufferInfo
            nullptr,                             // pTexelBufferView
        }},
        {});
  }
}

void GltfModel::Mesh::updateUniform(int frameIndex) {
  memcpy(this->mappings[frameIndex], &this->ubo, sizeof(MeshUniform));
}

glm::mat4 GltfModel::Node::localMatrix() {
  return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) *
         glm::scale(glm::mat4(1.0f), scale) * matrix;
}

glm::mat4 GltfModel::Node::getMatrix(GltfModel &model) {
  glm::mat4 m = localMatrix();
  int p = this->parentIndex;
  while (p != -1) {
    m = model.nodes[this->parentIndex].localMatrix() * m;
    p = model.nodes[this->parentIndex].parentIndex;
  }

  auto translation = glm::translate(glm::mat4(1.0f), model.pos);
  auto rotation = glm::rotate(
      glm::mat4(1.0f), glm::radians(model.rotation.x), {1.0, 0.0, 0.0});
  rotation =
      glm::rotate(rotation, glm::radians(model.rotation.y), {0.0, 1.0, 0.0});
  rotation =
      glm::rotate(rotation, glm::radians(model.rotation.z), {0.0, 0.0, 1.0});
  auto scaling = glm::scale(glm::mat4(1.0f), model.scale);

  auto modelMatrix = translation * rotation * scaling;

  m = modelMatrix * m;

  return m;
}

void GltfModel::Node::update(GltfModel &model, int frameIndex) {
  if (this->meshIndex != -1) {
    glm::mat4 m = this->getMatrix(model);
    auto &mesh = model.meshes[meshIndex];
    mesh.ubo.model = m;
    mesh.updateUniform(frameIndex);
  }

  for (auto &childIndex : childrenIndices) {
    model.nodes[childIndex].update(model, frameIndex);
  }
}

GltfModel::GltfModel(Window &window, const std::string &path, bool flipUVs) {
  fstl::log::debug("Loading glTF model: {}", path);

  tinygltf::TinyGLTF loader;
  tinygltf::Model model;

  std::string err, warn;

  // TODO: check file extension and load accordingly
  std::string ext = path.substr(path.find_last_of('.'), path.size());
  bool ret;
  if (std::strncmp(ext.c_str(), ".glb", ext.length()) == 0) {
    ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
  } else {
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
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
  std::vector<Vertex> vertices;

  tinygltf::Scene &scene = model.scenes[model.defaultScene];

  this->nodes.resize(model.nodes.size());
  this->meshes.resize(model.meshes.size());

  for (size_t i = 0; i < scene.nodes.size(); i++) {
    this->loadNode(-1, scene.nodes[i], model, indices, vertices, flipUVs);
  }

  for (auto &node : nodes) {
    if (node.meshIndex != -1 && meshes[node.meshIndex].uniformBuffers[0]) {
      node.update(*this, window.getCurrentFrameIndex());
    }
  }

  size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
  // TODO: index buffer size could be larger than it needs to be
  // due to other index types
  size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  assert((vertexBufferSize > 0) && (indexBufferSize > 0));

  fstl::unique<StagingBuffer> vertexStagingBuffer{{vertexBufferSize}};
  fstl::unique<StagingBuffer> indexStagingBuffer{{indexBufferSize}};

  this->vertexBuffer = Buffer{
      vertexBufferSize,
      vkr::BufferUsageFlagBits::eVertexBuffer |
          vkr::BufferUsageFlagBits::eTransferDst,
      vkr::MemoryUsageFlagBits::eGpuOnly,
      vkr::MemoryPropertyFlagBits::eDeviceLocal,
  };

  this->indexBuffer = Buffer{
      indexBufferSize,
      vkr::BufferUsageFlagBits::eIndexBuffer |
          vkr::BufferUsageFlagBits::eTransferDst,
      vkr::MemoryUsageFlagBits::eGpuOnly,
      vkr::MemoryPropertyFlagBits::eDeviceLocal,
  };

  vertexStagingBuffer->copyMemory(vertices.data(), vertexBufferSize);
  vertexStagingBuffer->transfer(this->vertexBuffer, vertexBufferSize);

  indexStagingBuffer->copyMemory(indices.data(), indexBufferSize);
  indexStagingBuffer->transfer(this->indexBuffer, indexBufferSize);
}

GltfModel::~GltfModel() {}

void GltfModel::draw(Window &window, GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  commandBuffer.bindGraphicsPipeline(pipeline);
  commandBuffer.bindVertexBuffers(vertexBuffer);
  commandBuffer.bindIndexBuffer(indexBuffer, 0, vkr::IndexType::eUint32);

  for (auto &node : nodes) {
    if (node.meshIndex != -1 && meshes[node.meshIndex].uniformBuffers[0]) {
      node.update(*this, window.getCurrentFrameIndex());
    }
  }

  for (auto &node : nodes) {
    drawNode(node, window, pipeline);
  }
}

void GltfModel::setPosition(glm::vec3 pos) { this->pos = pos; }

glm::vec3 GltfModel::getPosition() const { return this->pos; }

void GltfModel::setRotation(glm::vec3 rotation) { this->rotation = rotation; }

glm::vec3 GltfModel::getRotation() const { return this->rotation; }

void GltfModel::setScale(glm::vec3 scale) { this->scale = scale; }

glm::vec3 GltfModel::getScale() const { return this->scale; }

void GltfModel::destroy() {
  vertexBuffer.destroy();
  indexBuffer.destroy();

  for (auto &mesh : meshes) {
    for (auto &uniformBuffer : mesh.uniformBuffers) {
      if (uniformBuffer)
        uniformBuffer.destroy();
    }

    auto descriptorPool = Context::getDescriptorManager().getPool(DESC_MESH);
    assert(descriptorPool != nullptr);

    for (auto &descriptorSet : mesh.descriptorSets) {
      if (descriptorSet)
        Context::getDevice().freeDescriptorSets(*descriptorPool, descriptorSet);
    }
  }

  for (auto &material : materials) {
    for (auto &uniformBuffer : material.uniformBuffers) {
      if (uniformBuffer)
        uniformBuffer.destroy();
    }

    auto descriptorPool =
        Context::getDescriptorManager().getPool(DESC_MATERIAL);

    assert(descriptorPool != nullptr);

    for (auto &descriptorSet : material.descriptorSets) {
      if (descriptorSet)
        Context::getDevice().freeDescriptorSets(*descriptorPool, descriptorSet);
    }
  }

  for (auto &texture : textures) {
    if (texture)
      texture.destroy();
  }
}

void GltfModel::loadMaterials(tinygltf::Model &model) {
  this->materials.resize(model.materials.size());
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

    this->materials[i] = Material{*this, albedoTextureIndex, baseColorFactor};
  }
}

void GltfModel::loadTextures(tinygltf::Model &model) {
  this->textures.resize(model.images.size());
  for (size_t i = 0; i < model.images.size(); i++) {
    if (model.images[i].component != 4) {
      // TODO: support RGB images
      throw std::runtime_error("Only 4-component images are supported.");
    }

    this->textures[i] = {model.images[i].image,
                         static_cast<uint32_t>(model.images[i].width),
                         static_cast<uint32_t>(model.images[i].height)};
  }
}

void GltfModel::loadNode(
    int parentIndex,
    int nodeIndex,
    const tinygltf::Model &model,
    std::vector<uint32_t> &indices,
    std::vector<Vertex> &vertices,
    bool flipUVs) {
  const tinygltf::Node &node = model.nodes[nodeIndex];
  Node newNode;
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
    Mesh newMesh(newNode.matrix);

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
          Vertex vert{};
          // TODO: fix rendundancies and see if it works?
          vert.pos = glm::make_vec3(&bufferPos[v * 3]);
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

    this->meshes[node.mesh] = newMesh;

    newNode.meshIndex = node.mesh;
  }

  if (parentIndex != -1) {
    nodes[parentIndex].childrenIndices.push_back(nodeIndex);
  }

  nodes[nodeIndex] = newNode;
}

void GltfModel::getNodeDimensions(Node &node, glm::vec3 &min, glm::vec3 &max) {
  if (node.meshIndex != -1) {
    for (Primitive &primitive : this->meshes[node.meshIndex].primitives) {
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
    getNodeDimensions(this->nodes[childIndex], min, max);
  }
}

void GltfModel::getSceneDimensions() {
  dimensions.min = glm::vec3(FLT_MAX);
  dimensions.max = glm::vec3(-FLT_MAX);
  for (auto node : nodes) {
    getNodeDimensions(node, dimensions.min, dimensions.max);
  }
  dimensions.size = dimensions.max - dimensions.min;
  dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
  dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
}

void GltfModel::drawNode(
    Node &node, Window &window, GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  if (node.meshIndex != -1) {
    commandBuffer.bindDescriptorSets(
        vkr::PipelineBindPoint::eGraphics,
        pipeline.getLayout(),
        2, // firstSet
        this->meshes[node.meshIndex].descriptorSets[i],
        {});
    for (Primitive &primitive : this->meshes[node.meshIndex].primitives) {
      if (primitive.materialIndex != -1 && this->materials.size() > 0) {
        commandBuffer.bindDescriptorSets(
            vkr::PipelineBindPoint::eGraphics,
            pipeline.getLayout(),
            1, // firstSet
            this->materials[primitive.materialIndex].descriptorSets[i],
            {});
      }

      commandBuffer.drawIndexed(
          primitive.indexCount, 1, primitive.firstIndex, 0, 0);
    }
  }

  for (auto &childIndex : node.childrenIndices) {
    drawNode(this->nodes[childIndex], window, pipeline);
  }
}
