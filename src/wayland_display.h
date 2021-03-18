// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#include <memory>
#include <string>

#include "flutter_application.h"
#include "macros.h"

namespace flutter {

class WaylandDisplay : public FlutterApplication::RenderDelegate {
 public:
  WaylandDisplay(size_t width, size_t height);

  ~WaylandDisplay();

  bool IsValid() const;

  bool Run();
  wl_display* getDisplay();
  FlutterApplication* application = nullptr;

 private:
  static const wl_registry_listener kRegistryListener; 
  static const wl_shell_surface_listener kShellSurfaceListener;
  static const wl_pointer_listener kPointerListener; // Add For Poiter Event Handling
  static const wl_touch_listener kTouchListener; // Add For Touch Event Handling
  static const wl_keyboard_listener kKeyboardListener; // Add For Keyboard Event Handling
  static const wl_seat_listener kSeatListener; // Add For Seat Event Handling
  bool valid_ = false;
  const int screen_width_;
  const int screen_height_;
  wl_display* display_ = nullptr;
  wl_registry* registry_ = nullptr;
  wl_compositor* compositor_ = nullptr;
  wl_shell* shell_ = nullptr;
  wl_shell_surface* shell_surface_ = nullptr;
  wl_surface* surface_ = nullptr;

  wl_seat* seat_ = nullptr; // Add For Poiter Event Handling
  wl_pointer* pointer_ = nullptr; // Add For Pointer Event Handling
  wl_touch* touch_ = nullptr; // Add For Touch Event Handling
	wl_keyboard *keyboard_; // Add For Keyboard Event Handling

  wl_shm *shm_ = nullptr; // Add For Drawing Cursor
  wl_cursor_theme *cursor_theme_ = nullptr; // Add For Drawing Cursor
  wl_cursor *default_cursor_ = nullptr; // Add For Drawing Cursor
  wl_surface *cursor_surface_ = nullptr; // Add For Drawing Cursor

  wl_egl_window* window_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLSurface egl_surface_ = nullptr;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  
  double mouse_x_ = 0.0; // Add For Poiter Event Handling
  double mouse_y_ = 0.0; // Add For Poiter Event Handling
  double touch_x_ = 0.0; // Add For Touch Event Handling
  double touch_y_ = 0.0; // Add For Touch Event Handling
  int32_t touch_id_ = -1; // Add For Poiter Event Handling

  bool SetupEGL();

  void AnnounceRegistryInterface(struct wl_registry* wl_registry,
                                 uint32_t name,
                                 const char* interface,
                                 uint32_t version);

  void UnannounceRegistryInterface(struct wl_registry* wl_registry,
                                   uint32_t name);

  bool StopRunning();

  // |flutter::FlutterApplication::RenderDelegate|
  bool OnApplicationContextMakeCurrent() override;

  // |flutter::FlutterApplication::RenderDelegate|
  bool OnApplicationContextClearCurrent() override;

  // |flutter::FlutterApplication::RenderDelegate|
  bool OnApplicationPresent() override;

  // |flutter::FlutterApplication::RenderDelegate|
  uint32_t OnApplicationGetOnscreenFBO() override;

  FLWAY_DISALLOW_COPY_AND_ASSIGN(WaylandDisplay);
};

}  // namespace flutter
