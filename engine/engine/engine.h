#pragma once

#include <vulkan/vulkan.h>

typedef struct eg_engine_t {
  struct {
    VkDescriptorSetLayout camera;
    VkDescriptorSetLayout model;
    VkDescriptorSetLayout environment;
    VkDescriptorSetLayout material;
  } set_layouts;
} eg_engine_t;

extern eg_engine_t g_eng;

void eg_engine_init(const char *argv0);

void eg_engine_destroy();
