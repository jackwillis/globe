# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Existing development-host cross toolchain only. No installation or target-side build is performed by this file.
foreach(variable GLOBE_NETBSD_TOOLS GLOBE_NETBSD_SYSROOT)
  if("$ENV{${variable}}" STREQUAL "")
    message(FATAL_ERROR "Set ${variable} to the existing development-host directory")
  endif()
endforeach()
set(CMAKE_SYSTEM_NAME NetBSD)
set(CMAKE_SYSTEM_PROCESSOR powerpc)
set(CMAKE_CXX_COMPILER "$ENV{GLOBE_NETBSD_TOOLS}/powerpc--netbsd-g++")
set(CMAKE_SYSROOT "$ENV{GLOBE_NETBSD_SYSROOT}")
set(CMAKE_CXX_FLAGS_INIT "-isystem \"$ENV{GLOBE_NETBSD_SYSROOT}/usr/include/g++\"")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
set(CMAKE_FIND_ROOT_PATH "$ENV{GLOBE_NETBSD_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
