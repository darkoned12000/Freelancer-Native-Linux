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
  src/win32_user.cpp
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
  add_library(fl_linux_port_gameplay STATIC src/audio_backend.cpp src/pulse_audio_backend.cpp src/alsa_audio_backend.cpp src/xdg_config.cpp)
  target_link_libraries(fl_linux_port_gameplay PUBLIC fl_linux_port_platform fl_linux_port_path)
  target_include_directories(fl_linux_port_gameplay PUBLIC ${FL_LINUX_INCLUDE_DIR})
  if(FL_LINUX_32BIT)
    target_compile_definitions(fl_linux_port_gameplay PRIVATE FL_LINUX_32BIT=1)
  endif()

  # M5 graphics bring-up (scoped Vulkan backend behind IRenderPipeline).
  # M9.3: compile minimal GLSL shaders to SPIR-V at build time.
  if(FL_SDL3_USABLE AND FL_VULKAN_USABLE)
    find_program(GLSLC_EXECUTABLE glslc)
    if(GLSLC_EXECUTABLE)
      set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
      set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
      file(MAKE_DIRECTORY ${SHADER_BIN_DIR})
      foreach(sh triangle.vert triangle.frag skybox.vert skybox.frag text.vert text.frag nebula.vert nebula.frag)
        set(src ${SHADER_SRC_DIR}/${sh})
        set(out ${SHADER_BIN_DIR}/${sh}.spv)
        add_custom_command(OUTPUT ${out}
          COMMAND ${GLSLC_EXECUTABLE} ${src} -o ${out}
          DEPENDS ${src}
          COMMENT "Compiling ${sh} to SPIR-V")
        list(APPEND SHADER_SPVS ${out})
      endforeach()
      add_custom_target(fl_linux_shaders ALL DEPENDS ${SHADER_SPVS})
    endif()
    add_library(fl_linux_port_render STATIC src/render_pipeline.cpp src/vertex_buffer.cpp src/vb_adapter.cpp src/texture_manager.cpp src/texture_adapter.cpp src/font_manager.cpp)
    target_link_libraries(fl_linux_port_render PUBLIC fl_linux_port_window fl_linux_port_platform ${FL_SDL3_PC_LIBRARIES} ${FL_VULKAN_PC_LIBRARIES} freetype)
    target_include_directories(fl_linux_port_render BEFORE PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      ${FL_LINUX_INCLUDE_DIR}
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}"
      "/usr/include/freetype2"
      PRIVATE ${FL_SDL3_PC_INCLUDE_DIRS} ${FL_VULKAN_PC_INCLUDE_DIRS})
    target_link_directories(fl_linux_port_render PUBLIC ${FL_SDL3_PC_LIBRARY_DIRS} ${FL_VULKAN_PC_LIBRARY_DIRS})
    target_compile_definitions(fl_linux_port_render PUBLIC FL_HAS_SDL3=1 FL_HAS_VULKAN=1)
    if(FL_LINUX_32BIT)
      target_compile_definitions(fl_linux_port_render PUBLIC FL_LINUX_32BIT=1)
    endif()

    add_executable(m5_graphics_harness tests/m5_graphics_harness.cpp)
    add_executable(m9_render_harness tests/m9_render_harness.cpp)
    target_link_libraries(m9_render_harness PRIVATE fl_linux_port_render fl_linux_port_window fl_linux_port_platform ${FL_SDL3_PC_LIBRARIES} ${FL_VULKAN_PC_LIBRARIES})
    target_include_directories(m9_render_harness PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include/linux_port"
      ${FL_LINUX_INCLUDE_DIR}
      "${FL_SRC_ROOT}/include"
      "${FL_SRC_ROOT}")
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

    # M8.7 phase 2 — minimal upstream Common/EngBase/system prerequisites for early client init
    add_library(fl_upstream_system_tu OBJECT "${FL_SRC_ROOT}/system/system.cpp")
    target_link_libraries(fl_upstream_system_tu PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
    target_compile_definitions(fl_upstream_system_tu PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1)
    target_compile_options(fl_upstream_system_tu PRIVATE -fms-extensions -w)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fl_upstream_system_tu PRIVATE -fdeclspec)
    endif()

    add_library(fl_upstream_engbase_tu OBJECT
      "${FL_SRC_ROOT}/EngBase/EngBase.cpp"
      "${FL_SRC_ROOT}/EngBase/EngBaseMixer.cpp"
    )
    target_link_libraries(fl_upstream_engbase_tu PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
    target_compile_definitions(fl_upstream_engbase_tu PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1)
    target_compile_options(fl_upstream_engbase_tu PRIVATE -fms-extensions -w)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fl_upstream_engbase_tu PRIVATE -fdeclspec)
    endif()

    # Minimal upstream Common subset for early init (config/INI readers + required file/text/parse runtime support)
    add_library(fl_upstream_common_ini_tu OBJECT
      "${FL_SRC_ROOT}/Common/common_ini.cpp"
      "${FL_SRC_ROOT}/Common/common_textfile.cpp"
      "${FL_SRC_ROOT}/Common/msvcrt_atof.cpp"
    )
    target_link_libraries(fl_upstream_common_ini_tu PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
    target_compile_definitions(fl_upstream_common_ini_tu PRIVATE COMMON_EXPORTS DALIB_EXPORTS FL_LINUX_PORT_UPSTREAM_TU=1)
    target_compile_options(fl_upstream_common_ini_tu PRIVATE -fms-extensions -w)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fl_upstream_common_ini_tu PRIVATE -fdeclspec)
    endif()

    # M8.7 path A: upstream Common timing TU (portable/guarded build) for early runtime init
    add_library(fl_upstream_common_timing_tu OBJECT
      "${FL_SRC_ROOT}/Common/common_timing.cpp"
    )
    target_link_libraries(fl_upstream_common_timing_tu PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
    target_compile_definitions(fl_upstream_common_timing_tu PRIVATE COMMON_EXPORTS DALIB_EXPORTS FL_LINUX_PORT_UPSTREAM_TU=1)
    target_compile_options(fl_upstream_common_timing_tu PRIVATE -fms-extensions -w)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(fl_upstream_common_timing_tu PRIVATE -fdeclspec)
    endif()


    add_executable(m3_math_data_tests
      tests/m3_math_data_tests.cpp
      $<TARGET_OBJECTS:fl_upstream_dacom_tu>
      $<TARGET_OBJECTS:fl_upstream_readfile_tu>
      $<TARGET_OBJECTS:fl_upstream_x86math_tu>
    )
    target_link_libraries(m3_math_data_tests PRIVATE fl_linux_port_platform fl_linux_port_path fl_upstream_tu_iface)

    # M8.1–M8.7 — Linux client bring-up executable (32-bit): entry + stubs + upstream
    # DACOM/ReadFile/x86math wrapper TUs. M8.7 links real upstream DALib TU(s) in Linux-portable
    # mode (DALib.cpp globals/OpenData/CloseData/portable Startup+Shutdown, md5hash, CDPNetCodec,
    # CDPMsgList) into freelancer_linux_client; DirectPlay/CDP client/server/proxy/GUN TUs remain
    # dependency-blocked (dplay8/COM runtime).
    if(FL_LINUX_BUILD_CLIENT)
      add_library(fl_dalib_core_bringup OBJECT
        "${FL_SRC_ROOT}/DALib/DALib.cpp"
        "${FL_SRC_ROOT}/DALib/CDPNetCodec.cpp"
        "${FL_SRC_ROOT}/DALib/md5hash.cpp"
        "${FL_SRC_ROOT}/DALib/CDPMsgList.cpp"
      )
      target_link_libraries(fl_dalib_core_bringup PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_dalib_core_bringup PRIVATE FL_LINUX_PORT_DALIB_CORE_ONLY FL_LINUX_PORT_DALIB_PORTABLE DALIB_EXPORTS ZLIB_WINAPI)
      target_include_directories(fl_dalib_core_bringup PRIVATE
        "${FL_SRC_ROOT}/DALib"
        "${FL_SRC_ROOT}/zlib/vendor"
      )

      add_library(fl_upstream_client_init_tu OBJECT
        "${FL_SRC_ROOT}/freelancer/Client/engine_init.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/main.cpp"
      )
      target_link_libraries(fl_upstream_client_init_tu PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_upstream_client_init_tu PRIVATE FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS)
      target_include_directories(fl_upstream_client_init_tu PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_upstream_client_init_tu PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_upstream_client_init_tu PRIVATE -fdeclspec)
      endif()

      add_executable(freelancer_linux_client
        src/linux_client_main.cpp
        src/linux_client_stubs.cpp
        src/cdp_stubs.cpp
        src/m87_upstream_bootstrap.cpp
        $<TARGET_OBJECTS:fl_dalib_core_bringup>
        $<TARGET_OBJECTS:fl_upstream_dacom_tu>
        $<TARGET_OBJECTS:fl_upstream_readfile_tu>
        $<TARGET_OBJECTS:fl_upstream_x86math_tu>
        $<TARGET_OBJECTS:fl_upstream_system_tu>
        $<TARGET_OBJECTS:fl_upstream_engbase_tu>
        $<TARGET_OBJECTS:fl_upstream_common_ini_tu>
        $<TARGET_OBJECTS:fl_upstream_common_timing_tu>
      )
      target_link_libraries(freelancer_linux_client PRIVATE fl_linux_port_platform fl_linux_port_path fl_linux_port_gameplay fl_upstream_tu_iface fl_upstream_zlib)
      if(FL_LINUX_32BIT)
        target_link_libraries(freelancer_linux_client PRIVATE asound)
      endif()      # M8.3 window/input/render hookups (optional when 32-bit SDL3/Vulkan libs unavailable)
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

      # M8.7 integration probes (EXCLUDE_FROM_ALL): compile experiments for upstream
      # DALib/Common/freelancer TUs. These targets are not part of normal builds and may
      # fail while porting; use --target <name> to iterate on blockers.
      # (common_ini probe removed: minimal Common INI subset is now linked into freelancer_linux_client)

      # Combined EXCLUDE_FROM_ALL compile probe for upstream client bootstrap/init TUs.
      # Build with: --target fl_m87_freelancer_client_probe (iterative blocker triage).
      add_library(fl_m87_freelancer_client_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/engine_init.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/main.cpp"
      )
      target_link_libraries(fl_m87_freelancer_client_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_freelancer_client_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_freelancer_client_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_freelancer_client_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_freelancer_client_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client window globals (defines g_client/g_display)
      add_library(fl_m87_client_window_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/window.cpp"
      )
      target_link_libraries(fl_m87_client_window_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_window_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_window_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_window_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_window_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client input/ime/error/exit modules
      add_library(fl_m87_client_input_ime_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/input_manager.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_ime_focus.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_error.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_exit.cpp"
      )
      target_link_libraries(fl_m87_client_input_ime_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_input_ime_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_input_ime_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_input_ime_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_input_ime_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client nn_error/nn_exit/input/message_pump modules
      add_library(fl_m87_client_nn_input_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_error.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_exit.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/input_manager.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/message_pump.cpp"
      )
      target_link_libraries(fl_m87_client_nn_input_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_nn_input_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_nn_input_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_nn_input_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_nn_input_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream Win3D base
      add_library(fl_m87_win3d_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/win3d.cpp"
      )
      target_link_libraries(fl_m87_win3d_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_win3d_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_win3d_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_win3d_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_win3d_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream Win3D immediate/render/renderables
      add_library(fl_m87_win3d_extra_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/win3d_immediate.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_render.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_renderables.cpp"
      )
      target_link_libraries(fl_m87_win3d_extra_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_win3d_extra_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_win3d_extra_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_win3d_extra_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_win3d_extra_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream Async + ErrorHandler + intro_connect
      add_library(fl_m87_async_error_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/Common/common_async.cpp"
        "${FL_SRC_ROOT}/Common/common_errorhandler.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/intro_connect.cpp"
      )
      target_link_libraries(fl_m87_async_error_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_async_error_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_async_error_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_async_error_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_async_error_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream chat + dataconfig
      add_library(fl_m87_chat_dataconfig_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/chat_init.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/dataconfig_progress.cpp"
      )
      target_link_libraries(fl_m87_chat_dataconfig_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_chat_dataconfig_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_chat_dataconfig_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_chat_dataconfig_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_chat_dataconfig_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client state/frame/timing
      add_library(fl_m87_client_state_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_frame_tick.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_timing.cpp"
      )
      target_link_libraries(fl_m87_client_state_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_state_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_state_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_state_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_state_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream Chat + DataConfig
      add_library(fl_m87_chat_dataconfig2_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/chat_init.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_text_input.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/dataconfig_progress.cpp"
      )
      target_link_libraries(fl_m87_chat_dataconfig2_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_chat_dataconfig2_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_chat_dataconfig2_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_chat_dataconfig2_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_chat_dataconfig2_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 2
      add_library(fl_m87_client_core2_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_gameobj.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient_space.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
      )
      target_link_libraries(fl_m87_client_core2_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core2_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core2_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core2_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core2_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 3
      add_library(fl_m87_client_core3_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_base_market.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_debris.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_equip_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_explosion.cpp"
      )
      target_link_libraries(fl_m87_client_core3_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core3_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core3_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core3_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core3_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 4
      add_library(fl_m87_client_core4_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_fuse.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_mesh_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_objdb.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_tick_profile.cpp"
      )
      target_link_libraries(fl_m87_client_core4_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core4_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core4_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core4_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core4_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 5
      add_library(fl_m87_client_core5_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_timing.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_land_action.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_ime_focus.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_info.cpp"
      )
      target_link_libraries(fl_m87_client_core5_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core5_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core5_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core5_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core5_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 6
      add_library(fl_m87_client_core6_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/character_select_equip.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_flow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_handshake.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_lists.cpp"
      )
      target_link_libraries(fl_m87_client_core6_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core6_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core6_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core6_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core6_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 7
      add_library(fl_m87_client_core7_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/effect_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_info.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_list.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_vertex_batch.cpp"
      )
      target_link_libraries(fl_m87_client_core7_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core7_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core7_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core7_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core7_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 8
      add_library(fl_m87_client_core8_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/engine_character.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/engine_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/equip_visual_set.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fade_to_black.cpp"
      )
      target_link_libraries(fl_m87_client_core8_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core8_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core8_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core8_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core8_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 9
      add_library(fl_m87_client_core9_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/fl_fx_mesh.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_atlas.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_imp.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_string.cpp"
      )
      target_link_libraries(fl_m87_client_core9_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core9_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core9_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core9_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core9_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 10
      add_library(fl_m87_client_core10_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/gameflow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/rich_text_win.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_command_dispatch.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
      )
      target_link_libraries(fl_m87_client_core10_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core10_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core10_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core10_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core10_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 11
      add_library(fl_m87_client_core11_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/hud_target.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_comm.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_button.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_contact_list.cpp"
      )
      target_link_libraries(fl_m87_client_core11_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core11_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core11_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core11_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core11_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 12
      add_library(fl_m87_client_core12_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/hud_cruise_progress.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_damage_indicator.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_element_list.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_maneuvers.cpp"
      )
      target_link_libraries(fl_m87_client_core12_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core12_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core12_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core12_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core12_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 13
      add_library(fl_m87_client_core13_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/hud_status_core.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_targetable_objects.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_target_panel.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_waypoint.cpp"
      )
      target_link_libraries(fl_m87_client_core13_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core13_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core13_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core13_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core13_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 14
      add_library(fl_m87_client_core14_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/hud_weapon_groups.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/hud_widescreen.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/input_manager.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_error.cpp"
      )
      target_link_libraries(fl_m87_client_core14_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core14_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core14_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core14_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core14_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 15
      add_library(fl_m87_client_core15_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_exit.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_montage_draw.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_rollover.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_tooltip.cpp"
      )
      target_link_libraries(fl_m87_client_core15_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core15_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core15_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core15_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core15_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 16
      add_library(fl_m87_client_core16_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_ui_pulse.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/star_sphere.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_renderables.cpp"
      )
      target_link_libraries(fl_m87_client_core16_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core16_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core16_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core16_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core16_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 17
      add_library(fl_m87_client_core17_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/texture_panels.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/texture_shapes.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_immediate.cpp"
      )
      target_link_libraries(fl_m87_client_core17_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core17_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core17_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core17_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core17_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 18
      add_library(fl_m87_client_core18_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/win3d_render.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_renderables.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/wincontrol_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/wincontrol_children.cpp"
      )
      target_link_libraries(fl_m87_client_core18_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core18_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core18_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core18_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core18_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 19
      add_library(fl_m87_client_core19_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/win3d_engine_submit.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/beam_effect.cpp"
      )
      target_link_libraries(fl_m87_client_core19_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core19_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core19_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core19_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core19_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 20
      add_library(fl_m87_client_core20_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/base_walk.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/button_mover.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cargo_bar_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_asteroid.cpp"
      )
      target_link_libraries(fl_m87_client_core20_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core20_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core20_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core20_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core20_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 21
      add_library(fl_m87_client_core21_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/cli_asteroid_dynamic.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_asteroid_med.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_nebula.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_projectile_fx.cpp"
      )
      target_link_libraries(fl_m87_client_core21_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core21_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core21_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core21_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core21_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 22
      add_library(fl_m87_client_core22_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/cli_tractor_beam.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cockpit_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/commodity_by_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/dealer_equip_stats.cpp"
      )
      target_link_libraries(fl_m87_client_core22_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core22_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core22_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core22_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core22_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 23
      add_library(fl_m87_client_core23_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/fl_ime_input_box.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fl_fx_mesh.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/flash_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/best_path.cpp"
      )
      target_link_libraries(fl_m87_client_core23_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core23_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core23_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core23_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core23_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 24
      add_library(fl_m87_client_core24_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_base_market.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_cliobj.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_debris.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_equip_fx.cpp"
      )
      target_link_libraries(fl_m87_client_core24_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core24_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core24_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core24_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core24_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 25
      add_library(fl_m87_client_core25_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_explosion.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_frame_tick.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_fuse.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_gameobj.cpp"
      )
      target_link_libraries(fl_m87_client_core25_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core25_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core25_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core25_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core25_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 26
      add_library(fl_m87_client_core26_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient_space.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_ime_focus.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_land_action.cpp"
      )
      target_link_libraries(fl_m87_client_core26_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core26_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core26_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core26_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core26_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 27
      add_library(fl_m87_client_core27_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_mesh_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_objdb.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_tick_profile.cpp"
      )
      target_link_libraries(fl_m87_client_core27_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core27_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core27_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core27_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core27_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 28
      add_library(fl_m87_client_core28_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_timing.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_info.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_select_equip.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_flow.cpp"
      )
      target_link_libraries(fl_m87_client_core28_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core28_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core28_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core28_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core28_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 29
      add_library(fl_m87_client_core29_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/connect_handshake.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_lists.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_info.cpp"
      )
      target_link_libraries(fl_m87_client_core29_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core29_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core29_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core29_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core29_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 30
      add_library(fl_m87_client_core30_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_list.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_vertex_batch.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/engine_character.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/engine_light.cpp"
      )
      target_link_libraries(fl_m87_client_core30_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core30_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core30_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core30_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core30_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 31
      add_library(fl_m87_client_core31_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/equip_visual_set.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fade_to_black.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/flash_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fl_fx_mesh.cpp"
      )
      target_link_libraries(fl_m87_client_core31_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core31_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core31_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core31_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core31_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 32
      add_library(fl_m87_client_core32_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/fl_ime_input_box.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_atlas.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_imp.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_string.cpp"
      )
      target_link_libraries(fl_m87_client_core32_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core32_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core32_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core32_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core32_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 33
      add_library(fl_m87_client_core33_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/gameflow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/best_path.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/base_walk.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/button_mover.cpp"
      )
      target_link_libraries(fl_m87_client_core33_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core33_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core33_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core33_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core33_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 34
      add_library(fl_m87_client_core34_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/cargo_bar_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_asteroid.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_asteroid_dynamic.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_asteroid_med.cpp"
      )
      target_link_libraries(fl_m87_client_core34_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core34_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core34_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core34_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core34_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 35
      add_library(fl_m87_client_core35_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/cli_nebula.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_projectile_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cli_tractor_beam.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cockpit_load.cpp"
      )
      target_link_libraries(fl_m87_client_core35_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core35_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core35_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core35_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core35_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 36
      add_library(fl_m87_client_core36_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/commodity_by_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/dealer_equip_stats.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/beam_effect.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/best_path.cpp"
      )
      target_link_libraries(fl_m87_client_core36_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core36_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core36_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core36_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core36_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 37
      add_library(fl_m87_client_core37_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/win3d.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_immediate.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_render.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win3d_renderables.cpp"
      )
      target_link_libraries(fl_m87_client_core37_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core37_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core37_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core37_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core37_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 38
      add_library(fl_m87_client_core38_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/win3d_engine_submit.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/wincontrol_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/wincontrol_children.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/window.cpp"
      )
      target_link_libraries(fl_m87_client_core38_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core38_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core38_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core38_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core38_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 39
      add_library(fl_m87_client_core39_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/alchemy_float_forward.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/anim_script_set.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/asteroid_billboard.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/base_walk.cpp"
      )
      target_link_libraries(fl_m87_client_core39_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core39_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core39_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core39_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core39_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 40
      add_library(fl_m87_client_core40_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/button_mover.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cargo_bar_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_init.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_control.cpp"
      )
      target_link_libraries(fl_m87_client_core40_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core40_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core40_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core40_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core40_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 41
      add_library(fl_m87_client_core41_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/chat_text_input.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_base_market.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_cliobj.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_debris.cpp"
      )
      target_link_libraries(fl_m87_client_core41_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core41_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core41_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core41_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core41_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 42
      add_library(fl_m87_client_core42_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_equip_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_explosion.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_frame_tick.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_fuse.cpp"
      )
      target_link_libraries(fl_m87_client_core42_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core42_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core42_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core42_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core42_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 43
      add_library(fl_m87_client_core43_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_gameobj.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient_space.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_ime_focus.cpp"
      )
      target_link_libraries(fl_m87_client_core43_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core43_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core43_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core43_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core43_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 44
      add_library(fl_m87_client_core44_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_land_action.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_mesh_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_objdb.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
      )
      target_link_libraries(fl_m87_client_core44_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core44_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core44_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core44_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core44_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 45
      add_library(fl_m87_client_core45_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_tick_profile.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_timing.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_info.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_select_equip.cpp"
      )
      target_link_libraries(fl_m87_client_core45_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core45_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core45_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core45_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core45_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 46
      add_library(fl_m87_client_core46_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/connect_flow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_handshake.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_lists.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_base.cpp"
      )
      target_link_libraries(fl_m87_client_core46_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core46_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core46_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core46_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core46_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 47
      add_library(fl_m87_client_core47_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_info.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_list.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_vertex_batch.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/engine_character.cpp"
      )
      target_link_libraries(fl_m87_client_core47_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core47_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core47_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core47_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core47_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 48
      add_library(fl_m87_client_core48_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/engine_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/equip_visual_set.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fade_to_black.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/flash_light.cpp"
      )
      target_link_libraries(fl_m87_client_core48_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core48_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core48_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core48_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core48_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 49
      add_library(fl_m87_client_core49_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/fl_fx_mesh.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fl_ime_input_box.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_atlas.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_imp.cpp"
      )
      target_link_libraries(fl_m87_client_core49_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core49_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core49_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core49_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core49_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 50
      add_library(fl_m87_client_core50_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/base_walk.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/button_mover.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cargo_bar_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_init.cpp"
      )
      target_link_libraries(fl_m87_client_core50_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core50_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core50_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core50_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core50_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 51
      add_library(fl_m87_client_core51_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/chat_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_text_input.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_base_market.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_cliobj.cpp"
      )
      target_link_libraries(fl_m87_client_core51_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core51_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core51_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core51_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core51_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 52
      add_library(fl_m87_client_core52_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_debris.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_equip_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_explosion.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_frame_tick.cpp"
      )
      target_link_libraries(fl_m87_client_core52_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core52_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core52_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core52_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core52_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 53
      add_library(fl_m87_client_core53_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_fuse.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_gameobj.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient_space.cpp"
      )
      target_link_libraries(fl_m87_client_core53_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core53_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core53_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core53_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core53_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 54
      add_library(fl_m87_client_core54_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_ime_focus.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_land_action.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_mesh_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_objdb.cpp"
      )
      target_link_libraries(fl_m87_client_core54_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core54_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core54_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core54_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core54_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 55
      add_library(fl_m87_client_core55_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_tick_profile.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_timing.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_info.cpp"
      )
      target_link_libraries(fl_m87_client_core55_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core55_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core55_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core55_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core55_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 56
      add_library(fl_m87_client_core56_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/character_select_equip.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_flow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_handshake.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_lists.cpp"
      )
      target_link_libraries(fl_m87_client_core56_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core56_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core56_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core56_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core56_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 57
      add_library(fl_m87_client_core57_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/effect_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_info.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_list.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_vertex_batch.cpp"
      )
      target_link_libraries(fl_m87_client_core57_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core57_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core57_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core57_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core57_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 58
      add_library(fl_m87_client_core58_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/engine_character.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/engine_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/equip_visual_set.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fade_to_black.cpp"
      )
      target_link_libraries(fl_m87_client_core58_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core58_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core58_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core58_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core58_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 59
      add_library(fl_m87_client_core59_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/flash_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fl_fx_mesh.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fl_ime_input_box.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_atlas.cpp"
      )
      target_link_libraries(fl_m87_client_core59_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core59_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core59_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core59_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core59_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 60
      add_library(fl_m87_client_core60_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/font_imp.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_string.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gameflow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/best_path.cpp"
      )
      target_link_libraries(fl_m87_client_core60_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core60_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core60_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core60_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core60_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 61
      add_library(fl_m87_client_core61_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/base_walk.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/button_mover.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cargo_bar_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_init.cpp"
      )
      target_link_libraries(fl_m87_client_core61_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core61_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core61_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core61_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core61_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 62
      add_library(fl_m87_client_core62_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/chat_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_text_input.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_base_market.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_cliobj.cpp"
      )
      target_link_libraries(fl_m87_client_core62_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core62_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core62_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core62_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core62_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 63
      add_library(fl_m87_client_core63_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_debris.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_equip_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_explosion.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_frame_tick.cpp"
      )
      target_link_libraries(fl_m87_client_core63_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core63_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core63_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core63_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core63_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 64
      add_library(fl_m87_client_core64_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_fuse.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_gameobj.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient_space.cpp"
      )
      target_link_libraries(fl_m87_client_core64_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core64_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core64_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core64_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core64_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 65
      add_library(fl_m87_client_core65_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_ime_focus.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_land_action.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_mesh_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_objdb.cpp"
      )
      target_link_libraries(fl_m87_client_core65_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core65_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core65_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core65_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core65_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 66
      add_library(fl_m87_client_core66_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_tick_profile.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_timing.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_info.cpp"
      )
      target_link_libraries(fl_m87_client_core66_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core66_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core66_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core66_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core66_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 67
      add_library(fl_m87_client_core67_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/character_select_equip.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_flow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_handshake.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/connect_lists.cpp"
      )
      target_link_libraries(fl_m87_client_core67_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core67_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core67_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core67_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core67_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 68
      add_library(fl_m87_client_core68_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/effect_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_info.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_type_list.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/effect_vertex_batch.cpp"
      )
      target_link_libraries(fl_m87_client_core68_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core68_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core68_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core68_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core68_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 69
      add_library(fl_m87_client_core69_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/engine_character.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/engine_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/equip_visual_set.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fade_to_black.cpp"
      )
      target_link_libraries(fl_m87_client_core69_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core69_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core69_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core69_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core69_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 70
      add_library(fl_m87_client_core70_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/flash_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fl_fx_mesh.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/fl_ime_input_box.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_atlas.cpp"
      )
      target_link_libraries(fl_m87_client_core70_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core70_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core70_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core70_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core70_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 71
      add_library(fl_m87_client_core71_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/font_imp.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/font_string.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gameflow.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/best_path.cpp"
      )
      target_link_libraries(fl_m87_client_core71_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core71_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core71_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core71_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core71_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 72
      add_library(fl_m87_client_core72_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/base_walk.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/button_mover.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/cargo_bar_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_init.cpp"
      )
      target_link_libraries(fl_m87_client_core72_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core72_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core72_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core72_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core72_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 73
      add_library(fl_m87_client_core73_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/chat_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/chat_text_input.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_base_market.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_cliobj.cpp"
      )
      target_link_libraries(fl_m87_client_core73_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core73_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core73_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core73_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core73_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 74
      add_library(fl_m87_client_core74_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_debris.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_equip_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_explosion.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_frame_tick.cpp"
      )
      target_link_libraries(fl_m87_client_core74_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core74_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core74_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core74_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core74_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 75
      add_library(fl_m87_client_core75_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_fuse.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_gameobj.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_iclient_space.cpp"
      )
      target_link_libraries(fl_m87_client_core75_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core75_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core75_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core75_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core75_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 76
      add_library(fl_m87_client_core76_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_ime_focus.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_land_action.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_mesh_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_objdb.cpp"
      )
      target_link_libraries(fl_m87_client_core76_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core76_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core76_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core76_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core76_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 77
      add_library(fl_m87_client_core77_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_tick_profile.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/client_timing.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/character_info.cpp"
      )
      target_link_libraries(fl_m87_client_core77_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core77_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core77_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core77_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core77_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 78
      add_library(fl_m87_client_core78_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/client_exports.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/game_options.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gate_tunnel.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_character_object.cpp"
      )
      target_link_libraries(fl_m87_client_core78_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core78_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core78_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core78_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core78_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 79
      add_library(fl_m87_client_core79_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/gf_hotspot.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_object_ship.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_script.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_script_object.cpp"
      )
      target_link_libraries(fl_m87_client_core79_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core79_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core79_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core79_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core79_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 80
      add_library(fl_m87_client_core80_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/gf_script_spawn.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_camera.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_component.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_compound.cpp"
      )
      target_link_libraries(fl_m87_client_core80_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core80_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core80_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core80_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core80_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 81
      add_library(fl_m87_client_core81_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_deformable.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_light.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_marker.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_psys.cpp"
      )
      target_link_libraries(fl_m87_client_core81_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core81_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core81_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core81_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core81_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 82
      add_library(fl_m87_client_core82_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_scene.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/gf_thorn_sound.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/graph_library.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ids_rdl.cpp"
      )
      target_link_libraries(fl_m87_client_core82_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core82_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core82_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core82_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core82_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 83
      add_library(fl_m87_client_core83_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ids_strings.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/image.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/infocard_map.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/intro_page_util.cpp"
      )
      target_link_libraries(fl_m87_client_core83_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core83_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core83_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core83_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core83_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 84
      add_library(fl_m87_client_core84_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/jump_effect.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/knowledge_map.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/lights_camera.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/list_box.cpp"
      )
      target_link_libraries(fl_m87_client_core84_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core84_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core84_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core84_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core84_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 85
      add_library(fl_m87_client_core85_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/loading_screen.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/main_command_handler.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/menu_enter_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/menu_fonts_load.cpp"
      )
      target_link_libraries(fl_m87_client_core85_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core85_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core85_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core85_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core85_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 86
      add_library(fl_m87_client_core86_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/menu_keymap.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/menu_mp_session.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/menu_sound_arch.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/menu_ui_draw.cpp"
      )
      target_link_libraries(fl_m87_client_core86_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core86_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core86_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core86_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core86_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 87
      add_library(fl_m87_client_core87_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/menu_ui_init.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/menu_ui_widgets.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/menu_universe_base.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/misc_util.cpp"
      )
      target_link_libraries(fl_m87_client_core87_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core87_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core87_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core87_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core87_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 88
      add_library(fl_m87_client_core88_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/mission_briefing.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/mission_created_solars.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nav_bar.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nebula_volume.cpp"
      )
      target_link_libraries(fl_m87_client_core88_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core88_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core88_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core88_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core88_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 89
      add_library(fl_m87_client_core89_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_blocking_dialog.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_button_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_confirm.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_dealer.cpp"
      )
      target_link_libraries(fl_m87_client_core89_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core89_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core89_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core89_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core89_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 90
      add_library(fl_m87_client_core90_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_group_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_help.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_inventory.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_load_save.cpp"
      )
      target_link_libraries(fl_m87_client_core90_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core90_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core90_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core90_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core90_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 91
      add_library(fl_m87_client_core91_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_main_button.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_messaging.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_mission_message.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_mission_vendor.cpp"
      )
      target_link_libraries(fl_m87_client_core91_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core91_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core91_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core91_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core91_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 92
      add_library(fl_m87_client_core92_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_modal.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_navmap.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_navmap_build.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_navmap_draw.cpp"
      )
      target_link_libraries(fl_m87_client_core92_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core92_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core92_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core92_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core92_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 93
      add_library(fl_m87_client_core93_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_navmap_input.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_navmap_panels.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_navmap_update.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_navmap_view.cpp"
      )
      target_link_libraries(fl_m87_client_core93_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core93_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core93_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core93_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core93_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 94
      add_library(fl_m87_client_core94_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_news.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_offer_dialog.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_pause.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_player_info.cpp"
      )
      target_link_libraries(fl_m87_client_core94_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core94_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core94_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core94_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core94_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 95
      add_library(fl_m87_client_core95_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_preferences.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_prompt.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_repair.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_scanner.cpp"
      )
      target_link_libraries(fl_m87_client_core95_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core95_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core95_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core95_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core95_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 96
      add_library(fl_m87_client_core96_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_ship_trader.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_static_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_story_star.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_text_control.cpp"
      )
      target_link_libraries(fl_m87_client_core96_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core96_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core96_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core96_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core96_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 97
      add_library(fl_m87_client_core97_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_text_scratch.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_trade.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/nn_ui_pulse.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/object_prop.cpp"
      )
      target_link_libraries(fl_m87_client_core97_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core97_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core97_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core97_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core97_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 98
      add_library(fl_m87_client_core98_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/page_account_select.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/page_character_create.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/page_character_select.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/page_character_select_single.cpp"
      )
      target_link_libraries(fl_m87_client_core98_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core98_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core98_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core98_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core98_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 99
      add_library(fl_m87_client_core99_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/page_options.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/page_server_select.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/part_cache.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/particle_effect.cpp"
      )
      target_link_libraries(fl_m87_client_core99_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core99_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core99_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core99_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core99_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 100
      add_library(fl_m87_client_core100_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/perf_options.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/pixel_format.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/player_behavior.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/player_spawn.cpp"
      )
      target_link_libraries(fl_m87_client_core100_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core100_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core100_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core100_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core100_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 101
      add_library(fl_m87_client_core101_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/reg_key_stack.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/render_ex.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/room_camera.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/room_control.cpp"
      )
      target_link_libraries(fl_m87_client_core101_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core101_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core101_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core101_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core101_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 102
      add_library(fl_m87_client_core102_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/room_control_hotspot.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/scene_render_registry.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/screen_quad_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/scroll_control.cpp"
      )
      target_link_libraries(fl_m87_client_core102_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core102_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core102_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core102_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core102_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 103
      add_library(fl_m87_client_core103_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/server_connect.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/server_filter.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/server_filter_dialog.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ship_camera_params.cpp"
      )
      target_link_libraries(fl_m87_client_core103_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core103_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core103_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core103_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core103_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 104
      add_library(fl_m87_client_core104_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ship_view_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/sound_config.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_death.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_docking.cpp"
      )
      target_link_libraries(fl_m87_client_core104_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core104_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core104_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core104_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core104_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 105
      add_library(fl_m87_client_core105_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_first.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_game.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_rear.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_script.cpp"
      )
      target_link_libraries(fl_m87_client_core105_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core105_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core105_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core105_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core105_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 106
      add_library(fl_m87_client_core106_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_static.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_third.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_camera_turret.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_dust.cpp"
      )
      target_link_libraries(fl_m87_client_core106_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core106_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core106_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core106_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core106_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 107
      add_library(fl_m87_client_core107_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/space_letterbox.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_scene_draw.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_scene_load.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_scene_obj_draw.cpp"
      )
      target_link_libraries(fl_m87_client_core107_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core107_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core107_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core107_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core107_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 108
      add_library(fl_m87_client_core108_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/space_solar_world.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/space_state.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/star_sys.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/startup_config.cpp"
      )
      target_link_libraries(fl_m87_client_core108_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core108_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core108_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core108_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core108_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 109
      add_library(fl_m87_client_core109_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/star_sys.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/startup_config.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/swapped_material.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/test_size.cpp"
      )
      target_link_libraries(fl_m87_client_core109_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core109_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core109_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core109_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core109_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 110
      add_library(fl_m87_client_core110_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/text_win.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/trade_equip_list.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/tradelane_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/type_select.cpp"
      )
      target_link_libraries(fl_m87_client_core110_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core110_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core110_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core110_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core110_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 111
      add_library(fl_m87_client_core111_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ui_fast_table.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_list_box.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_page.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_page_desc.cpp"
      )
      target_link_libraries(fl_m87_client_core111_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core111_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core111_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core111_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core111_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 112
      add_library(fl_m87_client_core112_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ui_rich_text_win.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_scroll.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_ship_view_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_static_text.cpp"
      )
      target_link_libraries(fl_m87_client_core112_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core112_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core112_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core112_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core112_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 113
      add_library(fl_m87_client_core113_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ui_table.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_text_edit.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_text_msg_button.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_win_deform.cpp"
      )
      target_link_libraries(fl_m87_client_core113_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core113_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core113_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core113_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core113_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 114
      add_library(fl_m87_client_core114_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ui_win_object.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/vis_effect_arch.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/voice_profiles.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win_factory.cpp"
      )
      target_link_libraries(fl_m87_client_core114_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core114_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core114_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core114_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core114_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 115
      add_library(fl_m87_client_core115_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/win_object.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/wire_controls.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/text_win.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/trade_equip_list.cpp"
      )
      target_link_libraries(fl_m87_client_core115_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core115_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core115_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core115_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core115_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 116
      add_library(fl_m87_client_core116_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/tradelane_fx.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/type_select.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_fast_table.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_list_box.cpp"
      )
      target_link_libraries(fl_m87_client_core116_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core116_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core116_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core116_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core116_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 117
      add_library(fl_m87_client_core117_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ui_page.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_page_desc.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_rich_text_win.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_scroll.cpp"
      )
      target_link_libraries(fl_m87_client_core117_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core117_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core117_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core117_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core117_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 118
      add_library(fl_m87_client_core118_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ui_ship_view_control.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_static_text.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_table.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_text_edit.cpp"
      )
      target_link_libraries(fl_m87_client_core118_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core118_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core118_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core118_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core118_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 119
      add_library(fl_m87_client_core119_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/ui_text_msg_button.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_win_deform.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/ui_win_object.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/vis_effect_arch.cpp"
      )
      target_link_libraries(fl_m87_client_core119_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core119_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core119_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core119_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core119_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 120
      add_library(fl_m87_client_core120_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/voice_profiles.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win_factory.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/win_object.cpp"
        "${FL_SRC_ROOT}/freelancer/Client/wire_controls.cpp"
      )
      target_link_libraries(fl_m87_client_core120_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core120_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core120_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core120_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core120_probe PRIVATE -fdeclspec)
      endif()

      # Probe: upstream client core modules batch 121
      add_library(fl_m87_client_core121_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/freelancer/Client/nn_network_operation.cpp"
      )
      target_link_libraries(fl_m87_client_core121_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_client_core121_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 FREELANCER_EXPORTS COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_include_directories(fl_m87_client_core121_probe PRIVATE
        "${FL_SRC_ROOT}/freelancer/Client"
        "${FL_SRC_ROOT}/Content"
        "${FL_SRC_ROOT}/imeui"
      )
      target_compile_options(fl_m87_client_core121_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_client_core121_probe PRIVATE -fdeclspec)
      endif()

      # M8.7 path A/B probes: incremental upstream Common init/data TU triage (below client UI layer)
      add_library(fl_m87_common_timing_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/Common/common_timing.cpp"
      )
      target_link_libraries(fl_m87_common_timing_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_common_timing_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_compile_options(fl_m87_common_timing_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_common_timing_probe PRIVATE -fdeclspec)
      endif()

      add_library(fl_m87_common_misc_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/Common/common_misc.cpp"
      )
      target_link_libraries(fl_m87_common_misc_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_common_misc_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_compile_options(fl_m87_common_misc_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_common_misc_probe PRIVATE -fdeclspec)
      endif()

      add_library(fl_m87_common_dataconfig_probe OBJECT EXCLUDE_FROM_ALL
        "${FL_SRC_ROOT}/Common/common_dataconfig.cpp"
      )
      target_link_libraries(fl_m87_common_dataconfig_probe PRIVATE fl_linux_port_platform fl_upstream_tu_iface)
      target_compile_definitions(fl_m87_common_dataconfig_probe PRIVATE
        FL_LINUX_PORT_UPSTREAM_TU=1 FL_LINUX_PORT_LAYOUT_RELAXED=1 COMMON_EXPORTS DALIB_EXPORTS WIN32 _WINDOWS
      )
      target_compile_options(fl_m87_common_dataconfig_probe PRIVATE -fms-extensions -w)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(fl_m87_common_dataconfig_probe PRIVATE -fdeclspec)
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
