#pragma once

/** @file cppreact.hpp
 *  @brief Umbrella header — include everything needed to use cppreact.
 *
 *  Renderer backends (sdl_renderer.hpp / sfml_renderer.hpp) are NOT
 *  included here.  Include the one you need, or let CMake's
 *  @c CPPREACT_RENDERER_HEADER define point to it.
 */

// NOLINTBEGIN(misc-include-cleaner)
// ── Containers ────────────────────────────────────────────────
#include "container/func.hpp"
#include "container/stack.hpp"
#include "container/vbox.hpp"
#include "container/hbox.hpp"
#include "container/viewport.hpp"

// ── Widgets ───────────────────────────────────────────────────
#include "widgets/rect.hpp"
#include "widgets/text.hpp"
#include "widgets/input.hpp"
#include "widgets/image.hpp"
#include "widgets/scrolling.hpp"
#include "widgets/button_simple.hpp"
#include "widgets/button_full.hpp"

// ── Renderer base ─────────────────────────────────────────────
#include "renderer/lookup_font.hpp"
#include "renderer/renderer.hpp"
#include "renderer/sized_renderer.hpp"

// ── CMake-generated config (if present) ───────────────────────
#if __has_include("cppreact_config.h")
#  include "cppreact_config.h"
#endif
// NOLINTEND(misc-include-cleaner)
