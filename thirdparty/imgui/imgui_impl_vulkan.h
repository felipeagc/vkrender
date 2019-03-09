// dear imgui: Renderer for Vulkan
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// Missing features:
//  [ ] Renderer: User texture binding. Changes of ImTextureID aren't supported by this binding! See https://github.com/ocornut/imgui/pull/914

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// The aim of imgui_impl_vulkan.h/.cpp is to be usable in your engine without any modification.
// IF YOU FEEL YOU NEED TO MAKE ANY CHANGE TO THIS CODE, please share them and your feedback at https://github.com/ocornut/imgui/

#pragma once

#include <vulkan/vulkan.h>

#if defined _WIN32 || defined __CYGWIN__
    #ifdef CIMGUI_NO_EXPORT
        #define API
    #else
        #define API __declspec(dllexport)
    #endif
    #ifndef __GNUC__
    #define snprintf sprintf_s
    #endif
#else
    #define API
#endif

#if defined __cplusplus
    #define EXTERN extern "C"
#else
    #include <stdbool.h>
    #define EXTERN extern
#endif

#define CIMGUI_API EXTERN API

#define IMGUI_VK_QUEUED_FRAMES      2

// Please zero-clear before use.
typedef struct ImGui_ImplVulkan_InitInfo
{
    VkInstance                      Instance;
    VkPhysicalDevice                PhysicalDevice;
    VkDevice                        Device;
    uint32_t                        QueueFamily;
    VkQueue                         Queue;
    VkPipelineCache                 PipelineCache;
    VkDescriptorPool                DescriptorPool;
    const VkAllocationCallbacks*    Allocator;
    void                            (*CheckVkResultFn)(VkResult err);
} ImGui_ImplVulkan_InitInfo;

// Called by user code
CIMGUI_API bool     ImGui_ImplVulkan_Init(ImGui_ImplVulkan_InitInfo* info, VkRenderPass render_pass);
CIMGUI_API void     ImGui_ImplVulkan_Shutdown();
CIMGUI_API void     ImGui_ImplVulkan_NewFrame();
CIMGUI_API void     ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer);
CIMGUI_API bool     ImGui_ImplVulkan_CreateFontsTexture(VkCommandBuffer command_buffer);
CIMGUI_API void     ImGui_ImplVulkan_InvalidateFontUploadObjects();

// Called by ImGui_ImplVulkan_Init() might be useful elsewhere.
CIMGUI_API bool     ImGui_ImplVulkan_CreateDeviceObjects();
CIMGUI_API void     ImGui_ImplVulkan_InvalidateDeviceObjects();
