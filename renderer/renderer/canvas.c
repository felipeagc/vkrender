#include "canvas.h"

#include "context.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>
#include <string.h>

static inline void create_resources(re_canvas_t *canvas) {
  // Create renderpass
  {
    VkAttachmentDescription attachment_descriptions[] = {
        // Multisampled color attachment
        (VkAttachmentDescription){
            0,                                        // flags
            canvas->color_format,                     // format
            canvas->render_target.sample_count,       // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
            VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // finalLayout
        },

        // Multisampled depth attachment
        (VkAttachmentDescription){
            0,                                                // flags
            canvas->depth_format,                             // format
            canvas->render_target.sample_count,               // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,                  // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED,                        // initialLayout
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // finalLayout
        },

        // Resolved color attachment
        (VkAttachmentDescription){
            0,                                        // flags
            canvas->color_format,                     // format
            VK_SAMPLE_COUNT_1_BIT,                    // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
            VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // finalLayout
        },
    };

    VkAttachmentReference color_attachment_reference = {
        0,                                        // attachment
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // layout
    };

    VkAttachmentReference depth_attachment_reference = {
        1,                                                // attachment
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // layout
    };

    VkAttachmentReference resolve_color_attachment_reference = {
        2,                                        // attachment
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // layout
    };

    VkSubpassDescription subpass_description = {
        .flags                   = 0,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount    = 0,
        .pInputAttachments       = NULL,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &color_attachment_reference,
        .pResolveAttachments     = &resolve_color_attachment_reference,
        .pDepthStencilAttachment = &depth_attachment_reference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments    = NULL,
    };

    if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
      subpass_description.pResolveAttachments = NULL;
    }

    VkSubpassDependency dependencies[] = {
        (VkSubpassDependency){
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        (VkSubpassDependency){
            .srcSubpass    = 0,
            .dstSubpass    = VK_SUBPASS_EXTERNAL,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = (uint32_t)ARRAY_SIZE(attachment_descriptions),
        .pAttachments    = attachment_descriptions,
        .subpassCount    = 1,
        .pSubpasses      = &subpass_description,
        .dependencyCount = (uint32_t)ARRAY_SIZE(dependencies),
        .pDependencies   = dependencies,
    };

    if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
      render_pass_create_info.attachmentCount -= 1;
    }

    VK_CHECK(vkCreateRenderPass(
        g_ctx.device,
        &render_pass_create_info,
        NULL,
        &canvas->render_target.render_pass));
  }

  // Create images and framebuffers
  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_image_init(
        &canvas->resources[i].color,
        &(re_image_options_t){
            .flags = RE_IMAGE_FLAG_DEDICATED,
            .usage = RE_IMAGE_USAGE_COLOR_ATTACHMENT |
                     RE_IMAGE_USAGE_TRANSFER_SRC | RE_IMAGE_USAGE_SAMPLED,
            .format          = canvas->color_format,
            .sample_count    = canvas->render_target.sample_count,
            .width           = canvas->render_target.width,
            .height          = canvas->render_target.height,
            .layer_count     = 1,
            .mip_level_count = 1,
        });

    re_image_init(
        &canvas->resources[i].color_resolve,
        &(re_image_options_t){
            .flags = RE_IMAGE_FLAG_DEDICATED,
            .usage = RE_IMAGE_USAGE_COLOR_ATTACHMENT |
                     RE_IMAGE_USAGE_TRANSFER_SRC | RE_IMAGE_USAGE_SAMPLED,
            .format          = canvas->color_format,
            .sample_count    = VK_SAMPLE_COUNT_1_BIT,
            .width           = canvas->render_target.width,
            .height          = canvas->render_target.height,
            .layer_count     = 1,
            .mip_level_count = 1,
        });

    re_image_init(
        &canvas->resources[i].depth,
        &(re_image_options_t){
            .flags = RE_IMAGE_FLAG_DEDICATED,
            .usage = RE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT |
                     RE_IMAGE_USAGE_TRANSFER_SRC,
            .format          = canvas->depth_format,
            .sample_count    = canvas->render_target.sample_count,
            .width           = canvas->render_target.width,
            .height          = canvas->render_target.height,
            .layer_count     = 1,
            .mip_level_count = 1,
            .aspect          = RE_IMAGE_ASPECT_DEPTH,
        });

    VkImageView attachments[] = {
        canvas->resources[i].color.image_view,
        canvas->resources[i].depth.image_view,
        canvas->resources[i].color_resolve.image_view,
    };

    VkFramebufferCreateInfo create_info = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = canvas->render_target.render_pass,
        .attachmentCount = (uint32_t)ARRAY_SIZE(attachments),
        .pAttachments    = attachments,
        .width           = canvas->render_target.width,
        .height          = canvas->render_target.height,
        .layers          = 1,
    };

    if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
      create_info.attachmentCount -= 1;
    }

    VK_CHECK(vkCreateFramebuffer(
        g_ctx.device, &create_info, NULL, &canvas->resources[i].framebuffer));
  }
}

