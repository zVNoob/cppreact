/** @file test.cpp
 *  @brief Test/demo application for the cppreact UI framework */

#include "container/stack.hpp"
#include "container/vbox.hpp"
#include "container/viewport.hpp"
#include "renderer/lookup_font.hpp"
#include "widgets/input.hpp"
#include "widgets/rect.hpp"
#include "widgets/text.hpp"
#include "renderer/sdl_renderer.hpp"

using namespace cppreact;
/** @brief Application entry point
 *
 * Creates an 800x600 SDL renderer, loads fonts, builds a simple UI tree
 * (stack -> viewport -> vbox containing text widgets and styled rectangles),
 * and enters the main event loop.
 * @return 0 on success */
int main() {
  sdl_renderer r({800, 600});
  auto f18 = r.load_font(lookup_font("Noto Sans"), 18);
  auto f24 = r.load_font({"./tests/NotoSans-Variable.ttf"}, 24);

  auto ui = stack({.sizing = {GROW(),GROW()}}, { 
      viewport({.sizing = {GROW(),GROW()}, .margin = {10,100,10,10}, .scroll_y = 25},
        vbox({.sizing = {GROW(),GROW()}, .padding = PAD(10), .spacing = 10}, {
          text(f24, "cppreact Text Widget Demo á 🥹", {.col = {200, 180, 255}}),
          border({.col = {20, 20, 60}, .radius = 20, .sizing = {GROW(), FIT()}, .padding = PAD(20)},
            text(f18, "This is a long paragraph that should wrap to multiple lines. "
                      "The text widget uses FreeType and HarfBuzz for shaping and "
                      "rasterization, and SDL_SetTextureColorMod for colored rendering.",
                  {.col = {130, 200, 255}})
          ),
          rect({.col = {60, 20, 20}, .radius = 4, .sizing = {GROW(), FIT()}, .padding = PAD(8)},
            text(f18, "Word wrapping fits words within the available width. "
                      "Each word that exceeds the current line wraps to the next line.",
                  {.col = {255, 180, 130}})
          ),
          rect({.col = {20, 60, 20}, .radius = 4, .sizing = {GROW(), FIT()}, .padding = PAD(8)},
            text(f18, "á con  à", {.col = {180, 255, 130}, .alignment = {ALIGN_CENTER, ALIGN_CENTER}})
          ),
          rect({.col = {20, 60, 60}, .radius = 4, .sizing = {GROW(), FIT()}, .padding = PAD(8)},
            rect({.col = {20, 250, 60}, .radius = 4, .sizing = {FIXED(50), FIXED(10)}, .alignment = {ALIGN_CENTER, ALIGN_CENTER}})
          )
        })
      )
    });
  r.run(ui);
  return 0;
}
