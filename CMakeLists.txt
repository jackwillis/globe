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
  src/render/framebuffer.cpp)
target_include_directories(globe_core PUBLIC include)
target_compile_options(globe_core PUBLIC -fno-exceptions -fno-rtti)
target_compile_options(globe_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(GLOBE_SANITIZE)
  target_compile_options(globe_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(globe_core PUBLIC -fsanitize=address,undefined)
endif()

if(GLOBE_BUILD_TESTS)
  enable_testing()
  foreach(name bytes framebuffer projection)
    add_executable(test_${name} tests/test_${name}.cpp)
    target_link_libraries(test_${name} PRIVATE globe_core)
    target_compile_options(test_${name} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${name} COMMAND test_${name})
  endforeach()
endif()
