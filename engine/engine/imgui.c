#include "imgui.h"

#include "filesystem.h"
#include "pipelines.h"
#include "util.h"
#include <fstd_util.h>
#include <imgui_impl_glfw.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

static re_pipeline_t g_pipeline                              = {0};
static re_image_t g_atlas                                    = {0};
static re_buffer_t g_vertex_buffers[RE_MAX_FRAMES_IN_FLIGHT] = {0};
static re_buffer_t g_index_buffers[RE_MAX_FRAMES_IN_FLIGHT]  = {0};
static uint32_t g_frame_index                                = 0;

static void setup_style() {
  ImGuiStyle *style = igGetStyle();

  style->WindowRounding   = 5.0f;
  style->FrameRounding    = 5.0f;
  style->TabRounding      = 2.0f;
  style->WindowTitleAlign = (ImVec2){0.5f, 0.5f};
  style->TabBorderSize    = 0.0f;
  style->FrameBorderSize  = 0.0f;
  style->WindowBorderSize = 0.0f;
  style->ScrollbarSize    = 12.0f;

  ImVec4 *colors = style->Colors;

  colors[ImGuiCol_Text]                  = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
  colors[ImGuiCol_TextDisabled]          = (ImVec4){0.55f, 0.61f, 0.71f, 1.00f};
  colors[ImGuiCol_WindowBg]              = (ImVec4){0.09f, 0.08f, 0.15f, 0.95f};
  colors[ImGuiCol_ChildBg]               = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_PopupBg]               = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_Border]                = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_BorderShadow]          = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_FrameBg]               = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_FrameBgHovered]        = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_FrameBgActive]         = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_TitleBg]               = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_TitleBgActive]         = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_TitleBgCollapsed]      = (ImVec4){0.09f, 0.08f, 0.15f, 0.76f};
  colors[ImGuiCol_MenuBarBg]             = (ImVec4){0.09f, 0.08f, 0.15f, 0.76f};
  colors[ImGuiCol_ScrollbarBg]           = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_ScrollbarGrab]         = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_ScrollbarGrabHovered]  = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_ScrollbarGrabActive]   = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_CheckMark]             = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_SliderGrab]            = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_SliderGrabActive]      = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Button]                = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_ButtonHovered]         = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_ButtonActive]          = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Header]                = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_HeaderHovered]         = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_HeaderActive]          = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Separator]             = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_SeparatorHovered]      = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_SeparatorActive]       = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_ResizeGrip]            = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_ResizeGripHovered]     = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_ResizeGripActive]      = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Tab]                   = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_TabHovered]            = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_TabActive]             = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_TabUnfocused]          = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_TabUnfocusedActive]    = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_PlotLines]             = (ImVec4){0.75f, 0.29f, 0.18f, 1.00f};
  colors[ImGuiCol_PlotLinesHovered]      = (ImVec4){0.84f, 0.46f, 0.26f, 1.00f};
  colors[ImGuiCol_PlotHistogram]         = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
  colors[ImGuiCol_PlotHistogramHovered]  = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
  colors[ImGuiCol_TextSelectedBg]        = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_DragDropTarget]        = (ImVec4){1.00f, 1.00f, 0.00f, 0.90f};
  colors[ImGuiCol_NavHighlight]          = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_NavWindowingHighlight] = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
  colors[ImGuiCol_NavWindowingDimBg]     = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
  colors[ImGuiCol_ModalWindowDimBg]      = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};
}

