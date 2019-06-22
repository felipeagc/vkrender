#include "render_target.h"

static void
hash_attachment(re_hasher_t *hasher, const VkAttachmentDescription *att) {
  re_hash_u32(hasher, att->flags);
  re_hash_u32(hasher, att->initialLayout);
  re_hash_u32(hasher, att->finalLayout);
  re_hash_u32(hasher, att->format);
  re_hash_u32(hasher, att->loadOp);
  re_hash_u32(hasher, att->storeOp);
  re_hash_u32(hasher, att->stencilLoadOp);
  re_hash_u32(hasher, att->stencilStoreOp);
  re_hash_u32(hasher, att->samples);
}

static void
hash_dependency(re_hasher_t *hasher, const VkSubpassDependency *dep) {
  re_hash_u32(hasher, dep->dependencyFlags);
  re_hash_u32(hasher, dep->dstAccessMask);
  re_hash_u32(hasher, dep->srcAccessMask);
  re_hash_u32(hasher, dep->srcSubpass);
  re_hash_u32(hasher, dep->dstSubpass);
  re_hash_u32(hasher, dep->srcStageMask);
  re_hash_u32(hasher, dep->dstStageMask);
}

static void
hash_subpass(re_hasher_t *hasher, const VkSubpassDescription *subpass) {
  re_hash_u32(hasher, subpass->flags);
  re_hash_u32(hasher, subpass->colorAttachmentCount);
  re_hash_u32(hasher, subpass->inputAttachmentCount);
  re_hash_u32(hasher, subpass->preserveAttachmentCount);
  re_hash_u32(hasher, subpass->pipelineBindPoint);

  for (uint32_t i = 0; i < subpass->preserveAttachmentCount; i++)
    re_hash_u32(hasher, subpass->pPreserveAttachments[i]);

  for (uint32_t i = 0; i < subpass->colorAttachmentCount; i++) {
    re_hash_u32(hasher, subpass->pColorAttachments[i].attachment);
    re_hash_u32(hasher, subpass->pColorAttachments[i].layout);
  }

  for (uint32_t i = 0; i < subpass->inputAttachmentCount; i++) {
    re_hash_u32(hasher, subpass->pInputAttachments[i].attachment);
    re_hash_u32(hasher, subpass->pInputAttachments[i].layout);
  }

  if (subpass->pResolveAttachments) {
    for (uint32_t i = 0; i < subpass->colorAttachmentCount; i++) {
      re_hash_u32(hasher, subpass->pResolveAttachments[i].attachment);
      re_hash_u32(hasher, subpass->pResolveAttachments[i].layout);
    }
  }

  if (subpass->pDepthStencilAttachment) {
    re_hash_u32(hasher, subpass->pDepthStencilAttachment->attachment);
    re_hash_u32(hasher, subpass->pDepthStencilAttachment->layout);
  } else {
    re_hash_u32(hasher, 0);
  }
}

re_hash_t re_hash_renderpass(VkRenderPassCreateInfo *create_info) {
  re_hasher_t hasher = re_hasher_create();

  re_hash_u32(&hasher, create_info->attachmentCount);
  re_hash_u32(&hasher, create_info->dependencyCount);
  re_hash_u32(&hasher, create_info->subpassCount);

  for (uint32_t i = 0; i < create_info->attachmentCount; i++) {
    hash_attachment(&hasher, &create_info->pAttachments[i]);
  }

  for (uint32_t i = 0; i < create_info->dependencyCount; i++) {
    hash_dependency(&hasher, &create_info->pDependencies[i]);
  }

  for (uint32_t i = 0; i < create_info->subpassCount; i++) {
    hash_subpass(&hasher, &create_info->pSubpasses[i]);
  }

  return re_hasher_get(&hasher);
}
