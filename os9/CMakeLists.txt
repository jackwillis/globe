# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
cmake_minimum_required(VERSION 3.20)
project(globe_os9 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
# OS 9 is the floppy release; the root build produces the full X11 release.
set(APP PoodleEarth)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
foreach(kind atlas elevation landclasses countries places)
  if(kind STREQUAL countries)
    set(DATA countries.gpol)
    set(TYPE Countries)
  elseif(kind STREQUAL places)
    set(DATA cities.gplc)
    set(TYPE Places)
  elseif(kind STREQUAL atlas)
    set(DATA atlas.gatl)
    set(TYPE Atlas)
  elseif(kind STREQUAL landclasses)
    set(DATA landclasses-1deg.ggrd)
    set(TYPE Landclasses)
  else()
    set(DATA elevation-1deg.gelv)
    set(TYPE Elevation)
  endif()
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/embedded_${kind}.cpp
    COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/embed_atlas.py ${ROOT}/data/${DATA}
      ${CMAKE_BINARY_DIR}/embedded_${kind}.cpp ${TYPE}
    DEPENDS ${ROOT}/tools/embed_atlas.py ${ROOT}/data/${DATA})
endforeach()
# Floppy payload: atlas, one-degree elevation, land classes, country polygons and 304 cities.
add_application(${APP} CREATOR PdlE
  ${ROOT}/src/platform/os9/main.cpp ${ROOT}/src/platform/os9/clock_os9.cpp
  ${ROOT}/src/platform/os9/sidebar.cpp
  ${ROOT}/src/platform/os9/about_window.cpp
  ${ROOT}/src/platform/os9/prefs_window.cpp
  ${ROOT}/src/platform/os9/menu_bar.cpp
  ${ROOT}/src/platform/common/pixel_format.cpp
  ${ROOT}/src/core/geometry.cpp ${ROOT}/src/core/projection.cpp ${ROOT}/src/core/trackball.cpp
  ${ROOT}/src/core/app.cpp ${ROOT}/src/core/controls.cpp ${ROOT}/src/core/countries.cpp ${ROOT}/src/core/places.cpp ${ROOT}/src/core/strings.cpp
  ${ROOT}/src/data/grid_codec.cpp ${ROOT}/src/data/landclasses.cpp
  ${ROOT}/src/data/atlas.cpp ${ROOT}/src/data/elevation.cpp ${ROOT}/src/data/crc32.cpp
  ${ROOT}/src/data/countries.cpp ${ROOT}/src/data/places.cpp
  ${ROOT}/src/render/framebuffer.cpp ${ROOT}/src/render/raster.cpp
  ${ROOT}/src/render/render.cpp ${ROOT}/src/render/terrain.cpp ${ROOT}/src/render/places.cpp
  ${ROOT}/src/render/highlight.cpp ${ROOT}/src/render/daynight.cpp ${ROOT}/src/render/measure.cpp
  ${ROOT}/src/render/overlays.cpp ${ROOT}/src/render/zoom.cpp ${ROOT}/src/render/shown_frame.cpp ${ROOT}/src/render/refine_runner.cpp
  ${ROOT}/src/render/label_layout.cpp
  ${CMAKE_BINARY_DIR}/embedded_landclasses.cpp ${CMAKE_BINARY_DIR}/embedded_countries.cpp
  ${CMAKE_BINARY_DIR}/embedded_places.cpp
  ${CMAKE_BINARY_DIR}/embedded_atlas.cpp ${CMAKE_BINARY_DIR}/embedded_elevation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/globe.r)
target_compile_definitions(${APP} PRIVATE GLOBE_FLOPPY=1)
# Measurement builds show the settle timing in the window title; release builds keep "Poodle Earth".
option(GLOBE_TIMING_TITLE "Show render timing in the OS 9 window title" OFF)
if(GLOBE_TIMING_TITLE)
  target_compile_definitions(${APP} PRIVATE GLOBE_TIMING_TITLE=1)
endif()
target_include_directories(${APP} PRIVATE ${ROOT}/include ${ROOT}/src/platform/common)
target_compile_options(${APP} PRIVATE -Os -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti)
target_link_libraries(${APP} AppearanceLib)