static inline void destroy_resources(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (canvas->render_target.render_pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(g_ctx.device, canvas->render_target.render_pass, NULL);
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    if (canvas->resources[i].framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(
          g_ctx.device, canvas->resources[i].framebuffer, NULL);
    }

    re_image_destroy(&canvas->resources[i].color);
    re_image_destroy(&canvas->resources[i].color_resolve);
    re_image_destroy(&canvas->resources[i].depth);
  }
}

void re_canvas_init(re_canvas_t *canvas, re_canvas_options_t *options) {
  memset(canvas, 0, sizeof(*canvas));

  assert(options->width > 0);
  assert(options->height > 0);
  canvas->render_target.width  = options->width;
  canvas->render_target.height = options->height;

  if (options->sample_count == 0) {
    options->sample_count = VK_SAMPLE_COUNT_1_BIT;
  }
  canvas->render_target.sample_count = options->sample_count;

  if (options->color_format == 0) {
    options->color_format = VK_FORMAT_R8G8B8A8_UNORM;
  }
  canvas->color_format = options->color_format;

  canvas->clear_color = options->clear_color;

  bool res = re_ctx_get_supported_depth_format(&canvas->depth_format);
  assert(res);

  create_resources(canvas);
}

void re_canvas_begin(re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer) {
  canvas->current_frame = (canvas->current_frame + 1) % RE_MAX_FRAMES_IN_FLIGHT;

  // @TODO: make this customizable
  VkClearValue clear_values[] = {
      {.color = canvas->clear_color},
      {.depthStencil = {1.0f, 0}},
      {.color = canvas->clear_color},
  };

  VkRenderPassBeginInfo render_pass_begin_info = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass      = canvas->render_target.render_pass,
      .framebuffer     = canvas->resources[canvas->current_frame].framebuffer,
      .renderArea      = {{0, 0},
                     {canvas->render_target.width,
                      canvas->render_target.height}},
      .clearValueCount = ARRAY_SIZE(clear_values),
      .pClearValues    = clear_values,
  };

  if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
    render_pass_begin_info.clearValueCount -= 1;
  }

  re_cmd_begin_render_target(
      cmd_buffer,
      &canvas->render_target,
      &render_pass_begin_info,
      VK_SUBPASS_CONTENTS_INLINE);

  re_cmd_set_viewport(
      cmd_buffer,
      &(re_viewport_t){
          .x         = 0.0f,
          .y         = 0.0f,
          .width     = (float)canvas->render_target.width,
          .height    = (float)canvas->render_target.height,
          .min_depth = 0.0f,
          .max_depth = 1.0f,
      });

  re_cmd_set_scissor(
      cmd_buffer,
      &(re_rect_2d_t){
          .offset = {0, 0},
          .extent = {canvas->render_target.width, canvas->render_target.height},
      });
}

void re_canvas_end(re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer) {
  // Supress unused warning
  (void)canvas;

  vkCmdEndRenderPass(cmd_buffer->cmd_buffer);
}

void re_canvas_draw(
    re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline) {
  re_cmd_bind_pipeline(cmd_buffer, pipeline);

  VkDescriptorImageInfo descriptor = {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
    descriptor.imageView =
        canvas->resources[canvas->current_frame].color.image_view;
    descriptor.sampler = canvas->resources[canvas->current_frame].color.sampler;
  } else {
    descriptor.imageView =
        canvas->resources[canvas->current_frame].color_resolve.image_view;
    descriptor.sampler =
        canvas->resources[canvas->current_frame].color_resolve.sampler;
  }

  re_cmd_bind_descriptor(
      cmd_buffer, 0, 0, (re_descriptor_info_t){.image = descriptor});

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 0);

  re_cmd_draw(cmd_buffer, 3, 1, 0, 0);
}

void re_canvas_resize(
    re_canvas_t *canvas, const uint32_t width, const uint32_t height) {
  canvas->render_target.width  = width;
  canvas->render_target.height = height;

  destroy_resources(canvas);
  create_resources(canvas);
}

void re_canvas_destroy(re_canvas_t *canvas) {
  destroy_resources(canvas);
  memset(canvas, 0, sizeof(*canvas));
}
