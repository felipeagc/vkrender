#pragma once

#include <util/bitset.h>
#include <vulkan/vulkan.h>

const size_t RE_GLOBAL_MAX_DESCRIPTOR_SETS = 1000;

typedef struct re_resource_set_t {
  VkDescriptorSet descriptor_set;
  uint32_t allocation;
} re_resource_set_t;

typedef struct re_resource_set_layout_t {
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorSetLayoutBinding *bindings;
  uint32_t binding_count;

  // Preallocated descriptor sets
  VkDescriptorSet *descriptor_sets;
  uint32_t max_sets;
  UT_BITSET(RE_GLOBAL_MAX_DESCRIPTOR_SETS) bitset;
} re_resource_set_layout_t;

void re_resource_set_layout_init(
    re_resource_set_layout_t *layout,
    uint32_t max_sets,
    VkDescriptorSetLayoutBinding *bindings,
    uint32_t binding_count);

re_resource_set_t re_allocate_resource_set(re_resource_set_layout_t *layout);

void re_free_resource_set(
    re_resource_set_layout_t *layout, re_resource_set_t *resource_set);

void re_resource_set_layout_destroy(re_resource_set_layout_t *layout);

typedef struct re_resource_set_provider_t {
  VkPipelineLayout pipeline_layout;
  VkDescriptorPool descriptor_pool;
} re_resource_set_provider_t;

void re_resource_set_provider_init(
    re_resource_set_provider_t *provider,
    re_resource_set_layout_t *set_layouts,
    uint32_t set_layout_count);

void re_resource_set_provider_destroy(re_resource_set_provider_t *provider);

typedef struct re_resource_manager_t {
  struct {
    re_resource_set_layout_t camera;
    re_resource_set_layout_t model;
    re_resource_set_layout_t material;
    re_resource_set_layout_t environment;
    re_resource_set_layout_t single_texture;
  } set_layouts;

  struct {
    re_resource_set_provider_t standard;
    re_resource_set_provider_t billboard;
    re_resource_set_provider_t wireframe;
    re_resource_set_provider_t skybox;
    re_resource_set_provider_t fullscreen;
    re_resource_set_provider_t bake_cubemap;
    re_resource_set_provider_t heightmap;
  } providers;
} re_resource_manager_t;

void re_resource_manager_init(re_resource_manager_t *manager);

void re_resource_manager_destroy(re_resource_manager_t *manager);
