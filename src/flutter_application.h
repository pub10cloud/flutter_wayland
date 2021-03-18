// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <flutter_embedder.h>

#include <functional>
#include <vector>

#include "macros.h"
#include "event_loop.h"

namespace flutter {

// Add For Touch Event
enum class EventPhase : int
{
    up,
    down,
    move,
    cancel,
};

class FlutterApplication {
 public:
  class RenderDelegate {
   public:
    virtual bool OnApplicationContextMakeCurrent() = 0;

    virtual bool OnApplicationContextClearCurrent() = 0;

    virtual bool OnApplicationPresent() = 0;

    virtual uint32_t OnApplicationGetOnscreenFBO() = 0;
  };

  FlutterApplication(std::string bundle_path,
                     const std::vector<std::string>& args,
                     RenderDelegate& render_delegate);

  ~FlutterApplication();
  std::unique_ptr<flutter::EventLoop> event_loop_;

  bool IsValid() const;

  void ProcessEvents();

  bool SetWindowSize(size_t width, size_t height);

  bool SendPointerEvent(int button, int x, int y);
  bool SendTouchEvent(EventPhase phase, int x, int y);

 private:
  bool valid_;
  RenderDelegate& render_delegate_;
  FlutterEngine engine_ = nullptr;
  int last_button_ = 0;

  bool SendFlutterPointerEvent(FlutterPointerPhase phase, double x, double y);
  bool SendFlutterTouchEvent(FlutterPointerPhase phase, double x, double y);

  FLWAY_DISALLOW_COPY_AND_ASSIGN(FlutterApplication);
};

}  // namespace flutter
