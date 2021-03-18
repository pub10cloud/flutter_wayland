// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_WAYLAND_EVENT_LOOP_H_
#define FLUTTER_SHELL_PLATFORM_WAYLAND_EVENT_LOOP_H_

#include "event_loop.h"
#include "wayland_display.h"

namespace flutter {

// An event loop implementation that supports Flutter Engine tasks scheduling in
// the GLFW event loop.
class WayLandEventLoop : public EventLoop {
 public:
  WayLandEventLoop(std::thread::id main_thread_id,
                  const TaskExpiredCallback& on_task_expired);

  WayLandEventLoop(std::thread::id main_thread_id,
                  const TaskExpiredCallback& on_task_expired,
                  WaylandDisplay* display);

  virtual ~WayLandEventLoop();

  // Prevent copying.
  WayLandEventLoop(const WayLandEventLoop&) = delete;
  WayLandEventLoop& operator=(const WayLandEventLoop&) = delete;

 private:
  // EventLoop
  void WaitUntil(const TaskTimePoint& time) override;
  void Wake() override;
  void WayLandWakeUp();
  void WayLandPollEvents();
  void WayLandWaitEventsTimeout(double timeout);
  int wakeup_fd[2] ;
  WaylandDisplay* display_;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_WAYLAND_EVENT_LOOP_H_
