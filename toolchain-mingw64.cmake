# CMake toolchain file for cross-compiling Windows 64-bit binaries
# using MinGW-w64 on Linux.
#
# Usage:
#   cmake -S <src> -B <build-win64> \
#       -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake \
#       -DCMAKE_BUILD_TYPE=Release

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use the posix threading variant for full std::thread support
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Search for headers/libraries in the MinGW sysroot
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# JUCE + MinGW: force static libstdc++/libgcc into binaries so the
# plugins don't depend on MinGW runtime DLLs being present on the
# target Windows machine.
add_link_options(-static-libgcc -static-libstdc++ -static)

# Avoid CMake trying to run cross-compiled test programs
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MinGW-w64's d2d1_3.h (needed by JUCE 8's Direct2D backend) guards the
# 1.3 interfaces behind NTDDI_VERSION >= NTDDI_WIN10_TH2/RS1, but JUCE only
# sets _WIN32_WINNT to base Win10 (which yields plain NTDDI_WIN10).
# Pin NTDDI_VERSION to Win10 RS5 so the D2D 1.3 declarations are visible.
add_compile_definitions(NTDDI_VERSION=0x0A000006)

# Force-include the MinGW compatibility header (fills small gaps in
# mingw-w64 headers vs. the Windows SDK: UIA attribute IDs, etc.)
add_compile_options(-include "${CMAKE_CURRENT_LIST_DIR}/mingw-compat.h")
