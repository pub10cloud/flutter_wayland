// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WL_EGL_PLATFORM
#define WL_EGL_PLATFORM 1
#endif

#include "wayland_display.h"

#include <stdlib.h>
#include <unistd.h>

#include <cstring>

namespace flutter {

#define DISPLAY reinterpret_cast<WaylandDisplay*>(data)

const wl_registry_listener WaylandDisplay::kRegistryListener = {
    .global = [](void* data,
                 struct wl_registry* wl_registry,
                 uint32_t name,
                 const char* interface,
                 uint32_t version) -> void {
      DISPLAY->AnnounceRegistryInterface(wl_registry, name, interface, version);
    },

    .global_remove =
        [](void* data, struct wl_registry* wl_registry, uint32_t name) -> void {
      DISPLAY->UnannounceRegistryInterface(wl_registry, name);
    },
};

const wl_shell_surface_listener WaylandDisplay::kShellSurfaceListener = {
    .ping = [](void* data,
               struct wl_shell_surface* wl_shell_surface,
               uint32_t serial) -> void {
      wl_shell_surface_pong(DISPLAY->shell_surface_, serial);
    },

    .configure = [](void* data,
                    struct wl_shell_surface* wl_shell_surface,
                    uint32_t edges,
                    int32_t width,
                    int32_t height) -> void {
      FLWAY_ERROR << "Unhandled resize." << std::endl;
    },

    .popup_done = [](void* data,
                     struct wl_shell_surface* wl_shell_surface) -> void {
      // Nothing to do.
    },
};

// Add For Pointer Event Handling Start
const wl_pointer_listener WaylandDisplay::kPointerListener = {
    .enter = [] (void *data,
                struct wl_pointer *wl_pointer,
                uint32_t serial,
                struct wl_surface *surface,
                wl_fixed_t surface_x,
                wl_fixed_t surface_y) -> void {
      // Enter Event
      //FLWAY_LOG << "Enter Event " << std::endl;
      // Drawing Cursor
      WaylandDisplay*   display = static_cast<WaylandDisplay*>(data);
      wl_buffer*        buffer = nullptr;
      wl_cursor_image*  image = nullptr;

      if (display->default_cursor_) {
        image = display->default_cursor_->images[0];
        buffer = wl_cursor_image_get_buffer(image);
        if (buffer == nullptr) {
          return;
        }
        wl_pointer_set_cursor(wl_pointer, serial,
                  display->cursor_surface_,
                  image->hotspot_x,
                  image->hotspot_y);
        wl_surface_attach(display->cursor_surface_, buffer, 0, 0);
        wl_surface_damage(display->cursor_surface_, 0, 0,
              image->width, image->height);
        wl_surface_commit(display->cursor_surface_);
	    }
    },

    .leave = [] (void *data,
                struct wl_pointer *wl_pointer,
                uint32_t serial,
                struct wl_surface *surface) -> void {
      // Leave Event
      //FLWAY_LOG << "Leave Event" << std::endl;
    },

    .motion = [] (void *data,
                  struct wl_pointer *wl_pointer,
                  uint32_t time,
                  wl_fixed_t surface_x,
                  wl_fixed_t surface_y) -> void {
      
      WaylandDisplay* display = static_cast<WaylandDisplay*>(data);

      //FLWAY_LOG << "s_x=" << surface_x << ",s_y=" << surface_y << std::endl;

      display->mouse_x_ = wl_fixed_to_double(surface_x);
      display->mouse_y_ = wl_fixed_to_double(surface_y);

      //FLWAY_LOG << "m_x=" << display->mouse_x_ << ",m_y=" \
                            << display->mouse_y_ << std::endl;

    },

    .button = [] (void *data,
                  struct wl_pointer *wl_pointer,
                  uint32_t serial,
                  uint32_t time,
                  uint32_t button,
                  uint32_t state) -> void {

      WaylandDisplay* display = static_cast<WaylandDisplay*>(data);

      if(display->application == nullptr) {
        FLWAY_ERROR << "Not exist an application to send events." << std::endl;
        return;
      }

      //FLWAY_LOG << "m_x=" << display->mouse_x_ << ",m_y=" << display->mouse_y_ << std::endl;
      //FLWAY_LOG << "button=" << button << ",state=" << state << std::endl;

      // Button Event
      if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        display->application->SendPointerEvent(button, 
          display->mouse_x_, display->mouse_y_);
      }
      else {
        display->application->SendPointerEvent(0, 
          display->mouse_x_, display->mouse_y_);
      }        
    }, 

