#include "descriptor_manager.hpp"

using namespace vkr;

const uint32_t CAMERA_MAX_SETS = 20;
const uint32_t MESH_MAX_SETS = 500;
const uint32_t MATERIAL_MAX_SETS = 50;
const uint32_t LIGHTING_MAX_SETS = 50;

const fstl::fixed_vector<DescriptorSetLayoutBinding> CAMERA_BINDINGS = {{
    0,                                   // binding
    vkr::DescriptorType::eUniformBuffer, // descriptorType
    1,                                   // descriptorCount
    vkr::ShaderStageFlagBits::eVertex |
        vkr::ShaderStageFlagBits::eFragment, // stageFlags
    nullptr,                                 // pImmutableSamplers
}};

const fstl::fixed_vector<DescriptorSetLayoutBinding> MESH_BINDINGS = {{
    0,                                   // binding
    vkr::DescriptorType::eUniformBuffer, // descriptorType
    1,                                   // descriptorCount
    vkr::ShaderStageFlagBits::eVertex,   // stageFlags
    nullptr,                             // pImmutableSamplers
}};

const fstl::fixed_vector<DescriptorSetLayoutBinding> MATERIAL_BINDINGS = {
    {
        0,                                          // binding
        vkr::DescriptorType::eCombinedImageSampler, // descriptorType
        1,                                          // descriptorCount
        vkr::ShaderStageFlagBits::eFragment,        // stageFlags
        nullptr,                                    // pImmutableSamplers
    },
    {
        1,                                   // binding
        vkr::DescriptorType::eUniformBuffer, // descriptorType
        1,                                   // descriptorCount
        vkr::ShaderStageFlagBits::eFragment, // stageFlags
        nullptr,                             // pImmutableSamplers
    },
};

const fstl::fixed_vector<DescriptorSetLayoutBinding> LIGHTING_BINDINGS = {
    {
        0,                                   // binding
        vkr::DescriptorType::eUniformBuffer, // descriptorType
        1,                                   // descriptorCount
        vkr::ShaderStageFlagBits::eFragment, // stageFlags
        nullptr,                             // pImmutableSamplers
    },
};

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

fstl::fixed_vector<DescriptorSetLayout>
DescriptorManager::getDefaultSetLayouts() {
  return fstl::fixed_vector<DescriptorSetLayout>{
      *this->getSetLayout(vkr::DESC_CAMERA),
      *this->getSetLayout(vkr::DESC_MATERIAL),
      *this->getSetLayout(vkr::DESC_MESH),
      *this->getSetLayout(vkr::DESC_LIGHTING),
  };
}

void DescriptorManager::init() {
  this->addPool(DESC_CAMERA, {CAMERA_MAX_SETS, CAMERA_BINDINGS});
  this->addSetLayout(DESC_CAMERA, {CAMERA_BINDINGS});

  this->addPool(DESC_MATERIAL, {MATERIAL_MAX_SETS, MATERIAL_BINDINGS});
  this->addSetLayout(DESC_MATERIAL, {MATERIAL_BINDINGS});

  this->addPool(DESC_MESH, {MESH_MAX_SETS, MESH_BINDINGS});
  this->addSetLayout(DESC_MESH, {MESH_BINDINGS});

  this->addPool(DESC_LIGHTING, {LIGHTING_MAX_SETS, LIGHTING_BINDINGS});
  this->addSetLayout(DESC_LIGHTING, {LIGHTING_BINDINGS});
}

void DescriptorManager::destroy() {
  for (auto &p : this->pools) {
    p.second.destroy();
  }

  for (auto &p : this->setLayouts) {
    p.second.destroy();
  }
}
