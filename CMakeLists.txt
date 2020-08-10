# Copyright 2018 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.5.2)

project(flutter_wayland)

set(FLUTTER_EXFILE_BASE "https://github.com/pub10cloud/flutter_wayland/releases/download/0.1/")

set(FLUTTER_ENGINE ${CMAKE_BINARY_DIR}/asset_bundle.tar.gz)
set(FLUTTER_ASSET  ${CMAKE_BINARY_DIR}/engine.tar.gz)

## Download and setup the Flutter Engine.
if(NOT EXISTS ${FLUTTER_ENGINE})
  file(DOWNLOAD
    ${FLUTTER_EXFILE_BASE}/engine.tar.gz
    ${FLUTTER_ENGINE}
    SHOW_PROGRESS
  )
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${FLUTTER_ENGINE}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  execute_process(
    COMMAND mv engine/* ./
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()

if(NOT EXISTS ${FLUTTER_ASSET})
  file(DOWNLOAD
    ${FLUTTER_EXFILE_BASE}/asset_bundle.tar.gz
    ${FLUTTER_ASSET}
    SHOW_PROGRESS
  )
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${FLUTTER_ASSET}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()

find_package(PkgConfig)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_CURSOR REQUIRED wayland-cursor)
pkg_check_modules(WAYLAND_EGL    REQUIRED wayland-egl)
pkg_check_modules(EGL            REQUIRED egl)

# Executable
file(GLOB_RECURSE FLUTTER_WAYLAND_SRC
  "src/*.cc"
  "src/*.h"
)

link_directories(${CMAKE_BINARY_DIR}/engine)

add_executable(flutter_wayland ${FLUTTER_WAYLAND_SRC})

target_link_libraries(flutter_wayland
  ${WAYLAND_CLIENT_LIBRARIES} 
  ${WAYLAND_CURSOR_LIBRARIES}
  ${WAYLAND_EGL_LIBRARIES}
  ${EGL_LIBRARIES}
  flutter_engine
)

target_include_directories(flutter_wayland
  PRIVATE
  ${WAYLAND_CLIENT_INCLUDE_DIRS}
  ${WAYLAND_EGL_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}/engine
)
