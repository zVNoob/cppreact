cmake_minimum_required(VERSION 3.16)
project(cppreact VERSION 0.2.0 LANGUAGES CXX)

# ── C++ standard ───────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies required by ALL renderers ────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE  freetype2  REQUIRED IMPORTED_TARGET)
pkg_check_modules(HARFBUZZ  harfbuzz   REQUIRED IMPORTED_TARGET)

# ── Fontconfig (optional, for lookup_font.hpp on Linux) ──────
find_package(Fontconfig QUIET)
if(Fontconfig_FOUND)
  set(CPPREACT_HAS_FONTCONFIG 1)
else()
  set(CPPREACT_HAS_FONTCONFIG 0)
endif()

# ── Renderer backends (auto-detected) ─────────────────────────
# SDL2
find_package(SDL2       QUIET)
find_package(SDL2_image QUIET)
if(SDL2_FOUND AND SDL2_image_FOUND)
  set(CPPREACT_HAS_SDL 1)
else()
  set(CPPREACT_HAS_SDL 0)
endif()

# SFML
find_package(SFML QUIET COMPONENTS Graphics Window System)
if(SFML_FOUND)
  set(CPPREACT_HAS_SFML 1)
else()
  set(CPPREACT_HAS_SFML 0)
endif()

# ── Pick preferred renderer ───────────────────────────────────
if(CPPREACT_HAS_SDL)
  set(CPPREACT_RENDERER_HEADER "renderer/sdl_renderer.hpp")
  set(CPPREACT_RENDERER_CLASS  "cppreact::sdl_renderer")
elseif(CPPREACT_HAS_SFML)
  set(CPPREACT_RENDERER_HEADER "renderer/sfml_renderer.hpp")
  set(CPPREACT_RENDERER_CLASS  "cppreact::sfml_renderer")
endif()

# ── Generate config header ───────────────────────────────────
configure_file(cppreact_config.h.in cppreact_config.h @ONLY)

# ── Common interface target (shared by all renderers) ────────
add_library(cppreact_common INTERFACE)
target_include_directories(cppreact_common INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cppreact>")
target_link_libraries(cppreact_common INTERFACE
  PkgConfig::FREETYPE
  PkgConfig::HARFBUZZ)
target_compile_features(cppreact_common INTERFACE cxx_std_20)

if(Fontconfig_FOUND)
  target_link_libraries(cppreact_common INTERFACE Fontconfig::Fontconfig)
  target_compile_definitions(cppreact_common INTERFACE CPPREACT_HAS_FONTCONFIG=1)
endif()

# ── SDL2 renderer target ──────────────────────────────────────
if(CPPREACT_HAS_SDL)
  add_library(cppreact_sdl INTERFACE)
  target_link_libraries(cppreact_sdl INTERFACE
    cppreact_common SDL2::SDL2 SDL2_image::SDL2_image)
  target_compile_definitions(cppreact_sdl INTERFACE CPPREACT_HAS_SDL=1)
endif()

# ── SFML renderer target ──────────────────────────────────────
if(CPPREACT_HAS_SFML)
  add_library(cppreact_sfml INTERFACE)
  target_link_libraries(cppreact_sfml INTERFACE
    cppreact_common sfml-graphics sfml-window sfml-system)
  target_compile_definitions(cppreact_sfml INTERFACE CPPREACT_HAS_SFML=1)
endif()

# ── Convenience alias ──────────────────────────────────────────
if(TARGET cppreact_sdl)
  add_library(cppreact ALIAS cppreact_sdl)
elseif(TARGET cppreact_sfml)
  add_library(cppreact ALIAS cppreact_sfml)
endif()

# ── Examples (only those whose dependencies are met) ──────────
if(CPPREACT_HAS_SDL)
  add_executable(cppreact_dashboard examples/dashboard.cpp)
  target_link_libraries(cppreact_dashboard PRIVATE cppreact_sdl)
endif()

# ── Tests ─────────────────────────────────────────────────────
if(CPPREACT_HAS_SDL)
  add_executable(cppreact_test tests/test.cpp)
  target_link_libraries(cppreact_test PRIVATE cppreact_sdl)
endif()

# ── Export for find_package(cppreact) ──────────────────────────
include(CMakePackageConfigHelpers)
install(TARGETS cppreact_common
  EXPORT cppreactTargets)
install(EXPORT cppreactTargets
  FILE cppreactTargets.cmake
  NAMESPACE cppreact::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppreact)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cppreact
  FILES_MATCHING PATTERN "*.hpp"
  PATTERN "build" EXCLUDE)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cppreact_config.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cppreact)

configure_package_config_file(
  cmake/cppreactConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/cppreactConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppreact)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/cppreactConfig.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppreact)