    .axis = [] (void *data,
                struct wl_pointer *wl_pointer,
                uint32_t time,
                uint32_t axis,
                wl_fixed_t value) -> void {
      //FLWAY_ERROR << "Unhandled Axis" << std::endl;
    },
};
// Add For Pointer Event Handling End
// Add For Touch Event Handling Start
const wl_touch_listener WaylandDisplay::kTouchListener = {
    .down = [] (void *data,
            struct wl_touch *wl_touch,
            uint32_t serial,
            uint32_t time,
            struct wl_surface *surface,
            int32_t id,
            wl_fixed_t x,
            wl_fixed_t y) -> void {

      WaylandDisplay* display = static_cast<WaylandDisplay*>(data);

      if(display->application == nullptr) {
        FLWAY_ERROR << "Not exist an application to send events." << std::endl;
        return;
      }

      // Flutter emebedder doesn't seem to allow multi touch. 
      // So if it get another touch_id, to do nothing.

      if(display->touch_id_ != -1) { 
        return;
      }

      display->touch_x_ = wl_fixed_to_double(x);
      display->touch_y_ = wl_fixed_to_double(y);
      //FLWAY_LOG << "t_x=" << display->touch_x_ << ",t_y=" << display->touch_y_ << std::endl;
      //FLWAY_LOG << "phase = down, id=" << id << std::endl;

      display->application->SendTouchEvent(EventPhase::down, 
          display->touch_x_, display->touch_y_);

      display->touch_id_ = id;

    },
    .up = [] (void *data,
            struct wl_touch *wl_touch,
            uint32_t serial,
            uint32_t time,
            int32_t id) -> void {

      WaylandDisplay* display = static_cast<WaylandDisplay*>(data);

      if(display->application == nullptr) {
        FLWAY_ERROR << "Not exist an application to send events." << std::endl;
        return;
      }

      if(display->touch_id_ != id) {
        return;
      }

      //FLWAY_LOG << "t_x=" << display->touch_x_ << ",t_y=" << display->touch_y_ << std::endl;
      //FLWAY_LOG << "phase = up, id=" << id << std::endl;

      display->application->SendTouchEvent(EventPhase::up, 
          display->touch_x_, display->touch_y_);
      
      display->touch_id_ = -1;

    },
    .motion = [] (void *data,
                struct wl_touch *wl_touch,
                uint32_t time,
                int32_t id,
                wl_fixed_t x,
                wl_fixed_t y) -> void {      
      
      WaylandDisplay* display = static_cast<WaylandDisplay*>(data);

      if(display->touch_id_ != id) {
        return;
      }

      display->touch_x_ = wl_fixed_to_double(x);
      display->touch_y_ = wl_fixed_to_double(y);

      //FLWAY_LOG << "t_x=" << display->touch_x_ << ",t_y=" << display->touch_y_ << std::endl;
      //FLWAY_LOG << "phase = move, id=" << id << std::endl;

      display->application->SendTouchEvent(EventPhase::move, 
          display->touch_x_, display->touch_y_);

      display->touch_id_ = id;

    },
    .frame = [] (void *data,
                struct wl_touch *wl_touch) -> void {

    },
    .cancel = [] (void *data,
                struct wl_touch *wl_touch) -> void {

      WaylandDisplay* display = static_cast<WaylandDisplay*>(data);

      display->touch_x_ = 0.0;
      display->touch_y_ = 0.0;

      //FLWAY_LOG << "t_x=" << display->touch_x_ << ",t_y=" << display->touch_y_ << std::endl;
      //FLWAY_LOG << "phase = cancel, id=" << display->touch_id_ << std::endl;

      display->application->SendTouchEvent(EventPhase::cancel, 
          display->touch_x_, display->touch_y_);

      display->touch_id_ = -1;

    },
};
// Add For Touch Event Handling End
// Add For Keyboard Event Handling Start
const wl_keyboard_listener WaylandDisplay::kKeyboardListener = {
	  .keymap = [] (void *data,
                struct wl_keyboard *wl_keyboard,
                uint32_t format,
                int32_t fd,
                uint32_t size) -> void {
  	  /* Just so we don’t leak the keymap fd */
      close(fd);
    },
	  .enter = [] (void *data,
                struct wl_keyboard *wl_keyboard,
                uint32_t serial,
                struct wl_surface *surface,
                struct wl_array *keys) -> void {

    },
	  .leave = [] (void *data,
                struct wl_keyboard *wl_keyboard,
                uint32_t serial,
                struct wl_surface *surface) -> void {

  },
	  .key = [] (void *data,
                struct wl_keyboard *wl_keyboard,
                uint32_t serial,
                uint32_t time,
                uint32_t key,
                uint32_t state) -> void {

    },
	  .modifiers = [] (void *data,
                struct wl_keyboard *wl_keyboard,
                uint32_t serial,
                uint32_t mods_depressed,
                uint32_t mods_latched,
                uint32_t mods_locked,
                uint32_t group) -> void {

    },
};
// Add For Keyboard Event Handling End

