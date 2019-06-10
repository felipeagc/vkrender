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
  memset(node, 0, sizeof(re_descriptor_set_allocator_node_t));
  node->next = NULL;

  // Allocate descriptor sets
  VkDescriptorSetLayout set_layouts[ARRAY_SIZE(node->descriptor_sets)];
  for (size_t i = 0; i < ARRAY_SIZE(node->descriptor_sets); i++) {
    set_layouts[i] = allocator->set_layout;
  }

  VK_CHECK(vkAllocateDescriptorSets(
      g_ctx.device,
      &(VkDescriptorSetAllocateInfo){
          .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool     = g_ctx.descriptor_pool,
          .descriptorSetCount = ARRAY_SIZE(node->descriptor_sets),
          .pSetLayouts        = set_layouts,
      },
      node->descriptor_sets));
}

void re_descriptor_set_allocator_init(
    re_descriptor_set_allocator_t *allocator,
    re_descriptor_set_layout_t layout) {
  memset(allocator, 0, sizeof(*allocator));

  allocator->current_frame = 0;
  allocator->layout        = layout;

  // Set up the bindings and entries
  VkDescriptorSetLayoutBinding bindings[RE_MAX_DESCRIPTOR_SET_BINDINGS]   = {0};
  VkDescriptorUpdateTemplateEntry entries[RE_MAX_DESCRIPTOR_SET_BINDINGS] = {0};

  allocator->binding_count = 0;
  for (uint32_t i = 0; i < RE_MAX_DESCRIPTOR_SET_BINDINGS; i++) {
    bindings[i].binding         = i;
    bindings[i].stageFlags      = allocator->layout.stage_flags[i];
    bindings[i].descriptorCount = (uint32_t)layout.array_size[i];

    entries[i].dstBinding      = i;
    entries[i].dstArrayElement = 0;
    entries[i].descriptorCount = bindings[i].descriptorCount;
    entries[i].offset          = sizeof(re_descriptor_info_t) * i;
    entries[i].stride          = sizeof(re_descriptor_info_t);

    if (layout.uniform_buffer_mask & (1u << i)) {
      assert(bindings[i].descriptorCount > 0);
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      entries[i].descriptorType  = bindings[i].descriptorType;
      allocator->binding_count++;
      continue;
    }

    if (layout.uniform_buffer_dynamic_mask & (1u << i)) {
      assert(bindings[i].descriptorCount > 0);
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      entries[i].descriptorType  = bindings[i].descriptorType;
      allocator->binding_count++;
      continue;
    }

    if (layout.combined_image_sampler_mask & (1u << i)) {
      assert(bindings[i].descriptorCount > 0);
      bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      entries[i].descriptorType  = bindings[i].descriptorType;
      allocator->binding_count++;
      continue;
    }
  }

  // Create set layout
  VkDescriptorSetLayoutCreateInfo set_layout_create_info = {
      .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = allocator->binding_count,
      .pBindings    = bindings,
  };

  VK_CHECK(vkCreateDescriptorSetLayout(
      g_ctx.device, &set_layout_create_info, NULL, &allocator->set_layout));

  // Create update template
  VK_CHECK(vkCreateDescriptorUpdateTemplate(
      g_ctx.device,
      &(VkDescriptorUpdateTemplateCreateInfo){
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
          .descriptorUpdateEntryCount = allocator->binding_count,
          .pDescriptorUpdateEntries   = entries,
          .templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET,
          .descriptorSetLayout = allocator->set_layout,
      },
      NULL,
      &allocator->update_template));

  // Create nodes
  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    node_init(allocator, &allocator->base_nodes[i]);
  }
}

