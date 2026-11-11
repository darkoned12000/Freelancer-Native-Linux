cmake_minimum_required(VERSION 3.20)

# Experimental Linux bring-up build (Phase 0).
# This does NOT build the full game yet. Upstream remains MSVC/Win32.
project(FreelancerLinuxPort LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

option(FL_LINUX_32BIT "Attempt to build 32-bit (x86) targets with -m32 (requires multilib toolchain/libs)" OFF)
option(FL_LINUX_BUILD_PHASE0 "Build minimal Phase 0 experimental targets (when sources are ported)" OFF)
option(FL_LINUX_BUILD_TOOLS "Build Linux helper tools/prototypes (path resolver, harnesses)" ON)

if(FL_LINUX_32BIT)
  add_compile_options(-m32 -msse2 -mfpmath=sse)
  add_link_options(-m32)
endif()

if(DEFINED FL_LINUX_PORT_WARNING_FLAGS AND NOT FL_LINUX_PORT_WARNING_FLAGS STREQUAL "")
  separate_arguments(_fl_port_warn NATIVE_COMMAND "${FL_LINUX_PORT_WARNING_FLAGS}")
  add_compile_options(${_fl_port_warn})
endif()

find_package(Threads REQUIRED)

# Source root of the original project
set(FL_SRC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../src")
set(FL_LINUX_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

message(STATUS "Freelancer Linux port scaffolding")
message(STATUS "  FL_SRC_ROOT=${FL_SRC_ROOT}")
message(STATUS "  FL_LINUX_32BIT=${FL_LINUX_32BIT}")
message(STATUS "  FL_LINUX_BUILD_PHASE0=${FL_LINUX_BUILD_PHASE0}")
message(STATUS "  FL_LINUX_BUILD_TOOLS=${FL_LINUX_BUILD_TOOLS}")

# --- Phase 0 utilities/prototypes ---
add_library(fl_linux_port_path STATIC
  src/path_util.cpp
)
target_include_directories(fl_linux_port_path PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
)

add_library(fl_linux_port_platform STATIC
  src/platform.cpp
  src/module_loader.cpp
  src/win32_file_shim.cpp
)
target_include_directories(fl_linux_port_platform PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(fl_linux_port_platform PUBLIC fl_linux_port_path Threads::Threads ${CMAKE_DL_LIBS})

if(FL_LINUX_BUILD_TOOLS)
  add_executable(fl_path_resolve tests/path_resolve_main.cpp)
  target_link_libraries(fl_path_resolve PRIVATE fl_linux_port_path)

  add_executable(dacom_ini_probe tests/dacom_ini_probe.cpp)
  target_link_libraries(dacom_ini_probe PRIVATE fl_linux_port_path)

  add_executable(core_shim_harness tests/core_shim_harness.cpp)
  target_link_libraries(core_shim_harness PRIVATE fl_linux_port_platform fl_linux_port_path)

  add_executable(abi_module_harness tests/abi_module_harness.cpp)
  target_link_libraries(abi_module_harness PRIVATE fl_linux_port_platform fl_linux_port_path)

  add_executable(dacom_boot_harness tests/dacom_boot_harness.cpp)
  target_link_libraries(dacom_boot_harness PRIVATE fl_linux_port_platform fl_linux_port_path)

  add_executable(m1_platform_tests tests/m1_platform_tests.cpp)
  target_link_libraries(m1_platform_tests PRIVATE fl_linux_port_platform fl_linux_port_path)

  add_executable(dacom_upstream_compile_probe tests/dacom_upstream_compile_probe.cpp)
  target_link_libraries(dacom_upstream_compile_probe PRIVATE fl_linux_port_platform fl_linux_port_path)

  # #4 prereq probe (optional SDL3/Vulkan)
  find_package(PkgConfig QUIET)
  set(FL_SDL3_USABLE OFF)
  set(FL_VULKAN_USABLE OFF)
  if(PkgConfig_FOUND)
    pkg_check_modules(FL_SDL3_PC QUIET sdl3)
    pkg_check_modules(FL_VULKAN_PC QUIET vulkan)
  endif()

  if(FL_LINUX_32BIT)
    # Avoid linking 64-bit dev libs in 32-bit builds when 32-bit runtime libs are absent.
    if(EXISTS "/usr/lib32/libSDL3.so" OR EXISTS "/usr/lib32/libSDL3.so.0")
      set(FL_SDL3_USABLE ${FL_SDL3_PC_FOUND})
    endif()
    if(EXISTS "/usr/lib32/libvulkan.so" OR EXISTS "/usr/lib32/libvulkan.so.1")
      set(FL_VULKAN_USABLE ${FL_VULKAN_PC_FOUND})
    endif()
  else()
    set(FL_SDL3_USABLE ${FL_SDL3_PC_FOUND})
    set(FL_VULKAN_USABLE ${FL_VULKAN_PC_FOUND})
  endif()

  add_executable(gfx_env_probe tests/gfx_env_probe.cpp)
  target_link_libraries(gfx_env_probe PRIVATE fl_linux_port_platform)
  if(FL_SDL3_USABLE)
    target_compile_definitions(gfx_env_probe PRIVATE FL_HAS_SDL3=1)
    target_include_directories(gfx_env_probe PRIVATE ${FL_SDL3_PC_INCLUDE_DIRS})
    target_link_directories(gfx_env_probe PRIVATE ${FL_SDL3_PC_LIBRARY_DIRS})
    target_link_libraries(gfx_env_probe PRIVATE ${FL_SDL3_PC_LIBRARIES})
  endif()
  if(FL_VULKAN_USABLE)
    target_compile_definitions(gfx_env_probe PRIVATE FL_HAS_VULKAN=1)
    target_include_directories(gfx_env_probe PRIVATE ${FL_VULKAN_PC_INCLUDE_DIRS})
    target_link_directories(gfx_env_probe PRIVATE ${FL_VULKAN_PC_LIBRARY_DIRS})
    target_link_libraries(gfx_env_probe PRIVATE ${FL_VULKAN_PC_LIBRARIES})
  endif()

  # Standalone #4 Wayland/SDL3/Vulkan presentation PoC (scaffold; requires deps)
  if(FL_SDL3_USABLE AND FL_VULKAN_USABLE)
    add_executable(gfx_wayland_poc tests/gfx_wayland_poc.cpp)
    target_link_libraries(gfx_wayland_poc PRIVATE fl_linux_port_platform ${FL_SDL3_PC_LIBRARIES} ${FL_VULKAN_PC_LIBRARIES})
    target_include_directories(gfx_wayland_poc PRIVATE ${FL_SDL3_PC_INCLUDE_DIRS} ${FL_VULKAN_PC_INCLUDE_DIRS})
    target_link_directories(gfx_wayland_poc PRIVATE ${FL_SDL3_PC_LIBRARY_DIRS} ${FL_VULKAN_PC_LIBRARY_DIRS})
    target_compile_definitions(gfx_wayland_poc PRIVATE FL_HAS_SDL3=1 FL_HAS_VULKAN=1)
  else()
    message(STATUS "gfx_wayland_poc not built (requires usable 32-bit/target sdl3 and vulkan libraries)")
  endif()

  if(FL_LINUX_32BIT)
    # Upstream source integration wrappers (guarded Linux harness TUs).
    # These target the original 32-bit/x86 upstream layout and ABI assumptions.
    add_library(fl_upstream_dacom_tu OBJECT tests/upstream_dacom_tu.cpp)
    target_link_libraries(fl_upstream_dacom_tu PRIVATE fl_linux_port_platform fl_linux_port_path)
    target_include_directories(fl_upstream_dacom_tu BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      "${FL_LINUX_INCLUDE_DIR}"
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
    )
    target_compile_definitions(fl_upstream_dacom_tu PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1)

    add_library(fl_upstream_readfile_tu OBJECT tests/upstream_readfile_tu.cpp)
    target_link_libraries(fl_upstream_readfile_tu PRIVATE fl_linux_port_platform fl_linux_port_path)
    target_include_directories(fl_upstream_readfile_tu BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      "${FL_LINUX_INCLUDE_DIR}"
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
    )
    target_compile_definitions(fl_upstream_readfile_tu PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1)

    add_executable(upstream_dacom_readfile_harness
      tests/upstream_dacom_readfile_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
    )
    target_link_libraries(upstream_dacom_readfile_harness PRIVATE fl_linux_port_platform fl_linux_port_path)
    target_include_directories(upstream_dacom_readfile_harness PRIVATE "${FL_LINUX_INCLUDE_DIR}")

    add_executable(upstream_dacom_integration_harness
      tests/upstream_dacom_integration_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
    )
    target_link_libraries(upstream_dacom_integration_harness PRIVATE fl_linux_port_platform fl_linux_port_path)
    target_include_directories(upstream_dacom_integration_harness PRIVATE
      "${FL_LINUX_INCLUDE_DIR}"
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
    )
    target_compile_definitions(upstream_dacom_integration_harness PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1)

    add_executable(upstream_dacom_readfile_integration_harness
      tests/upstream_dacom_readfile_integration_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
    )
    target_link_libraries(upstream_dacom_readfile_integration_harness PRIVATE fl_linux_port_platform fl_linux_port_path)
    target_include_directories(upstream_dacom_readfile_integration_harness BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      "${FL_LINUX_INCLUDE_DIR}"
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
    )
    target_compile_definitions(upstream_dacom_readfile_integration_harness PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1)

    add_executable(m2_engine_init_harness
      tests/m2_engine_init_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
    )
    target_link_libraries(m2_engine_init_harness PRIVATE fl_linux_port_platform fl_linux_port_path)
    target_include_directories(m2_engine_init_harness BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      "${FL_LINUX_INCLUDE_DIR}"
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
    )
    target_compile_definitions(m2_engine_init_harness PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1)
  endif()
endif()

# Phase 0 engine-module targets will be added here as modules are made portable
# behind platform shims (e.g. dacom, ReadFile).
if(FL_LINUX_BUILD_PHASE0)
  message(WARNING "Phase 0 engine targets are not implemented yet. Add portable module targets under linux/ as bring-up progresses.")
endif()
