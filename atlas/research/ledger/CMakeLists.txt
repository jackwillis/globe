# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Standalone primitive verification; deliberately does not modify Globe's shared build integration.
cmake_minimum_required(VERSION 3.20)
project(globe_memory_ledger_primitive LANGUAGES CXX)
get_filename_component(GLOBE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
option(LEDGER_SANITIZE "Run the host primitive test with ASan and UBSan" OFF)
add_executable(test_memory_ledger "${GLOBE_ROOT}/tests/test_memory_ledger.cpp")
target_include_directories(test_memory_ledger PRIVATE "${GLOBE_ROOT}/include")
target_compile_features(test_memory_ledger PRIVATE cxx_std_20)
target_compile_options(test_memory_ledger PRIVATE
  -fno-exceptions -fno-rtti -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
if(LEDGER_SANITIZE)
  target_compile_options(test_memory_ledger PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(test_memory_ledger PRIVATE -fsanitize=address,undefined)
endif()
enable_testing()
add_test(NAME memory_ledger_primitive COMMAND test_memory_ledger)
