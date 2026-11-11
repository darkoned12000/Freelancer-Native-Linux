# 64-bit (x86_64) Linux toolchain for Freelancer native port bring-up.
# Usage:
#   cmake -S linux -B linux/build-x64 --toolchain linux/cmake/toolchain-linux-x86_64.cmake
#
# Provides a dedicated toolchain file (mirroring the i686 bring-up toolchain)
# so configure-time dependency checks run with explicit, consistent flags.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Prefer clang/gcc; callers may override compilers.
if(NOT DEFINED CMAKE_C_COMPILER)
  find_program(CMAKE_C_COMPILER NAMES clang gcc)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
  find_program(CMAKE_CXX_COMPILER NAMES clang++ g++)
endif()

# Native x86_64 build flags (no -m32). SSE2 is baseline on x86_64.
set(CMAKE_C_FLAGS_INIT "-msse2 -mfpmath=sse")
set(CMAKE_CXX_FLAGS_INIT "-msse2 -mfpmath=sse")
set(CMAKE_EXE_LINKER_FLAGS_INIT "")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "")

# Improve diagnostics for portability/ABI issues early.
set(FL_LINUX_PORT_WARNING_FLAGS "-Wall -Wextra -Wconversion -Wno-unused-parameter" CACHE STRING "Default warning flags for Linux port targets")
