#pragma once

#include "limits.h"
#include "vulkan.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RE_DESCRIPTOR_RING_SIZE 8

typedef VkDescriptorSet re_descriptor_set_t;

typedef union re_descriptor_info_t {
  VkDescriptorImageInfo image;
  VkDescriptorBufferInfo buffer;
} re_descriptor_info_t;

typedef struct re_descriptor_set_layout_t {
  // Bindings
  uint32_t sampler_mask;
  uint32_t combined_image_sampler_mask;
  uint32_t sampled_image_mask;
  uint32_t storage_image_mask;
  uint32_t uniform_texel_buffer_mask;
  uint32_t storage_texel_buffer_mask;
  uint32_t uniform_buffer_mask;
  uint32_t storage_buffer_mask;
  uint32_t uniform_buffer_dynamic_mask;
  uint32_t storage_buffer_dynamic_mask;
  uint32_t input_attachment_mask;

  // How many descriptors for each binding
  uint8_t array_size[RE_MAX_DESCRIPTOR_SET_BINDINGS];
  uint32_t stage_flags[RE_MAX_DESCRIPTOR_SET_BINDINGS];
} re_descriptor_set_layout_t;

typedef struct re_descriptor_set_data_t {
  bool in_use;
  re_descriptor_info_t descriptor_infos[RE_MAX_DESCRIPTOR_SET_BINDINGS];
} re_descriptor_set_data_t;

typedef struct re_descriptor_set_allocator_node_t {
  VkDescriptorSet descriptor_sets[RE_DESCRIPTOR_RING_SIZE];
  re_descriptor_set_data_t data[RE_DESCRIPTOR_RING_SIZE];

  struct re_descriptor_set_allocator_node_t *next;
} re_descriptor_set_allocator_node_t;

typedef struct re_descriptor_set_allocator_t {
  re_descriptor_set_layout_t layout;

  VkDescriptorSetLayout set_layout;
  VkDescriptorUpdateTemplate update_template;
  uint32_t binding_count;

  re_descriptor_set_allocator_node_t base_nodes[RE_MAX_FRAMES_IN_FLIGHT];

  uint32_t current_frame;

  uint32_t writes[RE_MAX_FRAMES_IN_FLIGHT];
  uint32_t matches[RE_MAX_FRAMES_IN_FLIGHT];

  re_descriptor_set_allocator_node_t *last_nodes[RE_MAX_FRAMES_IN_FLIGHT];
  uint32_t last_sets[RE_MAX_FRAMES_IN_FLIGHT];

  uint32_t max_iterations[RE_MAX_FRAMES_IN_FLIGHT];
} re_descriptor_set_allocator_t;

void re_descriptor_set_allocator_init(
    re_descriptor_set_allocator_t *allocator,
    re_descriptor_set_layout_t layout);

void re_descriptor_set_allocator_begin_frame(
    re_descriptor_set_allocator_t *allocator);

VkDescriptorSet re_descriptor_set_allocator_alloc(
    re_descriptor_set_allocator_t *allocator,
    re_descriptor_info_t *descriptors);

void re_descriptor_set_allocator_destroy(
    re_descriptor_set_allocator_t *allocator);
