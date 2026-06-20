#include "container/stack.hpp"
#include "container/vbox.hpp"
#include "container/hbox.hpp"
#include "container/func.hpp"
#include "widgets/button_simple.hpp"
#include "renderer/lookup_font.hpp"
#include "widgets/rect.hpp"
#include "widgets/text.hpp"
#include "widgets/button_full.hpp"
#include "renderer/sdl_renderer.hpp"
#include "internal/element.hpp"

using namespace cppreact;

int main() {
  sdl_renderer r({480, 360}, "cppreact Counter");

  auto font_display = r.load_font(lookup_font("Arial"), 72);
  auto font_button = r.load_font(lookup_font("Noto Sans"), 40);
  auto font_title  = r.load_font({"./tests/NotoSans-Variable.ttf"}, 16);

  auto make_button = [&](const char* label, color col, auto action) {
    auto lambda = [action](uint16_t, std::pair<uint16_t, uint16_t>) { action(); };
    return button({.sizing = {GROW(), GROW()}, .on_mouse_down = lambda}, 
        rect({.col = col, .radius = 8, .sizing = {GROW(), GROW()}},
          text(font_button, label, {.col = {255, 255, 255}, .blend = NONE, .sizing = {FIT(), FIT()},
                .margin = PAD(), .alignment = {ALIGN_CENTER, ALIGN_CENTER}}
          )
        )
    );
  };

  auto ui = func([&](state& s) {
    auto count = s.get<int>(0);
    auto display = rect({.col = {25, 25, 50}, .radius = 12, .sizing = {GROW(), FIT()}, .padding = PAD(20)},
      vbox({.sizing = {GROW(), FIT()}, .alignment = {ALIGN_CENTER, ALIGN_CENTER},.spacing = 10}, {
        text(font_title, "Current Count", {.col = {150, 150, 180}, .blend = NONE, .sizing = {GROW(), FIT()}}),
        text(font_display, std::to_string(count), {.col = {255, 255, 255}, .blend = NONE}),
      })
    );

    return stack({.sizing = {GROW(), GROW()}}, {
      vbox({.sizing = {GROW(), GROW()}, .margin = PAD(), .alignment = {ALIGN_CENTER, ALIGN_CENTER}, .padding = PAD(24), .spacing = 20}, {
        display,
        hbox({.sizing = {GROW(), FIT()}, .spacing = 12}, {
          make_button("-",  {60, 30, 30}, [=]() { count = count - 1; }),
          make_button("Reset", {40, 40, 40}, [=]() { count = 0; }),
          make_button("+",  {30, 60, 30}, [=]() { count = count + 1; }),
        }),
      })
    });
  });

  r.run(ui);
  return 0;
}
