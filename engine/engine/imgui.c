#include "imgui.h"

#include "filesystem.h"
#include <fstd_util.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <renderer/window.h>

static void check_vk_result_fn(VkResult result) {
  if (result != VK_SUCCESS) {
    RE_LOG_FATAL("Failed to initialize IMGUI!");
    abort();
  }
}

static void setup_style() {
  ImGuiStyle *style = igGetStyle();

  style->WindowRounding = 5.0f;
  style->FrameRounding = 5.0f;
  style->TabRounding = 2.0f;
  style->WindowTitleAlign = (ImVec2){0.5f, 0.5f};
  style->TabBorderSize = 0.0f;
  style->FrameBorderSize = 0.0f;
  style->WindowBorderSize = 0.0f;
  style->ScrollbarSize = 12.0f;

  ImVec4 *colors = style->Colors;

  colors[ImGuiCol_Text] = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
  colors[ImGuiCol_TextDisabled] = (ImVec4){0.55f, 0.61f, 0.71f, 1.00f};
  colors[ImGuiCol_WindowBg] = (ImVec4){0.09f, 0.08f, 0.15f, 0.95f};
  colors[ImGuiCol_ChildBg] = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_PopupBg] = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_Border] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_BorderShadow] = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_FrameBg] = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_FrameBgHovered] = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_FrameBgActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_TitleBg] = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_TitleBgActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_TitleBgCollapsed] = (ImVec4){0.09f, 0.08f, 0.15f, 0.76f};
  colors[ImGuiCol_MenuBarBg] = (ImVec4){0.09f, 0.08f, 0.15f, 0.76f};
  colors[ImGuiCol_ScrollbarBg] = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_ScrollbarGrab] = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_ScrollbarGrabHovered] = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_ScrollbarGrabActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_CheckMark] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_SliderGrab] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_SliderGrabActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Button] = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_ButtonHovered] = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_ButtonActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Header] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_HeaderHovered] = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_HeaderActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Separator] = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_SeparatorHovered] = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_SeparatorActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_ResizeGrip] = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_ResizeGripHovered] = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_ResizeGripActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_Tab] = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_TabHovered] = (ImVec4){0.35f, 0.41f, 0.53f, 1.00f};
  colors[ImGuiCol_TabActive] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_TabUnfocused] = (ImVec4){0.09f, 0.08f, 0.15f, 1.00f};
  colors[ImGuiCol_TabUnfocusedActive] = (ImVec4){0.15f, 0.17f, 0.27f, 1.00f};
  colors[ImGuiCol_PlotLines] = (ImVec4){0.75f, 0.29f, 0.18f, 1.00f};
  colors[ImGuiCol_PlotLinesHovered] = (ImVec4){0.84f, 0.46f, 0.26f, 1.00f};
  colors[ImGuiCol_PlotHistogram] = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
  colors[ImGuiCol_PlotHistogramHovered] = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
  colors[ImGuiCol_TextSelectedBg] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_DragDropTarget] = (ImVec4){1.00f, 1.00f, 0.00f, 0.90f};
  colors[ImGuiCol_NavHighlight] = (ImVec4){0.23f, 0.27f, 0.40f, 1.00f};
  colors[ImGuiCol_NavWindowingHighlight] = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
  colors[ImGuiCol_NavWindowingDimBg] = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
  colors[ImGuiCol_ModalWindowDimBg] = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};
}

void eg_imgui_init(re_window_t *window, re_render_target_t *render_target) {
  igCreateContext(NULL);
  ImGuiIO *io = igGetIO();

  {
    eg_file_t *file = eg_file_open_read("/assets/fonts/opensans_semibold.ttf");
    assert(file);
    size_t size = eg_file_size(file);
    uint8_t *font_data = calloc(1, size);
    eg_file_read_bytes(file, font_data, size);
    eg_file_close(file);

    ImFontConfig *config = ImFontConfig_ImFontConfig();
    config->GlyphOffset = (ImVec2){1.0f, -1.0f};

    ImFontAtlas_AddFontFromMemoryTTF(
        io->Fonts, font_data, size, 17.0f, config, NULL);

    ImFontConfig_destroy(config);
  }

  // Setup style
  setup_style();

  ImGui_ImplGlfw_InitForVulkan(window->glfw_window, false);

  // Setup Vulkan binding
  ImGui_ImplVulkan_InitInfo init_info = {0};
  init_info.Instance = g_ctx.instance;
  init_info.PhysicalDevice = g_ctx.physical_device;
  init_info.Device = g_ctx.device;
  init_info.QueueFamily = g_ctx.graphics_queue_family_index;
  init_info.Queue = g_ctx.graphics_queue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = g_ctx.descriptor_pool;
  init_info.SampleCount = render_target->sample_count;
  init_info.Allocator = NULL;
  init_info.CheckVkResultFn = check_vk_result_fn;
  ImGui_ImplVulkan_Init(&init_info, render_target->render_pass);

  // Upload Fonts
  {
    // Use any command queue
    VkCommandPool command_pool = g_ctx.graphics_command_pool;
    re_cmd_buffer_t *cmd_buffer = re_window_get_current_command_buffer(window);

    VK_CHECK(vkResetCommandPool(g_ctx.device, command_pool, 0));

    re_begin_cmd_buffer(
        cmd_buffer,
        &(re_cmd_buffer_begin_info_t){.usage =
                                          RE_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT});

    ImGui_ImplVulkan_CreateFontsTexture(cmd_buffer->cmd_buffer);

    VkSubmitInfo end_info = {0};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &cmd_buffer->cmd_buffer;

    re_end_cmd_buffer(cmd_buffer);

    mtx_lock(&g_ctx.queue_mutex);
    VK_CHECK(vkQueueSubmit(g_ctx.graphics_queue, 1, &end_info, VK_NULL_HANDLE));
    mtx_unlock(&g_ctx.queue_mutex);

    VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
  }
}

void eg_imgui_begin() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();
}

void eg_imgui_end() { igRender(); }

void eg_imgui_draw(re_cmd_buffer_t *cmd_buffer) {
  ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), cmd_buffer->cmd_buffer);
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

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(NULL);
}
