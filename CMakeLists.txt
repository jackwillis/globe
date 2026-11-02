# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
cmake_minimum_required(VERSION 3.20)
project(globe VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GLOBE_SANITIZE "Build with AddressSanitizer and UBSan (host only)" OFF)
option(GLOBE_BUILD_TESTS "Build the unit tests" ON)

# The core targets old, small systems: no exceptions, no RTTI, anywhere.
add_library(globe_core
  src/core/geometry.cpp
  src/core/projection.cpp
  src/core/trackball.cpp
  src/core/app.cpp
  src/core/trace.cpp
  src/data/atlas.cpp
  src/data/crc32.cpp
  src/render/framebuffer.cpp
  src/render/raster.cpp
  src/render/render.cpp)
target_include_directories(globe_core PUBLIC include)
target_compile_options(globe_core PUBLIC -fno-exceptions -fno-rtti)
target_compile_options(globe_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

# Platform adapter code that needs no OS headers, so it can be tested anywhere.
add_library(globe_pixel_format src/platform/x11/pixel_format.cpp)
target_include_directories(globe_pixel_format PUBLIC src/platform/x11)
target_link_libraries(globe_pixel_format PUBLIC globe_core)

add_library(globe_frame_stats src/platform/common/frame_stats.cpp src/platform/common/cli.cpp)
target_include_directories(globe_frame_stats PUBLIC src/platform/common)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/embedded_atlas.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/embed_atlas.py
            ${CMAKE_SOURCE_DIR}/data/atlas.gatl ${CMAKE_BINARY_DIR}/embedded_atlas.cpp
    DEPENDS ${CMAKE_SOURCE_DIR}/data/atlas.gatl ${CMAKE_SOURCE_DIR}/tools/embed_atlas.py
    COMMENT "Embedding data/atlas.gatl")
  add_library(globe_embedded_atlas ${CMAKE_BINARY_DIR}/embedded_atlas.cpp)
  target_link_libraries(globe_embedded_atlas PUBLIC globe_core)

  add_executable(globe-bench src/platform/bench/bench.cpp)
  target_link_libraries(globe-bench PRIVATE globe_core globe_embedded_atlas globe_frame_stats
                                            globe_pixel_format)
  target_compile_options(globe-bench PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(X11)
if(X11_FOUND AND TARGET globe_embedded_atlas)
  add_executable(globe-x11 src/platform/x11/main.cpp)
  target_include_directories(globe-x11 PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(globe-x11 PRIVATE globe_core globe_embedded_atlas globe_frame_stats
                                          globe_pixel_format ${X11_LIBRARIES})
  target_compile_options(globe-x11 PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(GLOBE_SANITIZE)
  target_compile_options(globe_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(globe_core PUBLIC -fsanitize=address,undefined)
endif()

if(GLOBE_BUILD_TESTS)
  enable_testing()
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    add_test(NAME atlas_builder COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_atlas_builder.py)
  endif()
  foreach(name bytes framebuffer projection trackball atlas render app)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE globe_core)
    target_compile_options(test_${name} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${name} COMMAND test_${name} ${CMAKE_SOURCE_DIR})
  endforeach()
  add_executable(test_pixel_format tests/test_pixel_format.cpp)
  target_link_libraries(test_pixel_format PRIVATE globe_pixel_format)
  target_compile_options(test_pixel_format PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME pixel_format COMMAND test_pixel_format)
  add_executable(test_frame_stats tests/test_frame_stats.cpp)
  target_link_libraries(test_frame_stats PRIVATE globe_frame_stats)
  target_compile_options(test_frame_stats PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME frame_stats COMMAND test_frame_stats)
  add_executable(test_cli tests/test_cli.cpp)
  target_link_libraries(test_cli PRIVATE globe_frame_stats)
  target_compile_options(test_cli PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME cli COMMAND test_cli)
  if(TARGET globe-x11)
    find_program(XVFB_RUN xvfb-run)
    if(XVFB_RUN)
      # Smoke tests against a private virtual X server at both common depths. trace1 includes a
      # resize, so the buffer-recreation path runs too.
      foreach(depth 24 16)
        add_test(NAME x11_smoke_${depth}
                 COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x${depth}" $<TARGET_FILE:globe-x11>
                         --size 160x120 --script ${CMAKE_SOURCE_DIR}/tests/data/trace1.txt
                         --exit-when-done --stats)
        set_tests_properties(x11_smoke_${depth} PROPERTIES PASS_REGULAR_EXPRESSION
                             "\"frames\": 1[0-9][0-9]"
                             TIMEOUT 120)
      endforeach()
      # Nothing may be drawn while the window sits idle.
      add_test(NAME x11_idle
               COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x24" $<TARGET_FILE:globe-x11>
                       --size 160x120 --script ${CMAKE_SOURCE_DIR}/tests/data/trace1.txt
                       --idle-seconds 2 --stats)
      set_tests_properties(x11_idle PROPERTIES PASS_REGULAR_EXPRESSION "\"idle_frames\": 0,"
                           TIMEOUT 120)
    endif()
  endif()
  if(TARGET globe-bench)
    add_test(NAME bench_smoke COMMAND globe-bench --repeat 1 --size 320x240
             ${CMAKE_SOURCE_DIR}/tests/data/bench_drag.txt)
  endif()
endif()
