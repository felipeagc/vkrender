// dear imgui: Platform Binding for SDL2
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)
// (Info: SDL2 is a cross-platform general purpose library for handling windows, inputs, graphics context creation, etc.)

// Implemented features:
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [X] Platform: Clipboard support.
//  [X] Platform: Keyboard arrays indexed using SDL_SCANCODE_* codes, e.g. ImGui::IsKeyPressed(SDL_SCANCODE_SPACE).
// Missing features:
//  [ ] Platform: SDL2 handling of IME under Windows appears to be broken and it explicitly disable the regular Windows IME. You can restore Windows IME by compiling SDL with SDL_DISABLE_WINDOWS_IME.
//  [ ] Platform: Gamepad support (need to use SDL_GameController API to fill the io.NavInputs[] value when ImGuiConfigFlags_NavEnableGamepad is set).

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once


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

struct SDL_Window;
typedef union SDL_Event SDL_Event;

CIMGUI_API bool     ImGui_ImplSDL2_InitForOpenGL(SDL_Window* window, void* sdl_gl_context);
CIMGUI_API bool     ImGui_ImplSDL2_InitForVulkan(SDL_Window* window);
CIMGUI_API void     ImGui_ImplSDL2_Shutdown();
CIMGUI_API void     ImGui_ImplSDL2_NewFrame(SDL_Window* window);
CIMGUI_API bool     ImGui_ImplSDL2_ProcessEvent(const SDL_Event* event);