const wl_seat_listener WaylandDisplay::kSeatListener = {
    .capabilities = [] (void *data, 
                        wl_seat *seat,
                        uint32_t caps) -> void {

      WaylandDisplay* display = static_cast<WaylandDisplay*>(data);

      // Pointer Event
      if ((caps & WL_SEAT_CAPABILITY_POINTER) && display->pointer_ == nullptr) {
        display->pointer_  = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(display->pointer_, &kPointerListener, display);
      } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && display->pointer_ != nullptr) {
        wl_pointer_destroy(display->pointer_);
        display->pointer_ = nullptr;
      }

      // Keyboard Event
      if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && display->keyboard_ == nullptr) {
        display->keyboard_ = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(display->keyboard_, &kKeyboardListener, display);
      } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && display->keyboard_ != nullptr) {
        wl_keyboard_destroy(display->keyboard_);
        display->keyboard_ = nullptr;
      }

      // Touch Event
      if ((caps & WL_SEAT_CAPABILITY_TOUCH) && display->touch_ == nullptr) {
        display->touch_ = wl_seat_get_touch(seat);
        wl_touch_set_user_data(display->touch_, display);
        wl_touch_add_listener(display->touch_, &kTouchListener, display);
      } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && display->touch_ != nullptr) {
        // FIXME  
        // It needs to manage multiple seat instances, and might change the whole class design.
        // wl_touch_destroy(display->touch_);
        // display->touch_ = nullptr;
      }

    },
};
// Add For Pointer Event Handling End

WaylandDisplay::WaylandDisplay(size_t width, size_t height)
    : screen_width_(width), screen_height_(height) {
  if (screen_width_ == 0 || screen_height_ == 0) {
    FLWAY_ERROR << "Invalid screen dimensions." << std::endl;
    return;
  }

  display_ = wl_display_connect(nullptr);

  if (!display_) {
    FLWAY_ERROR << "Could not connect to the wayland display." << std::endl;
    return;
  }

  registry_ = wl_display_get_registry(display_);
  if (!registry_) {
    FLWAY_ERROR << "Could not get the wayland registry." << std::endl;
    return;
  }

  wl_registry_add_listener(registry_, &kRegistryListener, this);

  wl_display_roundtrip(display_);

  if (!SetupEGL()) {
    FLWAY_ERROR << "Could not setup EGL." << std::endl;
    return;
  }

  valid_ = true;
}

WaylandDisplay::~WaylandDisplay() {

  // Add For Drawing Cursor Start
  if (cursor_surface_) {
    wl_surface_destroy(cursor_surface_);
    cursor_surface_ = nullptr;
  }

  if (cursor_theme_) {
    wl_cursor_theme_destroy(cursor_theme_);
    cursor_surface_ = nullptr;
  }
  // Add For Drawing Cursor End

  if (shell_surface_) {
    wl_shell_surface_destroy(shell_surface_);
    shell_surface_ = nullptr;
  }

  if (shell_) {
    wl_shell_destroy(shell_);
    shell_ = nullptr;
  }

  if (egl_surface_) {
    eglDestroySurface(egl_display_, egl_surface_);
    egl_surface_ = nullptr;
  }

  if (egl_display_) {
    eglTerminate(egl_display_);
    egl_display_ = nullptr;
  }

  if (window_) {
    wl_egl_window_destroy(window_);
    window_ = nullptr;
  }

  if (surface_) {
    wl_surface_destroy(surface_);
    surface_ = nullptr;
  }

  if (compositor_) {
    wl_compositor_destroy(compositor_);
    compositor_ = nullptr;
  }

  if (registry_) {
    wl_registry_destroy(registry_);
    registry_ = nullptr;
  }

  if (display_) {
    wl_display_flush(display_);
    wl_display_disconnect(display_);
    display_ = nullptr;
  }
}

