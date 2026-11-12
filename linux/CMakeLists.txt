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
option(FL_LINUX_BUILD_CLIENT "Build freelancer_linux_client bring-up target (requires FL_LINUX_BUILD_TOOLS; full M8.1+ path requires 32-bit)" ON)

include(CTest)

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

  # M4 window/input abstraction (SDL3, Wayland-first) and harness.
  if(FL_SDL3_USABLE)
    add_library(fl_linux_port_window STATIC src/window_input.cpp)
    target_link_libraries(fl_linux_port_window PUBLIC fl_linux_port_platform ${FL_SDL3_PC_LIBRARIES})
    target_include_directories(fl_linux_port_window PUBLIC ${FL_LINUX_INCLUDE_DIR} PRIVATE ${FL_SDL3_PC_INCLUDE_DIRS})
    target_link_directories(fl_linux_port_window PUBLIC ${FL_SDL3_PC_LIBRARY_DIRS})
    target_compile_definitions(fl_linux_port_window PUBLIC FL_HAS_SDL3=1)

    add_executable(m4_window_input_harness tests/m4_window_input_harness.cpp)
    target_link_libraries(m4_window_input_harness PRIVATE fl_linux_port_window fl_linux_port_platform)
  else()
    message(STATUS "m4_window_input_harness not built (requires usable 32-bit/target sdl3 libraries)")
  endif()

  # M6 gameplay bring-up (null audio + XDG config/registry + boot harness).
  add_library(fl_linux_port_gameplay STATIC src/audio_backend.cpp src/xdg_config.cpp)
  target_link_libraries(fl_linux_port_gameplay PUBLIC fl_linux_port_platform fl_linux_port_path)
  target_include_directories(fl_linux_port_gameplay PUBLIC ${FL_LINUX_INCLUDE_DIR})

  # M5 graphics bring-up (scoped Vulkan backend behind IRenderPipeline).
  if(FL_SDL3_USABLE AND FL_VULKAN_USABLE)
    add_library(fl_linux_port_render STATIC src/render_pipeline.cpp)
    target_link_libraries(fl_linux_port_render PUBLIC fl_linux_port_window fl_linux_port_platform ${FL_SDL3_PC_LIBRARIES} ${FL_VULKAN_PC_LIBRARIES})
    target_include_directories(fl_linux_port_render BEFORE PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      ${FL_LINUX_INCLUDE_DIR}
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
      PRIVATE ${FL_SDL3_PC_INCLUDE_DIRS} ${FL_VULKAN_PC_INCLUDE_DIRS})
    target_link_directories(fl_linux_port_render PUBLIC ${FL_SDL3_PC_LIBRARY_DIRS} ${FL_VULKAN_PC_LIBRARY_DIRS})
    target_compile_definitions(fl_linux_port_render PUBLIC FL_HAS_SDL3=1 FL_HAS_VULKAN=1)
    if(FL_LINUX_32BIT)
      target_compile_definitions(fl_linux_port_render PUBLIC FL_LINUX_32BIT=1)
    endif()

    add_executable(m5_graphics_harness tests/m5_graphics_harness.cpp)
    target_link_libraries(m5_graphics_harness PRIVATE fl_linux_port_render fl_linux_port_window fl_linux_port_platform ${FL_SDL3_PC_LIBRARIES} ${FL_VULKAN_PC_LIBRARIES})
    target_include_directories(m5_graphics_harness BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      ${FL_LINUX_INCLUDE_DIR}
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
      ${FL_SDL3_PC_INCLUDE_DIRS} ${FL_VULKAN_PC_INCLUDE_DIRS})
    target_link_directories(m5_graphics_harness PRIVATE ${FL_SDL3_PC_LIBRARY_DIRS} ${FL_VULKAN_PC_LIBRARY_DIRS})
    target_compile_definitions(m5_graphics_harness PRIVATE FL_HAS_SDL3=1 FL_HAS_VULKAN=1)
  else()
    message(STATUS "m5_graphics_harness not built (requires usable target sdl3 and vulkan libraries)")
  endif()

  # M6 boot harness (null audio + XDG + window + render).
  if(FL_SDL3_USABLE AND FL_VULKAN_USABLE)
    add_executable(m6_boot_harness tests/m6_boot_harness.cpp)
    target_link_libraries(m6_boot_harness PRIVATE
      fl_linux_port_gameplay fl_linux_port_render fl_linux_port_window
      fl_linux_port_platform fl_linux_port_path
      ${FL_SDL3_PC_LIBRARIES} ${FL_VULKAN_PC_LIBRARIES})
    target_include_directories(m6_boot_harness BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      ${FL_LINUX_INCLUDE_DIR}
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
      ${FL_SDL3_PC_INCLUDE_DIRS} ${FL_VULKAN_PC_INCLUDE_DIRS})
    target_compile_definitions(m6_boot_harness PRIVATE FL_HAS_SDL3=1 FL_HAS_VULKAN=1)
  else()
    message(STATUS "m6_boot_harness not built (requires usable target sdl3 and vulkan libraries)")
  endif()

  # M7 release candidate harness (final validation).
  if(FL_SDL3_USABLE AND FL_VULKAN_USABLE)
    add_executable(m7_release_harness tests/m7_release_harness.cpp)
    target_link_libraries(m7_release_harness PRIVATE
      fl_linux_port_gameplay fl_linux_port_render fl_linux_port_window
      fl_linux_port_platform fl_linux_port_path
      ${FL_SDL3_PC_LIBRARIES} ${FL_VULKAN_PC_LIBRARIES})
    target_include_directories(m7_release_harness BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      ${FL_LINUX_INCLUDE_DIR}
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
      ${FL_SDL3_PC_INCLUDE_DIRS} ${FL_VULKAN_PC_INCLUDE_DIRS})
    target_compile_definitions(m7_release_harness PRIVATE FL_HAS_SDL3=1 FL_HAS_VULKAN=1)
  else()
    message(STATUS "m7_release_harness not built (requires usable target sdl3 and vulkan libraries)")
  endif()

  # M8 — freelancer_linux_client bring-up target (reproducible CMake entry point).
  # Full bring-up path is 32-bit (upstream wrapper TUs / ABI/layout assumptions). Non-32-bit
  # builds get a fallback stub to keep x64 tools presets usable.
  if(FL_LINUX_BUILD_CLIENT)
    if(NOT FL_LINUX_32BIT)
      message(STATUS "freelancer_linux_client: building non-32-bit fallback stub (set -DFL_LINUX_32BIT=ON / preset linux-x86 for M8 bring-up client)")
      add_executable(freelancer_linux_client src/freelancer_client_stub.cpp)
      target_link_libraries(freelancer_linux_client PRIVATE fl_linux_port_platform fl_linux_port_path)
      target_include_directories(freelancer_linux_client PRIVATE ${FL_LINUX_INCLUDE_DIR})
    endif()
  else()
    message(STATUS "freelancer_linux_client: disabled (FL_LINUX_BUILD_CLIENT=OFF)")
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
    # Common config for guarded upstream wrapper TUs / consumers (32-bit x86 bring-up).
    add_library(fl_upstream_tu_iface INTERFACE)
    target_include_directories(fl_upstream_tu_iface BEFORE INTERFACE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      "${FL_LINUX_INCLUDE_DIR}"
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
    )
    target_compile_definitions(fl_upstream_tu_iface INTERFACE FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_32BIT=1)
    target_compile_options(fl_upstream_tu_iface INTERFACE -fvisibility=hidden -fvisibility-inlines-hidden)

    # Upstream source integration wrappers (guarded Linux harness TUs).
    # These target the original 32-bit/x86 upstream layout and ABI assumptions.
    add_library(fl_upstream_dacom_tu OBJECT tests/upstream_dacom_tu.cpp)
    target_link_libraries(fl_upstream_dacom_tu PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    add_library(fl_upstream_readfile_tu OBJECT tests/upstream_readfile_tu.cpp)
    target_link_libraries(fl_upstream_readfile_tu PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    add_executable(upstream_dacom_readfile_harness
      tests/upstream_dacom_readfile_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
    )
    target_link_libraries(upstream_dacom_readfile_harness PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    add_executable(upstream_dacom_integration_harness
      tests/upstream_dacom_integration_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
    )
    target_link_libraries(upstream_dacom_integration_harness PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    add_executable(upstream_dacom_readfile_integration_harness
      tests/upstream_dacom_readfile_integration_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
    )
    target_link_libraries(upstream_dacom_readfile_integration_harness PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    add_executable(m2_engine_init_harness
      tests/m2_engine_init_harness.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
    )
    target_link_libraries(m2_engine_init_harness PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    add_library(fl_upstream_x86math_tu OBJECT tests/upstream_x86math_tu.cpp)
    target_link_libraries(fl_upstream_x86math_tu PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    # M8.2 integration gate prep: leaf upstream libs (no Win32 API deps)
    add_library(fl_upstream_zlib STATIC
      "${FL_SRC_ROOT}/zlib/vendor/adler32.c"
      "${FL_SRC_ROOT}/zlib/vendor/compress.c"
      "${FL_SRC_ROOT}/zlib/vendor/crc32.c"
      "${FL_SRC_ROOT}/zlib/vendor/deflate.c"
      "${FL_SRC_ROOT}/zlib/vendor/gzclose.c"
      "${FL_SRC_ROOT}/zlib/vendor/gzlib.c"
      "${FL_SRC_ROOT}/zlib/vendor/gzread.c"
      "${FL_SRC_ROOT}/zlib/vendor/gzwrite.c"
      "${FL_SRC_ROOT}/zlib/vendor/inffast.c"
      "${FL_SRC_ROOT}/zlib/vendor/inflate.c"
      "${FL_SRC_ROOT}/zlib/vendor/inftrees.c"
      "${FL_SRC_ROOT}/zlib/vendor/trees.c"
      "${FL_SRC_ROOT}/zlib/vendor/uncompr.c"
      "${FL_SRC_ROOT}/zlib/vendor/zutil.c"
      "${FL_SRC_ROOT}/zlib/vendor/minizip/ioapi.c"
      "${FL_SRC_ROOT}/zlib/vendor/minizip/unzip.c"
      "${FL_SRC_ROOT}/zlib/vendor/minizip/zip.c"
      "${FL_SRC_ROOT}/zlib/zlib_version_compat.c"
    )
    target_include_directories(fl_upstream_zlib PRIVATE
      "${FL_SRC_ROOT}/zlib/vendor"
      "${FL_SRC_ROOT}/zlib/vendor/minizip"
    )
    target_compile_definitions(fl_upstream_zlib PRIVATE ZLIB_WINAPI _CRT_NONSTDC_NO_DEPRECATE _LARGEFILE64_SOURCE=1)
    target_compile_options(fl_upstream_zlib PRIVATE -w -Wno-implicit-function-declaration)

    add_library(fl_upstream_qhull STATIC
      "${FL_SRC_ROOT}/qhull/vendor/geom.c"
      "${FL_SRC_ROOT}/qhull/vendor/geom2.c"
      "${FL_SRC_ROOT}/qhull/vendor/global.c"
      "${FL_SRC_ROOT}/qhull/vendor/io.c"
      "${FL_SRC_ROOT}/qhull/vendor/mem.c"
      "${FL_SRC_ROOT}/qhull/vendor/merge.c"
      "${FL_SRC_ROOT}/qhull/vendor/poly.c"
      "${FL_SRC_ROOT}/qhull/vendor/poly2.c"
      "${FL_SRC_ROOT}/qhull/vendor/qhull.c"
      "${FL_SRC_ROOT}/qhull/vendor/qset.c"
      "${FL_SRC_ROOT}/qhull/vendor/stat.c"
      "${FL_SRC_ROOT}/qhull/vendor/user.c"
    )
    target_include_directories(fl_upstream_qhull PRIVATE "${FL_SRC_ROOT}/qhull/vendor")
    target_compile_definitions(fl_upstream_qhull PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    target_compile_options(fl_upstream_qhull PRIVATE -w)


    add_executable(m3_math_data_tests
      tests/m3_math_data_tests.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
      $<TARGET_OBJECTS:fl_upstream_x86math_tu>
    )
    target_link_libraries(m3_math_data_tests PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    # M8.1–M8.5 — Linux client bring-up executable (32-bit): entry + stubs + upstream
    # DACOM/ReadFile/x86math wrapper TUs. Full upstream src/freelancer/DALib/Common engine
    # TU linkage is not yet complete; M8.2 makes incremental progress by exercising
    # DALib OpenData/CloseData code paths via a minimal Linux bring-up implementation.
    if(FL_LINUX_BUILD_CLIENT)
      add_library(fl_dalib_core_bringup OBJECT src/dalib_core_bringup.cpp)
      target_link_libraries(fl_dalib_core_bringup PRIVATE fl_linux_port_platform fl_upstream_tu_iface)

      add_executable(freelancer_linux_client
        src/linux_client_main.cpp
        src/linux_client_stubs.cpp
        $<TARGET_OBJECTS:fl_dalib_core_bringup>
        $<TARGET_OBJECTS:fl_upstream_dacom_tu>
        $<TARGET_OBJECTS:fl_upstream_readfile_tu>
        $<TARGET_OBJECTS:fl_upstream_x86math_tu>
      )
      target_link_libraries(freelancer_linux_client PRIVATE fl_linux_port_platform fl_linux_port_path fl_linux_port_gameplay fl_upstream_tu_iface)
      # M8.3 window/input/render hookups (optional when 32-bit SDL3/Vulkan libs unavailable)
      if(FL_SDL3_USABLE AND TARGET fl_linux_port_window)
        target_link_libraries(freelancer_linux_client PRIVATE fl_linux_port_window ${FL_SDL3_PC_LIBRARIES})
        target_include_directories(freelancer_linux_client PRIVATE ${FL_SDL3_PC_INCLUDE_DIRS})
        target_link_directories(freelancer_linux_client PRIVATE ${FL_SDL3_PC_LIBRARY_DIRS})
        target_compile_definitions(freelancer_linux_client PRIVATE FL_HAS_SDL3=1)
      else()
        message(STATUS "freelancer_linux_client: SDL3 window/input hooks unavailable (client will fail M8.3+ acceptance at runtime)")
      endif()
      if(FL_SDL3_USABLE AND FL_VULKAN_USABLE AND TARGET fl_linux_port_render)
        target_link_libraries(freelancer_linux_client PRIVATE fl_linux_port_render ${FL_VULKAN_PC_LIBRARIES})
        target_include_directories(freelancer_linux_client PRIVATE ${FL_VULKAN_PC_INCLUDE_DIRS})
        target_link_directories(freelancer_linux_client PRIVATE ${FL_VULKAN_PC_LIBRARY_DIRS})
        target_compile_definitions(freelancer_linux_client PRIVATE FL_HAS_VULKAN=1 FL_LINUX_32BIT=1)
      else()
        message(STATUS "freelancer_linux_client: Vulkan render hooks unavailable (client will fail M8.3+ acceptance at runtime)")
      endif()

      # Opt-in smoke test (requires display + 32-bit SDL3/Vulkan runtime/data files in repo checkout)
      if(BUILD_TESTING)
        add_test(NAME m85_client_smoke
          COMMAND "${CMAKE_CURRENT_LIST_DIR}/scripts/smoke_freelancer_client_m85.sh" "$<TARGET_FILE_DIR:freelancer_linux_client>"
        )
        set_tests_properties(m85_client_smoke PROPERTIES
          LABELS "m8;smoke;display-required"
          TIMEOUT 120
        )
      endif()
    endif()

  endif()
endif()

# Phase 0 engine-module targets will be added here as modules are made portable
# behind platform shims (e.g. dacom, ReadFile).
if(FL_LINUX_BUILD_PHASE0)
  message(WARNING "Phase 0 engine targets are not implemented yet. Add portable module targets under linux/ as bring-up progresses.")
endif()
