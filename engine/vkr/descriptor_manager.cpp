#include "descriptor_manager.hpp"

using namespace vkr;

const uint32_t CAMERA_MAX_SETS = 20;
const uint32_t MATERIAL_MAX_SETS = 50;
const uint32_t MODEL_MAX_SETS = 500;

const std::vector<DescriptorSetLayoutBinding> CAMERA_BINDINGS = {{
    0,                                   // binding
    vkr::DescriptorType::eUniformBuffer, // descriptorType
    1,                                   // descriptorCount
    vkr::ShaderStageFlagBits::eVertex,   // stageFlags
    nullptr,                             // pImmutableSamplers
}};

const std::vector<DescriptorSetLayoutBinding> MODEL_BINDINGS = {{
    0,                                   // binding
    vkr::DescriptorType::eUniformBuffer, // descriptorType
    1,                                   // descriptorCount
    vkr::ShaderStageFlagBits::eVertex,   // stageFlags
    nullptr,                             // pImmutableSamplers
}};

const std::vector<DescriptorSetLayoutBinding> MATERIAL_BINDINGS = {{
    0,                                          // binding
    vkr::DescriptorType::eCombinedImageSampler, // descriptorType
    1,                                          // descriptorCount
    vkr::ShaderStageFlagBits::eFragment,        // stageFlags
    nullptr,                                    // pImmutableSamplers
}};

DescriptorSetLayout &DescriptorManager::getCameraSetLayout() {
  return this->cameraSetLayout;
}

DescriptorPool &DescriptorManager::getCameraPool() { return this->cameraPool; }

DescriptorSetLayout &DescriptorManager::getMaterialSetLayout() {
  return this->materialSetLayout;
}

DescriptorPool &DescriptorManager::getMaterialPool() {
  return this->materialPool;
}

DescriptorSetLayout &DescriptorManager::getModelSetLayout() {
  return this->modelSetLayout;
}

DescriptorPool &DescriptorManager::getModelPool() { return this->modelPool; }

void DescriptorManager::init() {
  this->cameraSetLayout = {CAMERA_BINDINGS};
  this->cameraPool = {CAMERA_MAX_SETS, CAMERA_BINDINGS};
  this->materialSetLayout = {MATERIAL_BINDINGS};
  this->materialPool = {MATERIAL_MAX_SETS, MATERIAL_BINDINGS};
  this->modelSetLayout = {MODEL_BINDINGS};
  this->modelPool = {MODEL_MAX_SETS, MODEL_BINDINGS};
}

void DescriptorManager::destroy() {
  cameraPool.destroy();
  cameraSetLayout.destroy();

  materialPool.destroy();
  materialSetLayout.destroy();

  modelPool.destroy();
  modelSetLayout.destroy();
}