void eg_imgui_init(re_window_t *window, re_render_target_t *render_target) {
  memset(&g_pipeline, 0, sizeof(g_pipeline));
  memset(&g_atlas, 0, sizeof(g_atlas));
  memset(g_vertex_buffers, 0, sizeof(g_vertex_buffers));
  memset(g_index_buffers, 0, sizeof(g_index_buffers));

  igCreateContext(NULL);
  ImGuiIO *io = igGetIO();

  // Create pipeline
  {
    eg_init_pipeline_spv(
        &g_pipeline,
        (const char *[]){"/shaders/imgui.vert.spv", "/shaders/imgui.frag.spv"},
        2,
        eg_imgui_pipeline_params());
  }

  // Load font
  {
    eg_file_t *file = eg_file_open_read("/assets/fonts/opensans_semibold.ttf");
    assert(file);
    size_t size        = eg_file_size(file);
    uint8_t *font_data = calloc(1, size);
    eg_file_read_bytes(file, font_data, size);
    eg_file_close(file);

    ImFontConfig *config = ImFontConfig_ImFontConfig();
    config->GlyphOffset  = (ImVec2){1.0f, -1.0f};

    ImFontAtlas_AddFontFromMemoryTTF(
        io->Fonts, font_data, (int)size, 17.0f, config, NULL);

    ImFontConfig_destroy(config);
  }

  // Setup style
  setup_style();

  // Upload atlas
  {
    unsigned char *pixels;
    int width, height;
    ImFontAtlas_GetTexDataAsRGBA32(io->Fonts, &pixels, &width, &height, NULL);

    re_image_init(
        &g_atlas,
        &(re_image_options_t){
            .width           = (uint32_t)width,
            .height          = (uint32_t)height,
            .layer_count     = 1,
            .mip_level_count = 1,
            .format          = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = RE_IMAGE_USAGE_SAMPLED | RE_IMAGE_USAGE_TRANSFER_DST,
        });

    re_image_upload(
        &g_atlas,
        g_ctx.transient_command_pool,
        pixels,
        (uint32_t)width,
        (uint32_t)height,
        0,
        0);

    io->Fonts->TexID = (void *)&g_atlas;
  }

  ImGui_ImplGlfw_InitForVulkan(window->glfw_window, false);
}

void eg_imgui_begin() {
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();
}

void eg_imgui_end() { igRender(); }

static void create_or_resize_buffer(
    re_buffer_t *buffer, re_buffer_usage_t usage, size_t size) {
  if (buffer->buffer == VK_NULL_HANDLE) {
    re_buffer_init(
        buffer,
        &(re_buffer_options_t){
            .usage = usage, .memory = RE_BUFFER_MEMORY_HOST, .size = size});
    return;
  }
  if (buffer->size < size) {
    re_buffer_destroy(buffer);
    re_buffer_init(
        buffer,
        &(re_buffer_options_t){
            .usage = usage, .memory = RE_BUFFER_MEMORY_HOST, .size = size});
    return;
  }
}

