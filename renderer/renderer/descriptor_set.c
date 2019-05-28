#include "descriptor_set.h"

#include "context.h"
#include <fstd_util.h>
#include <string.h>

// TODO: hashing
// TODO: create descriptor pools for the allocator
// TODO: make this usable from multiple threads

static void node_init(
    re_descriptor_set_allocator_t *allocator,
    re_descriptor_set_allocator_node_t *node) {
  node->next = NULL;

  // Allocate descriptor sets
  VkDescriptorSetLayout set_layouts[ARRAY_SIZE(node->descriptor_sets)];
  for (size_t i = 0; i < ARRAY_SIZE(node->descriptor_sets); i++) {
    set_layouts[i] = allocator->set_layout;
  }

  VK_CHECK(vkAllocateDescriptorSets(
      g_ctx.device,
      &(VkDescriptorSetAllocateInfo){
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = g_ctx.descriptor_pool,
          .descriptorSetCount = ARRAY_SIZE(node->descriptor_sets),
          .pSetLayouts = set_layouts,
      },
      node->descriptor_sets));

  memset(&node->data, 0, sizeof(node->data));
}

void re_descriptor_set_allocator_init(
    re_descriptor_set_allocator_t *allocator,
    re_descriptor_set_layout_t layout) {
  allocator->current_frame = 0;
  allocator->layout = layout;

  // Set up the bindings and entries
  VkDescriptorSetLayoutBinding bindings[RE_MAX_DESCRIPTOR_SET_BINDINGS] = {0};
  VkDescriptorUpdateTemplateEntry entries[RE_MAX_DESCRIPTOR_SET_BINDINGS] = {0};

  allocator->binding_count = 0;
  for (uint32_t i = 0; i < RE_MAX_DESCRIPTOR_SET_BINDINGS; i++) {
    bindings[i].binding = i;
    bindings[i].stageFlags = allocator->layout.stage_flags[i];
    bindings[i].descriptorCount = (uint32_t)layout.array_size[i];

    entries[i].dstBinding = i;
    entries[i].dstArrayElement = 0;
    entries[i].descriptorCount = bindings[i].descriptorCount;
    entries[i].offset = sizeof(re_descriptor_info_t) * i;
    entries[i].stride = sizeof(re_descriptor_info_t);

    if (layout.uniform_buffer_mask & (1u << i)) {
      assert(bindings[i].descriptorCount > 0);
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      entries[i].descriptorType = bindings[i].descriptorType;
      allocator->binding_count++;
      continue;
    }

    if (layout.combined_image_sampler_mask & (1u << i)) {
      assert(bindings[i].descriptorCount > 0);
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      entries[i].descriptorType = bindings[i].descriptorType;
      allocator->binding_count++;
      continue;
    }
  }

  // Create set layout
  VkDescriptorSetLayoutCreateInfo set_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = allocator->binding_count,
      .pBindings = bindings,
  };

  VK_CHECK(vkCreateDescriptorSetLayout(
      g_ctx.device, &set_layout_create_info, NULL, &allocator->set_layout));

  // Create update template
  VK_CHECK(vkCreateDescriptorUpdateTemplate(
      g_ctx.device,
      &(VkDescriptorUpdateTemplateCreateInfo){
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
          .descriptorUpdateEntryCount = allocator->binding_count,
          .pDescriptorUpdateEntries = entries,
          .templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET,
          .descriptorSetLayout = allocator->set_layout,
      },
      NULL,
      &allocator->update_template));

  // Create nodes
  node_init(allocator, &allocator->base_node);
}

void re_descriptor_set_allocator_begin_frame(
    re_descriptor_set_allocator_t *allocator) {
  allocator->current_frame =
      (allocator->current_frame + 1) % RE_MAX_FRAMES_IN_FLIGHT;

  re_descriptor_set_allocator_node_t *node = &allocator->base_node;

  while (node != NULL) {
    for (uint32_t i = 0; i < RE_DESCRIPTOR_RING_SIZE; i++) {
      if (node->data[i].in_use &&
          node->data[i].frame == allocator->current_frame) {
        node->data[i].in_use = false;
      }
    }

    node = node->next;
  }
}

VkDescriptorSet re_descriptor_set_allocator_alloc(
    re_descriptor_set_allocator_t *allocator,
    re_descriptor_info_t *descriptors) {
  re_descriptor_set_allocator_node_t *node = &allocator->base_node;

  while (1) {
    for (uint32_t i = 0; i < RE_DESCRIPTOR_RING_SIZE; i++) {
      if (node->data[i].in_use) {
        continue;
      }

      if (memcmp(
              descriptors,
              &node->data[i].descriptor_infos,
              sizeof(re_descriptor_info_t) * allocator->binding_count) == 0) {
        // If we DO *NOT* NEED to update the descriptor (hashes are equal)

        node->data[i].in_use = true;
        node->data[i].frame = allocator->current_frame;
        return node->descriptor_sets[i];
      }
    }

    for (uint32_t i = 0; i < RE_DESCRIPTOR_RING_SIZE; i++) {
      if (node->data[i].in_use) {
        continue;
      }

      if (memcmp(
              descriptors,
              &node->data[i].descriptor_infos,
              sizeof(re_descriptor_info_t) * allocator->binding_count) != 0) {
        // If we DO NEED to update the descriptor (in the case of a hash
        // mismatch)

        node->data[i].in_use = true;
        node->data[i].frame = allocator->current_frame;

        vkUpdateDescriptorSetWithTemplate(
            g_ctx.device,
            node->descriptor_sets[i],
            allocator->update_template,
            descriptors);

        return node->descriptor_sets[i];
      }
    }

    if (node->next == NULL) {
      // TODO: replace this malloc with something faster
      node->next = calloc(sizeof(re_descriptor_set_allocator_node_t), 1);
      node_init(allocator, node->next);
    }

    node = node->next;
  }

  return VK_NULL_HANDLE;
}

void re_descriptor_set_allocator_destroy(
    re_descriptor_set_allocator_t *allocator) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkDestroyDescriptorUpdateTemplate(
      g_ctx.device, allocator->update_template, NULL);

  vkDestroyDescriptorSetLayout(g_ctx.device, allocator->set_layout, NULL);

  re_descriptor_set_allocator_node_t *node = &allocator->base_node;

  while (node != NULL) {
    vkFreeDescriptorSets(
        g_ctx.device,
        g_ctx.descriptor_pool,
        ARRAY_SIZE(node->descriptor_sets),
        node->descriptor_sets);

    node = node->next;
  }

  node = allocator->base_node.next;

  while (node != NULL) {
    re_descriptor_set_allocator_node_t *old_node = node;
    node = old_node->next;
    free(old_node);
  }
}