void re_descriptor_set_allocator_begin_frame(
    re_descriptor_set_allocator_t *allocator) {
  allocator->current_frame =
      (allocator->current_frame + 1) % RE_MAX_FRAMES_IN_FLIGHT;

  allocator->writes[allocator->current_frame]         = 0;
  allocator->matches[allocator->current_frame]        = 0;
  allocator->max_iterations[allocator->current_frame] = 0;

  allocator->last_nodes[allocator->current_frame] =
      &allocator->base_nodes[allocator->current_frame];
  allocator->last_sets[allocator->current_frame] = 0;

  re_descriptor_set_allocator_node_t *node =
      &allocator->base_nodes[allocator->current_frame];

  while (node != NULL) {
    for (uint32_t i = 0; i < RE_DESCRIPTOR_RING_SIZE; i++) {
      node->data[i].in_use = false;
    }
    node = node->next;
  }
}

VkDescriptorSet re_descriptor_set_allocator_alloc(
    re_descriptor_set_allocator_t *allocator,
    re_descriptor_info_t *descriptors) {
  uint32_t first_set = allocator->last_sets[allocator->current_frame];

  uint32_t iters = 0;
  re_descriptor_set_allocator_node_t *last_node =
      allocator->last_nodes[allocator->current_frame];

  for (re_descriptor_set_allocator_node_t *node = last_node; node != NULL;
       node                                     = node->next) {
    for (uint32_t i = first_set; i < RE_DESCRIPTOR_RING_SIZE; i++) {
      iters++;

      // Find a matching descriptor set
      bool same_desc =
          memcmp(
              descriptors,
              node->data[i].descriptor_infos,
              sizeof(re_descriptor_info_t) * allocator->binding_count) == 0;

      if (same_desc) {
        allocator->matches[allocator->current_frame]++;
        node->data[i].in_use = true;

        allocator->last_sets[allocator->current_frame]  = i;
        allocator->last_nodes[allocator->current_frame] = node;

        allocator->max_iterations[allocator->current_frame] =
            MAX(allocator->max_iterations[allocator->current_frame], iters);

        assert(node->descriptor_sets[i] != VK_NULL_HANDLE);
        return node->descriptor_sets[i];
      }
    }

    first_set = 0;
  }

  re_descriptor_set_allocator_node_t *node =
      allocator->last_nodes[allocator->current_frame];

  while (node != NULL) {
    for (uint32_t i = first_set; i < RE_DESCRIPTOR_RING_SIZE; i++) {
      iters++;
      // Find a non-matching descriptor set that is not in use

      if (!node->data[i].in_use) {
        node->data[i].in_use = true;
        memcpy(
            node->data[i].descriptor_infos,
            descriptors,
            sizeof(re_descriptor_info_t) * allocator->binding_count);

        allocator->writes[allocator->current_frame]++;

        vkUpdateDescriptorSetWithTemplate(
            g_ctx.device,
            node->descriptor_sets[i],
            allocator->update_template,
            descriptors);

        allocator->last_sets[allocator->current_frame]  = i;
        allocator->last_nodes[allocator->current_frame] = node;

        allocator->max_iterations[allocator->current_frame] =
            MAX(allocator->max_iterations[allocator->current_frame], iters);

        assert(node->descriptor_sets[i] != VK_NULL_HANDLE);
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

  assert(0);
  return VK_NULL_HANDLE;
}

void re_descriptor_set_allocator_destroy(
    re_descriptor_set_allocator_t *allocator) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkDestroyDescriptorUpdateTemplate(
      g_ctx.device, allocator->update_template, NULL);

  vkDestroyDescriptorSetLayout(g_ctx.device, allocator->set_layout, NULL);

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_descriptor_set_allocator_node_t *node = &allocator->base_nodes[i];

    while (node != NULL) {
      vkFreeDescriptorSets(
          g_ctx.device,
          g_ctx.descriptor_pool,
          ARRAY_SIZE(node->descriptor_sets),
          node->descriptor_sets);

      node = node->next;
    }

    node = allocator->base_nodes[i].next;

    while (node != NULL) {
      re_descriptor_set_allocator_node_t *old_node = node;
      node                                         = old_node->next;
      free(old_node);
    }
  }
}
