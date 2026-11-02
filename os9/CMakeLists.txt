# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
cmake_minimum_required(VERSION 3.20)
project(globe_os9 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
option(GLOBE_FLOPPY "Designated app + atlas + one-degree elevation edition" OFF)
if(GLOBE_FLOPPY)
  set(APP GlobeFloppy)
else()
  set(APP Globe)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
foreach(kind atlas elevation)
  if(kind STREQUAL atlas)
    set(DATA atlas.gatl)
    set(TYPE Atlas)
  else()
    set(DATA elevation-1deg.gelv)
    set(TYPE Elevation)
  endif()
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/embedded_${kind}.cpp
    COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/embed_atlas.py ${ROOT}/data/${DATA}
      ${CMAKE_BINARY_DIR}/embedded_${kind}.cpp ${TYPE}
    DEPENDS ${ROOT}/tools/embed_atlas.py ${ROOT}/data/${DATA})
endforeach()
# Explicitly minimal proof-of-life payload: optional polygons and places are excluded.
add_application(${APP} CREATOR Glb2
  ${ROOT}/src/platform/os9/main.cpp ${ROOT}/src/platform/os9/clock_os9.cpp
  ${ROOT}/src/platform/common/pixel_format.cpp
  ${ROOT}/src/core/geometry.cpp ${ROOT}/src/core/projection.cpp ${ROOT}/src/core/trackball.cpp
  ${ROOT}/src/core/app.cpp
  ${ROOT}/src/data/atlas.cpp ${ROOT}/src/data/elevation.cpp ${ROOT}/src/data/crc32.cpp
  ${ROOT}/src/render/framebuffer.cpp ${ROOT}/src/render/raster.cpp
  ${ROOT}/src/render/render.cpp ${ROOT}/src/render/terrain.cpp
  ${CMAKE_BINARY_DIR}/embedded_atlas.cpp ${CMAKE_BINARY_DIR}/embedded_elevation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/globe.r)
target_include_directories(${APP} PRIVATE ${ROOT}/include ${ROOT}/src/platform/common)
target_compile_options(${APP} PRIVATE -Os -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti)
target_link_libraries(${APP} AppearanceLib)
