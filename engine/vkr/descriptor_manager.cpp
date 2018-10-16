#include "descriptor_manager.hpp"

using namespace vkr;

const uint32_t CAMERA_MAX_SETS = 20;
const uint32_t MATERIAL_MAX_SETS = 50;
const uint32_t MESH_MAX_SETS = 500;

const SmallVec<DescriptorSetLayoutBinding> CAMERA_BINDINGS = {{
    0,                                   // binding
    vkr::DescriptorType::eUniformBuffer, // descriptorType
    1,                                   // descriptorCount
    vkr::ShaderStageFlagBits::eVertex,   // stageFlags
    nullptr,                             // pImmutableSamplers
}};

const SmallVec<DescriptorSetLayoutBinding> MESH_BINDINGS = {{
    0,                                   // binding
    vkr::DescriptorType::eUniformBuffer, // descriptorType
    1,                                   // descriptorCount
    vkr::ShaderStageFlagBits::eVertex,   // stageFlags
    nullptr,                             // pImmutableSamplers
}};

const SmallVec<DescriptorSetLayoutBinding> MATERIAL_BINDINGS = {{
    0,                                          // binding
    vkr::DescriptorType::eCombinedImageSampler, // descriptorType
    1,                                          // descriptorCount
    vkr::ShaderStageFlagBits::eFragment,        // stageFlags
    nullptr,                                    // pImmutableSamplers
}};

std::pair<DescriptorPool *, DescriptorSetLayout *> DescriptorManager::
operator[](const std::string &key) {
  return {this->getPool(key), this->getSetLayout(key)};
}

DescriptorPool *DescriptorManager::getPool(const std::string &key) {
  for (auto &p : this->pools) {
    if (p.first == key) {
      return &p.second;
    }
  }

  return nullptr;
}

DescriptorSetLayout *DescriptorManager::getSetLayout(const std::string &key) {
  for (auto &p : this->setLayouts) {
    if (p.first == key) {
      return &p.second;
    }
  }

  return nullptr;
}

bool DescriptorManager::addPool(const std::string &key, DescriptorPool pool) {
  if (this->getPool(key)) {
    return false;
  }

  pools.push_back({key, pool});
  return true;
}

bool DescriptorManager::addSetLayout(
    const std::string &key, DescriptorSetLayout setLayout) {
  if (this->getSetLayout(key)) {
    return false;
  }

  setLayouts.push_back({key, setLayout});
  return true;
}

void DescriptorManager::init() {
  this->addPool("camera", {CAMERA_MAX_SETS, CAMERA_BINDINGS});
  this->addSetLayout("camera", {CAMERA_BINDINGS});

  this->addPool("material", {MATERIAL_MAX_SETS, MATERIAL_BINDINGS});
  this->addSetLayout("material", {MATERIAL_BINDINGS});

  this->addPool("mesh", {MESH_MAX_SETS, MESH_BINDINGS});
  this->addSetLayout("mesh", {MESH_BINDINGS});
}

void DescriptorManager::destroy() {
  for (auto &p : this->pools) {
    p.second.destroy();
  }

  for (auto &p : this->setLayouts) {
    p.second.destroy();
  }
}
