#pragma once

#include <GLFW/glfw3.h>

typedef struct re_window_t re_window_t;

typedef enum re_event_type_t {
  RE_EVENT_NONE,
  RE_EVENT_WINDOW_MOVED,
  RE_EVENT_WINDOW_RESIZED,
  RE_EVENT_WINDOW_CLOSED,
  RE_EVENT_WINDOW_REFRESH,
  RE_EVENT_WINDOW_FOCUSED,
  RE_EVENT_WINDOW_DEFOCUSED,
  RE_EVENT_WINDOW_ICONIFIED,
  RE_EVENT_WINDOW_UNICONIFIED,
  RE_EVENT_FRAMEBUFFER_RESIZED,
  RE_EVENT_BUTTON_PRESSED,
  RE_EVENT_BUTTON_RELEASED,
  RE_EVENT_CURSOR_MOVED,
  RE_EVENT_CURSOR_ENTERED,
  RE_EVENT_CURSOR_LEFT,
  RE_EVENT_SCROLLED,
  RE_EVENT_KEY_PRESSED,
  RE_EVENT_KEY_REPEATED,
  RE_EVENT_KEY_RELEASED,
  RE_EVENT_CODEPOINT_INPUT,
  RE_EVENT_MONITOR_CONNECTED,
  RE_EVENT_MONITOR_DISCONNECTED,
#if GLFW_VERSION_MINOR >= 2
  RE_EVENT_JOYSTICK_CONNECTED,
  RE_EVENT_JOYSTICK_DISCONNECTED,
#endif
#if GLFW_VERSION_MINOR >= 3
  RE_EVENT_WINDOW_MAXIMIZED,
  RE_EVENT_WINDOW_UNMAXIMIZED,
  RE_EVENT_WINDOW_SCALE_CHANGED,
#endif
} re_event_type_t;

typedef struct re_event_t {
  re_event_type_t type;
  union {
    re_window_t *window;
    GLFWmonitor *monitor;
    int joystick;
  };
  union {
    struct {
      int x;
      int y;
    } pos;
    struct {
      int width;
      int height;
    } size;
    struct {
      double x;
      double y;
    } scroll;
    struct {
      int key;
      int scancode;
      int mods;
    } keyboard;
    struct {
      int button;
      int mods;
    } mouse;
    unsigned int codepoint;
#if GLFW_VERSION_MINOR >= 3
    struct {
      float x;
      float y;
    } scale;
#endif
  };
} re_event_t;
