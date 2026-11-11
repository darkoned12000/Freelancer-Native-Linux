# 32-bit (i686) Linux toolchain for Freelancer native port bring-up.
# Usage:
#   cmake -S linux -B linux/build-x86 --toolchain linux/cmake/toolchain-linux-x86.cmake
#
# Requires multilib toolchain/runtime deps (compiler, libc, and later
# 32-bit Vulkan/Wayland/SDL/audio libs as needed).

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Prefer clang/gcc multilib targets when available; callers may override compilers.
if(NOT DEFINED CMAKE_C_COMPILER)
  find_program(CMAKE_C_COMPILER NAMES clang gcc)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
  find_program(CMAKE_CXX_COMPILER NAMES clang++ g++)
endif()

set(CMAKE_C_FLAGS_INIT "-m32 -msse2 -mfpmath=sse -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_INIT "-m32 -msse2 -mfpmath=sse -D_FILE_OFFSET_BITS=64")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-m32")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-m32")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-m32")

# Improve diagnostics for 32-bit portability issues.
set(FL_LINUX_PORT_WARNING_FLAGS "-Wall -Wextra -Wconversion -Wno-unused-parameter" CACHE STRING "Default warning flags for Linux port targets")