bool WaylandDisplay::IsValid() const {
  return valid_;
}

bool WaylandDisplay::Run() {
  if (!valid_) {
    FLWAY_ERROR << "Could not run an invalid display." << std::endl;
    return false;
  }

  while (valid_) {
    wl_display_dispatch(display_);
  }

  return true;
}

static void LogLastEGLError() {
  struct EGLNameErrorPair {
    const char* name;
    EGLint code;
  };

#define _EGL_ERROR_DESC(a) \
  { #a, a }

  const EGLNameErrorPair pairs[] = {
      _EGL_ERROR_DESC(EGL_SUCCESS),
      _EGL_ERROR_DESC(EGL_NOT_INITIALIZED),
      _EGL_ERROR_DESC(EGL_BAD_ACCESS),
      _EGL_ERROR_DESC(EGL_BAD_ALLOC),
      _EGL_ERROR_DESC(EGL_BAD_ATTRIBUTE),
      _EGL_ERROR_DESC(EGL_BAD_CONTEXT),
      _EGL_ERROR_DESC(EGL_BAD_CONFIG),
      _EGL_ERROR_DESC(EGL_BAD_CURRENT_SURFACE),
      _EGL_ERROR_DESC(EGL_BAD_DISPLAY),
      _EGL_ERROR_DESC(EGL_BAD_SURFACE),
      _EGL_ERROR_DESC(EGL_BAD_MATCH),
      _EGL_ERROR_DESC(EGL_BAD_PARAMETER),
      _EGL_ERROR_DESC(EGL_BAD_NATIVE_PIXMAP),
      _EGL_ERROR_DESC(EGL_BAD_NATIVE_WINDOW),
      _EGL_ERROR_DESC(EGL_CONTEXT_LOST),
  };

#undef _EGL_ERROR_DESC

  const auto count = sizeof(pairs) / sizeof(EGLNameErrorPair);

  EGLint last_error = eglGetError();

  for (size_t i = 0; i < count; i++) {
    if (last_error == pairs[i].code) {
      FLWAY_ERROR << "EGL Error: " << pairs[i].name << " (" << pairs[i].code
                  << ")" << std::endl;
      return;
    }
  }

  FLWAY_ERROR << "Unknown EGL Error" << std::endl;
}

bool WaylandDisplay::SetupEGL() {
  if (!compositor_ || !shell_) {
    FLWAY_ERROR << "EGL setup needs missing compositor and shell connection."
                << std::endl;
    return false;
  }

  surface_ = wl_compositor_create_surface(compositor_);

  if (!surface_) {
    FLWAY_ERROR << "Could not create compositor surface." << std::endl;
    return false;
  }

  shell_surface_ = wl_shell_get_shell_surface(shell_, surface_);

  if (!shell_surface_) {
    FLWAY_ERROR << "Could not shell surface." << std::endl;
    return false;
  }

  wl_shell_surface_add_listener(shell_surface_, &kShellSurfaceListener, this);

  wl_shell_surface_set_title(shell_surface_, "Flutter");

  wl_shell_surface_set_toplevel(shell_surface_);

  window_ = wl_egl_window_create(surface_, screen_width_, screen_height_);

  if (!window_) {
    FLWAY_ERROR << "Could not create EGL window." << std::endl;
    return false;
  }

  if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not bind the ES API." << std::endl;
    return false;
  }

  egl_display_ = eglGetDisplay(display_);
  if (egl_display_ == EGL_NO_DISPLAY) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not access EGL display." << std::endl;
    return false;
  }

  if (eglInitialize(egl_display_, nullptr, nullptr) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not initialize EGL display." << std::endl;
    return false;
  }

  EGLConfig egl_config = nullptr;

  // Choose an EGL config to use for the surface and context.
  {
    EGLint attribs[] = {
        // clang-format off
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      8,
      EGL_DEPTH_SIZE,      0,
      EGL_STENCIL_SIZE,    0,
      EGL_NONE,            // termination sentinel
        // clang-format on
    };

    EGLint config_count = 0;

    if (eglChooseConfig(egl_display_, attribs, &egl_config, 1, &config_count) !=
        EGL_TRUE) {
      LogLastEGLError();
      FLWAY_ERROR << "Error when attempting to choose an EGL surface config."
                  << std::endl;
      return false;
    }

    if (config_count == 0 || egl_config == nullptr) {
      LogLastEGLError();
      FLWAY_ERROR << "No matching configs." << std::endl;
      return false;
    }
  }

  // Create an EGL window surface with the matched config.
  {
    const EGLint attribs[] = {EGL_NONE};

    egl_surface_ =
        eglCreateWindowSurface(egl_display_, egl_config, window_, attribs);

    if (surface_ == EGL_NO_SURFACE) {
      LogLastEGLError();
      FLWAY_ERROR << "EGL surface was null during surface selection."
                  << std::endl;
      return false;
    }
  }

  // Create an EGL context with the match config.
  {
    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    egl_context_ = eglCreateContext(egl_display_, egl_config,
                                    nullptr /* share group */, attribs);

    if (egl_context_ == EGL_NO_CONTEXT) {
      LogLastEGLError();
      FLWAY_ERROR << "Could not create an onscreen context." << std::endl;
      return false;
    }
  }

  // Add For Drawing Cursor
  cursor_surface_ = wl_compositor_create_surface(compositor_);

  return true;
}

