#pragma once

#include "pipeline.hpp"

namespace vkr {
class DescriptorManager {
  friend class Context;

public:
  DescriptorSetLayout &getCameraSetLayout();
  DescriptorPool &getCameraPool();

  DescriptorSetLayout &getMaterialSetLayout();
  DescriptorPool &getMaterialPool();

  DescriptorSetLayout &getModelSetLayout();
  DescriptorPool &getModelPool();

protected:
  void init();
  void destroy();

  DescriptorSetLayout cameraSetLayout;
  DescriptorPool cameraPool;

  DescriptorSetLayout materialSetLayout;
  DescriptorPool materialPool;

  DescriptorSetLayout modelSetLayout;
  DescriptorPool modelPool;
};
} // namespace vkr
