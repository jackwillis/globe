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
  src/core/countries.cpp
  src/core/places.cpp
  src/core/trace.cpp
  src/data/elevation.cpp
  src/data/countries.cpp
  src/data/places.cpp
  src/data/atlas.cpp
  src/data/crc32.cpp
  src/render/framebuffer.cpp
  src/render/raster.cpp
  src/render/render.cpp
  src/render/terrain.cpp
  src/render/places.cpp)
target_include_directories(globe_core PUBLIC include)
target_compile_options(globe_core PUBLIC -fno-exceptions -fno-rtti)
target_compile_options(globe_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

# Platform-neutral helpers shared by every front end (pixel layout conversion, timing summaries,
# command-line parsing). No OS headers, so they are tested anywhere and reusable for Mac OS 9.
add_library(globe_platform_common
  src/platform/common/elevation_packs.cpp
  src/platform/common/pixel_format.cpp
  src/platform/common/frame_stats.cpp
  src/platform/common/cli.cpp)
target_include_directories(globe_platform_common PUBLIC src/platform/common)
target_link_libraries(globe_platform_common PUBLIC globe_core)

# The one platform-specific piece of the common code: each platform supplies its own clock.
add_library(globe_clock_posix src/platform/common/clock_posix.cpp)
target_include_directories(globe_clock_posix PUBLIC src/platform/common)

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

  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/embedded_elevation.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/embed_atlas.py
            ${CMAKE_SOURCE_DIR}/data/elevation-1deg.gelv ${CMAKE_BINARY_DIR}/embedded_elevation.cpp Elevation
    DEPENDS ${CMAKE_SOURCE_DIR}/data/elevation-1deg.gelv ${CMAKE_SOURCE_DIR}/tools/embed_atlas.py)
  add_library(globe_embedded_elevation ${CMAKE_BINARY_DIR}/embedded_elevation.cpp)
  target_link_libraries(globe_embedded_elevation PUBLIC globe_core)

  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/embedded_countries.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/embed_atlas.py
      ${CMAKE_SOURCE_DIR}/data/countries.gpol ${CMAKE_BINARY_DIR}/embedded_countries.cpp Countries
    DEPENDS data/countries.gpol tools/embed_atlas.py)
  add_library(globe_embedded_countries ${CMAKE_BINARY_DIR}/embedded_countries.cpp)
  target_link_libraries(globe_embedded_countries PUBLIC globe_core)

  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/embedded_places.cpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/embed_atlas.py
      ${CMAKE_SOURCE_DIR}/data/cities.gplc ${CMAKE_BINARY_DIR}/embedded_places.cpp Places
    DEPENDS data/cities.gplc tools/embed_atlas.py)
  add_library(globe_embedded_places ${CMAKE_BINARY_DIR}/embedded_places.cpp)
  target_link_libraries(globe_embedded_places PUBLIC globe_core)

  add_executable(globe-bench src/platform/bench/bench.cpp)
  target_link_libraries(globe-bench PRIVATE globe_embedded_atlas globe_embedded_elevation globe_platform_common
                                            globe_clock_posix)
  target_compile_options(globe-bench PRIVATE -Wall -Wextra -Wpedantic)

  add_executable(globe-moving-base-bench src/platform/bench/moving_base_bench.cpp)
  target_include_directories(globe-moving-base-bench PRIVATE src/platform/common)
  target_link_libraries(globe-moving-base-bench PRIVATE globe_embedded_elevation globe_clock_posix)
  target_compile_options(globe-moving-base-bench PRIVATE -Wall -Wextra -Wpedantic)

  add_executable(globe-refine-profile src/platform/bench/refine_profile.cpp)
  target_include_directories(globe-refine-profile PRIVATE src/platform/common)
  target_link_libraries(globe-refine-profile PRIVATE globe_embedded_atlas globe_embedded_elevation
                                                      globe_clock_posix)
  target_compile_options(globe-refine-profile PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(X11)
if(X11_FOUND AND TARGET globe_embedded_atlas)
  add_executable(globe-x11 src/platform/x11/main.cpp src/platform/x11/info_window.cpp)
  target_include_directories(globe-x11 PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(globe-x11 PRIVATE globe_embedded_places globe_embedded_countries globe_embedded_atlas globe_embedded_elevation globe_platform_common
                                          globe_clock_posix ${X11_LIBRARIES})
  target_compile_options(globe-x11 PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(GLOBE_SANITIZE)
  target_compile_options(globe_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(globe_core PUBLIC -fsanitize=address,undefined)
endif()

if(GLOBE_BUILD_TESTS)
  enable_testing()
  if(Python3_FOUND)
    add_test(NAME compression_bench COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_compression_bench.py)
    add_test(NAME floppy_report COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_floppy_report.py)
    add_test(NAME place_builder COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_place_builder.py)
    add_test(NAME country_builder COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_country_builder.py)
    add_test(NAME elevation_builder COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_elevation_builder.py)
    add_test(NAME atlas_builder
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_atlas_builder.py)
  endif()
  foreach(name bytes framebuffer projection trackball atlas render app elevation terrain toolbar countries places shaded)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE globe_core)
    target_compile_options(test_${name} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${name} COMMAND test_${name} ${CMAKE_SOURCE_DIR})
    set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  endforeach()
  # Tests of the platform-neutral helpers (frame_stats also needs the POSIX clock).
  foreach(name pixel_format frame_stats cli elevation_packs classic_wait)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE globe_platform_common globe_clock_posix)
    target_compile_options(test_${name} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${name} COMMAND test_${name} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
  endforeach()
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
      foreach(depth 24 16)
        add_test(NAME x11_pixels_${depth}
                 COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x${depth}"
                         ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_x11_pixels.py ${depth}
                         $<TARGET_FILE:globe-x11> --size 161x121
                         --script ${CMAKE_SOURCE_DIR}/tests/data/trace1.txt
                         --exit-when-done --verify-pixels --stats)
        set_tests_properties(x11_pixels_${depth} PROPERTIES TIMEOUT 120)
      endforeach()
      add_executable(test_x11_toolbar tests/test_x11_toolbar.cpp)
      target_include_directories(test_x11_toolbar PRIVATE ${X11_INCLUDE_DIR})
      target_link_libraries(test_x11_toolbar PRIVATE globe_core ${X11_LIBRARIES})
      add_test(NAME x11_toolbar COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x24"
               $<TARGET_FILE:test_x11_toolbar> $<TARGET_FILE:globe-x11>)
      set_tests_properties(x11_toolbar PROPERTIES TIMEOUT 20)
      add_test(NAME x11_repaint COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x24"
               $<TARGET_FILE:test_x11_toolbar> $<TARGET_FILE:globe-x11> repaint)
      set_tests_properties(x11_repaint PROPERTIES TIMEOUT 20)
      # A click and window exposes resend pixels; they never render or restart refinement.
      add_test(NAME x11_click
               COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x24"
                       ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_x11_click.py
                       $<TARGET_FILE:globe-x11>
                       --size 160x120 --script ${CMAKE_SOURCE_DIR}/tests/data/click_expose.txt
                       --exit-when-done --verify-pixels --stats)
      set_tests_properties(x11_click PROPERTIES TIMEOUT 60)
      # Nothing may be drawn while the window sits idle.
      add_test(NAME x11_idle
               COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x24"
                       ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_x11_wakes.py
                       $<TARGET_FILE:globe-x11>
                       --size 160x120 --script ${CMAKE_SOURCE_DIR}/tests/data/trace1.txt
                       --idle-seconds 2 --verify-pixels --stats)
      set_tests_properties(x11_idle PROPERTIES TIMEOUT 120)
    endif()
  endif()
  if(TARGET globe-bench)
    add_test(NAME bench_refine COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_refine.py
             $<TARGET_FILE:globe-bench> --refine --repeat 2 ${CMAKE_SOURCE_DIR}/tests/data/bench_drag.txt)
    add_test(NAME bench_smoke COMMAND globe-bench --repeat 1 --size 320x240
             ${CMAKE_SOURCE_DIR}/tests/data/bench_drag.txt)
  endif()
endif()