void WaylandDisplay::AnnounceRegistryInterface(struct wl_registry* wl_registry,
                                               uint32_t name,
                                               const char* interface_name,
                                               uint32_t version) {
  if (strcmp(interface_name, "wl_compositor") == 0) {
    compositor_ = static_cast<decltype(compositor_)>(
        wl_registry_bind(wl_registry, name, &wl_compositor_interface, 1));
    return;
  }

  if (strcmp(interface_name, "wl_shell") == 0) {
    shell_ = static_cast<decltype(shell_)>(
        wl_registry_bind(wl_registry, name, &wl_shell_interface, 1));
    return;
  }

  // Add For Pointer Event Handling
  if (strcmp(interface_name, "wl_seat") == 0) {
    seat_ = static_cast<decltype(seat_)>(
        wl_registry_bind(wl_registry, name, &wl_seat_interface, 1));
		wl_seat_add_listener(seat_, &kSeatListener, this);
    return;
  }

  // Add For Drawing Cursor
  if (strcmp(interface_name, "wl_shm") == 0) {
    shm_ = static_cast<decltype(shm_)>(
        wl_registry_bind(wl_registry, name, &wl_shm_interface, 1));
  
    if (shm_ == nullptr) {
      FLWAY_ERROR << "unable to bind to wl_shm" << std::endl;
      return;
    }

    cursor_theme_ = wl_cursor_theme_load(NULL, 32, shm_);
    if (cursor_theme_ == nullptr) {
      FLWAY_ERROR << "unable to load default theme" << std::endl;
      return;
    }
    default_cursor_ =
      wl_cursor_theme_get_cursor(cursor_theme_, "left_ptr");
    if (default_cursor_ == nullptr) {
      FLWAY_ERROR << "unable to load default left pointer" << std::endl;
    }
  }
}

void WaylandDisplay::UnannounceRegistryInterface(
    struct wl_registry* wl_registry,
    uint32_t name) {}

// |flutter::FlutterApplication::RenderDelegate|
bool WaylandDisplay::OnApplicationContextMakeCurrent() {
  if (!valid_) {
    FLWAY_ERROR << "Invalid display." << std::endl;
    return false;
  }

  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) !=
      EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not make the onscreen context current" << std::endl;
    return false;
  }

  return true;
}

// |flutter::FlutterApplication::RenderDelegate|
bool WaylandDisplay::OnApplicationContextClearCurrent() {
  if (!valid_) {
    FLWAY_ERROR << "Invalid display." << std::endl;
    return false;
  }

  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not clear the context." << std::endl;
    return false;
  }

  return true;
}

// |flutter::FlutterApplication::RenderDelegate|
bool WaylandDisplay::OnApplicationPresent() {
  if (!valid_) {
    FLWAY_ERROR << "Invalid display." << std::endl;
    return false;
  }

  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    LogLastEGLError();
    FLWAY_ERROR << "Could not swap the EGL buffer." << std::endl;
    return false;
  }

  return true;
}

// |flutter::FlutterApplication::RenderDelegate|
uint32_t WaylandDisplay::OnApplicationGetOnscreenFBO() {
  if (!valid_) {
    FLWAY_ERROR << "Invalid display." << std::endl;
    return 999;
  }

  return 0;  // FBO0
}

}  // namespace flutter