void eg_imgui_draw(re_cmd_buffer_t *cmd_buffer) {
  g_frame_index = (g_frame_index + 1) % RE_MAX_FRAMES_IN_FLIGHT;

  ImDrawData *draw_data = igGetDrawData();

  // Avoid rendering when minimized, scale coordinates for retina displays
  // (screen coordinates != framebuffer coordinates)
  int fb_width =
      (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
  int fb_height =
      (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
  if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0) return;

  // Create the Vertex and Index buffers:
  size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
  size_t index_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
  create_or_resize_buffer(
      &g_vertex_buffers[g_frame_index], RE_BUFFER_USAGE_VERTEX, vertex_size);
  create_or_resize_buffer(
      &g_index_buffers[g_frame_index], RE_BUFFER_USAGE_INDEX, index_size);

  // Upload vertex and index data
  {
    ImDrawVert *vtx_dst = NULL;
    ImDrawIdx *idx_dst  = NULL;

    bool res;
    res = re_buffer_map_memory(
        &g_vertex_buffers[g_frame_index], (void **)(&vtx_dst));
    assert(res);
    res = re_buffer_map_memory(
        &g_index_buffers[g_frame_index], (void **)(&idx_dst));
    assert(res);

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
      const ImDrawList *cmd_list = draw_data->CmdLists[n];
      memcpy(
          vtx_dst,
          cmd_list->VtxBuffer.Data,
          cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
      memcpy(
          idx_dst,
          cmd_list->IdxBuffer.Data,
          cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
      vtx_dst += cmd_list->VtxBuffer.Size;
      idx_dst += cmd_list->IdxBuffer.Size;
    }

    re_buffer_unmap_memory(&g_vertex_buffers[g_frame_index]);
    re_buffer_unmap_memory(&g_index_buffers[g_frame_index]);
  }

  // Bind pipeline
  re_cmd_bind_pipeline(cmd_buffer, &g_pipeline);

  // Bind buffers
  re_cmd_bind_index_buffer(
      cmd_buffer, &g_index_buffers[g_frame_index], 0, RE_INDEX_TYPE_UINT16);
  re_cmd_bind_vertex_buffers(
      cmd_buffer, 0, 1, &g_vertex_buffers[g_frame_index], &(size_t){0});

  // Setup viewport
  re_cmd_set_viewport(
      cmd_buffer,
      &(re_viewport_t){.x         = 0,
                       .y         = 0,
                       .width     = (float)fb_width,
                       .height    = (float)fb_height,
                       .min_depth = 0.0f,
                       .max_depth = 1.0f});

  // Push constants
  // Setup scale and translation:
  // Our visible imgui space lies from draw_data->DisplayPos (top left) to
  // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is
  // typically (0,0) for single viewport apps.
  {

    struct {
      float scale[2];
      float translate[2];
    } pc;

    pc.scale[0]     = 2.0f / draw_data->DisplaySize.x;
    pc.scale[1]     = 2.0f / draw_data->DisplaySize.y;
    pc.translate[0] = -1.0f - draw_data->DisplayPos.x * pc.scale[0];
    pc.translate[1] = -1.0f - draw_data->DisplayPos.y * pc.scale[1];

    re_cmd_push_constants(cmd_buffer, &g_pipeline, 0, sizeof(pc), &pc);
  }

  ImVec2 clip_off = draw_data->DisplayPos; // (0,0) unless using multi-viewports
  ImVec2 clip_scale =
      draw_data->FramebufferScale; // (1,1) unless using retina display which
                                   // are often (2,2)

  int vtx_offset = 0;
  int idx_offset = 0;
  for (int32_t n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList *cmd_list = draw_data->CmdLists[n];

    for (int32_t cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd *pcmd = &cmd_list->CmdBuffer.Data[cmd_i];

      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        // The texture for the draw call is specified by pcmd->TextureId.
        // The vast majority of draw calls will use the imgui texture atlas,
        // which value you have set yourself during initialization.
        re_cmd_bind_image(cmd_buffer, 0, 0, (re_image_t *)pcmd->TextureId);
        re_cmd_bind_descriptor_set(cmd_buffer, &g_pipeline, 0);

        ImVec4 clip_rect;
        clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
        clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
        clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
        clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

        if (clip_rect.x < fb_width && clip_rect.y < fb_height &&
            clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
          // Apply scissor/clipping rectangle
          re_cmd_set_scissor(
              cmd_buffer,
              &(re_rect_2d_t){
                  .offset = {(int32_t)(clip_rect.x), (int32_t)(clip_rect.y)},
                  .extent = {(uint32_t)(clip_rect.z - clip_rect.x),
                             (uint32_t)(clip_rect.w - clip_rect.y)}});

          // Draw
          re_cmd_draw_indexed(
              cmd_buffer,
              (uint32_t)pcmd->ElemCount,
              1,
              idx_offset,
              vtx_offset,
              0);
        }
      }
      idx_offset += pcmd->ElemCount;
    }
    vtx_offset += cmd_list->VtxBuffer.Size;
  }
}

void eg_imgui_process_event(const re_event_t *event) {
  switch (event->type) {
  case RE_EVENT_BUTTON_PRESSED: {
    ImGui_ImplGlfw_MouseButtonCallback(
        event->window->glfw_window,
        event->mouse.button,
        GLFW_PRESS,
        event->mouse.mods);
    break;
  }
  case RE_EVENT_BUTTON_RELEASED: {
    ImGui_ImplGlfw_MouseButtonCallback(
        event->window->glfw_window,
        event->mouse.button,
        GLFW_RELEASE,
        event->mouse.mods);
    break;
  }
  case RE_EVENT_SCROLLED: {
    ImGui_ImplGlfw_ScrollCallback(
        event->window->glfw_window, event->scroll.x, event->scroll.y);
    break;
  }
  case RE_EVENT_KEY_PRESSED: {
    ImGui_ImplGlfw_KeyCallback(
        event->window->glfw_window,
        event->keyboard.key,
        event->keyboard.scancode,
        GLFW_PRESS,
        event->keyboard.mods);
    break;
  }
  case RE_EVENT_KEY_REPEATED: {
    ImGui_ImplGlfw_KeyCallback(
        event->window->glfw_window,
        event->keyboard.key,
        event->keyboard.scancode,
        GLFW_REPEAT,
        event->keyboard.mods);
    break;
  }
  case RE_EVENT_KEY_RELEASED: {
    ImGui_ImplGlfw_KeyCallback(
        event->window->glfw_window,
        event->keyboard.key,
        event->keyboard.scancode,
        GLFW_RELEASE,
        event->keyboard.mods);
    break;
  }
  case RE_EVENT_CODEPOINT_INPUT: {
    ImGui_ImplGlfw_CharCallback(event->window->glfw_window, event->codepoint);
    break;
  }
  default: break;
  }
}

void eg_imgui_destroy() {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  re_pipeline_destroy(&g_pipeline);

  re_image_destroy(&g_atlas);

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; ++i) {
    re_buffer_destroy(&g_vertex_buffers[i]);
    re_buffer_destroy(&g_index_buffers[i]);
  }

  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(NULL);
}
