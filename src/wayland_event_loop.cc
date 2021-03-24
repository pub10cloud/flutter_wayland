// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wayland_event_loop.h"
#include <wayland-client.h>
#include "macros.h"
#include <atomic>
#include <utility>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

namespace flutter {

#define OPEN_FLAGS O_RDWR|O_CREAT
#define OPEN_MODE  00777
#define WAYLAND_WAKEUP "wayland_wakeup"

WayLandEventLoop::WayLandEventLoop(std::thread::id main_thread_id,
                             const TaskExpiredCallback& on_task_expired)
  : EventLoop(main_thread_id, std::move(on_task_expired)) {}

WayLandEventLoop::WayLandEventLoop(std::thread::id main_thread_id,
                             const TaskExpiredCallback& on_task_expired,
                             WaylandDisplay* display)
  : EventLoop(main_thread_id, std::move(on_task_expired)) {
    display_ = display;
    if (pipe(wakeup_fd) == -1) {
      FLWAY_ERROR << "pipe create failed" << std::endl; 
    }
}

WayLandEventLoop::~WayLandEventLoop() {
    close(wakeup_fd[0]);
    close(wakeup_fd[1]);
}

void WayLandEventLoop::WaitUntil(const TaskTimePoint& time) {
  const auto now = TaskTimePoint::clock::now();

  // Make sure the seconds are not integral.
  using Seconds = std::chrono::duration<double, std::ratio<1>>;
  const auto duration_to_wait = std::chrono::duration_cast<Seconds>(time - now);

  // Avoid engine task priority inversion by making sure WayLand events are
  // always processed even when there is no need to wait for pending engine
  // tasks.
  WayLandWaitEventsTimeout(duration_to_wait.count());
}

void WayLandEventLoop::Wake() {
  WayLandWakeUp();
}

void WayLandEventLoop::WayLandWaitEventsTimeout(double timeout) {
  struct pollfd pollfd[2] = {0};
  int i, ret, count = 0;
  uint32_t event = 0;
  uint32_t wake_event = 0;
  unsigned display_fd = wl_display_get_fd(display_->getDisplay());
  pollfd[0].fd = display_fd;
  pollfd[0].events = POLLIN | POLLERR | POLLHUP;
  pollfd[1].fd = wakeup_fd[0];
  pollfd[1].events = POLLIN ;
  char r_buf[12] = {0};

  std::chrono::duration<double> fs(timeout);
  std::chrono::microseconds mTimeout = std::chrono::duration_cast<std::chrono::microseconds>(fs);

  while (display_->IsValid()) {
    wl_display_dispatch_pending(display_->getDisplay());
    ret = wl_display_flush(display_->getDisplay());
    if (ret < 0 && errno != EAGAIN) {
      break;
    }

    count = poll(pollfd, 2, timeout == 0 ? -1 : mTimeout.count());
    if (count < 0 && errno != EINTR) {
      FLWAY_ERROR << "poll returned an error." << errno;
      break;
    }

    if (count >= 1) {
      wake_event = pollfd[1].revents;
      if (wake_event & POLLIN) {
        ret = read(wakeup_fd[0], r_buf, sizeof(r_buf));
        if (-1 == ret) {
          FLWAY_ERROR << "poll read failed: " << std::endl;
          return;
        }
        // break;
      }
      event = pollfd[0].revents;

      // We can have cases where POLLIN and POLLHUP are both set for
      // example. Don't break if both flags are set.
      if ((event & POLLERR || event & POLLHUP) && !(event & POLLIN)) {
        break;
      }

      if (event & POLLIN) {
        ret = wl_display_dispatch(display_->getDisplay());
        if (ret == -1) {
          FLWAY_ERROR << "wl_display_dispatch failed with an error." << errno;
        }
      }
      break;
    }
  }
  return ;
}


void WayLandEventLoop::WayLandWakeUp() {
  int ret = write(wakeup_fd[1], "wakeup", sizeof("wakeup"));
  // FLWAY_LOG << "Post an Wakeup Event"<< std::endl;
  if (-1 == ret) {
    FLWAY_ERROR << "write wakeup fd failed: " << std::endl;
  }
}

}  // namespace flutter
