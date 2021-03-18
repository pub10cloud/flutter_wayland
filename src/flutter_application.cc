// Copyright 2018 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_application.h"

#include <EGL/egl.h>
#include <sys/types.h>

#include <chrono>
#include <sstream>
#include <vector>

#include "utils.h"
#include "event_loop.h"
#include "wayland_event_loop.h"

namespace flutter {

static_assert(FLUTTER_ENGINE_VERSION == 1, "");

static const char* kICUDataFileName = "icudtl.dat";

static std::string GetICUDataPath() {
  auto exe_dir = GetExecutableDirectory();
  if (exe_dir == "") {
    return "";
  }
  std::stringstream stream;
  stream << exe_dir << kICUDataFileName;

  auto icu_path = stream.str();

  if (!FileExistsAtPath(icu_path.c_str())) {
    FLWAY_ERROR << "Could not find " << icu_path << std::endl;
    return "";
  }

  return icu_path;
}

// Populates |task_runner| with a description that uses |engine_state|'s event
// loop to run tasks.
static void ConfigurePlatformTaskRunner(
    FlutterTaskRunnerDescription* task_runner,
    FlutterApplication * state) {
  task_runner->struct_size = sizeof(FlutterTaskRunnerDescription);
  task_runner->user_data = state;
  task_runner->runs_task_on_current_thread_callback = [](void* state) -> bool {
    return reinterpret_cast<FlutterApplication*>(state)
      ->event_loop_->RunsTasksOnCurrentThread();
  };
  task_runner->post_task_callback =
      [](FlutterTask task, uint64_t target_time_nanos, void* state) -> void {
    reinterpret_cast<FlutterApplication*>(state)->event_loop_->PostTask(
        task, target_time_nanos);
  };
}

FlutterApplication::FlutterApplication(
    std::string bundle_path,
    const std::vector<std::string>& command_line_args,
    RenderDelegate& render_delegate)
    : render_delegate_(render_delegate) {
  if (!FlutterAssetBundleIsValid(bundle_path)) {
    FLWAY_ERROR << "Flutter asset bundle was not valid." << std::endl;
    return;
  }

  // Create an event loop for the window. It is not running yet.
  auto event_loop = std::make_unique<flutter::WayLandEventLoop>(
      std::this_thread::get_id(),  // main wayland thread
      [engine = &engine_](const auto* task) {
        if (FlutterEngineRunTask(*engine, task) != kSuccess) {
          FLWAY_ERROR << "Could not post an engine task." << std::endl;
        }
        // FLWAY_ERROR << "DEBUG:excute an engine task." << std::endl;
      }, reinterpret_cast<WaylandDisplay*>(&render_delegate));

  // Configure a task runner using the event loop.
  event_loop_ = std::move(event_loop);
  FlutterTaskRunnerDescription platform_task_runner = {};
  ConfigurePlatformTaskRunner(&platform_task_runner, this);
  FlutterCustomTaskRunners task_runners = {};
  task_runners.struct_size = sizeof(FlutterCustomTaskRunners);
  task_runners.platform_task_runner = &platform_task_runner;

  FlutterRendererConfig config = {};
  config.type = kOpenGL;
  config.open_gl.struct_size = sizeof(config.open_gl);
  config.open_gl.make_current = [](void* userdata) -> bool {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationContextMakeCurrent();
  };
  config.open_gl.clear_current = [](void* userdata) -> bool {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationContextClearCurrent();
  };
  config.open_gl.present = [](void* userdata) -> bool {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationPresent();
  };
  config.open_gl.fbo_callback = [](void* userdata) -> uint32_t {
    return reinterpret_cast<FlutterApplication*>(userdata)
        ->render_delegate_.OnApplicationGetOnscreenFBO();
  };
  config.open_gl.gl_proc_resolver = [](void* userdata,
                                       const char* name) -> void* {
    auto address = eglGetProcAddress(name);
    if (address != nullptr) {
      return reinterpret_cast<void*>(address);
    }
    FLWAY_ERROR << "Tried unsuccessfully to resolve: " << name << std::endl;
    return nullptr;
  };

  auto icu_data_path = GetICUDataPath();

  if (icu_data_path == "") {
    FLWAY_ERROR << "Could not find ICU data. It should be placed next to the "
                   "executable but it wasn't there."
                << std::endl;
    return;
  }

  std::vector<const char*> command_line_args_c;

  for (const auto& arg : command_line_args) {
    command_line_args_c.push_back(arg.c_str());
  }

  FlutterProjectArgs args = {
      .struct_size = sizeof(FlutterProjectArgs),
      .assets_path = bundle_path.c_str(),
      .main_path__unused__ = "",
      .packages_path__unused__ = "",
      .icu_data_path = icu_data_path.c_str(),
      .command_line_argc = static_cast<int>(command_line_args_c.size()),
      .command_line_argv = command_line_args_c.data(),
  };
  args.custom_task_runners = &task_runners;

  FlutterEngine engine = nullptr;
  auto result = FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, &args,
                                 this /* userdata */, &engine_);

  if (result != kSuccess || engine_ == nullptr) {
    FLWAY_ERROR << "Could not run the Flutter engine" << std::endl;
    return;
  }

  valid_ = true;
}

FlutterApplication::~FlutterApplication() {
  if (engine_ == nullptr) {
    return;
  }

  auto result = FlutterEngineShutdown(engine_);

  if (result != kSuccess) {
    FLWAY_ERROR << "Could not shutdown the Flutter engine." << std::endl;
  }
}

bool FlutterApplication::IsValid() const {
  return valid_;
}

bool FlutterApplication::SetWindowSize(size_t width, size_t height) {
  FlutterWindowMetricsEvent event = {};
  event.struct_size = sizeof(event);
  event.width = width;
  event.height = height;
  event.pixel_ratio = 1.0;
  return FlutterEngineSendWindowMetricsEvent(engine_, &event) == kSuccess;
}

void FlutterApplication::ProcessEvents() {
  __FlutterEngineFlushPendingTasksNow();
}

bool FlutterApplication::SendPointerEvent(int button, int x, int y) {
  if (!valid_) {
    FLWAY_ERROR << "Pointer events on an invalid application." << std::endl;
    return false;
  }

  // Simple hover event. Nothing to do.
  if (last_button_ == 0 && button == 0) {
    return true;
  }

  FlutterPointerPhase phase = kCancel;

  if (last_button_ == 0 && button != 0) {
    phase = kDown;
  } else if (last_button_ == button) {
    phase = kMove;
  } else {
    phase = kUp;
  }

  last_button_ = button;
  return SendFlutterPointerEvent(phase, x, y);
}

bool FlutterApplication::SendFlutterPointerEvent(FlutterPointerPhase phase,
                                                 double x,
                                                 double y) {
  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = x;
  event.y = y;
  event.device_kind = kFlutterPointerDeviceKindMouse;
  event.buttons = kFlutterPointerButtonMousePrimary;
  event.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  //FLWAY_LOG << "[M]" << phase << "," << x << "," << y << std::endl;
  return FlutterEngineSendPointerEvent(engine_, &event, 1) == kSuccess;
}

bool FlutterApplication::SendTouchEvent(EventPhase phase, int x, int y) {

  if (!valid_) {
    FLWAY_ERROR << "Touch events on an invalid application." << std::endl;
    return false;
  }

  FlutterPointerPhase fl_phase;

  switch(phase) {
    case EventPhase::up:
      fl_phase = kUp;
      break;
    case EventPhase::down:
      fl_phase = kDown;
      break;
    case EventPhase::move:
      fl_phase = kMove;
      break;
    case EventPhase::cancel:
      fl_phase = kCancel;
      break;
    default:
      fl_phase = kCancel;
      break;
  }

  return SendFlutterTouchEvent(fl_phase, x, y);
}

bool FlutterApplication::SendFlutterTouchEvent(FlutterPointerPhase phase,
                                                 double x,
                                                 double y) {
  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = x;
  event.y = y;
  event.device_kind = kFlutterPointerDeviceKindTouch;
  event.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  //FLWAY_LOG << "[T]" << phase << "," << x << "," << y << std::endl;
  return FlutterEngineSendPointerEvent(engine_, &event, 1) == kSuccess;
}

}  // namespace flutter
