#include "container/stack.hpp"
#include "container/vbox.hpp"
#include "container/hbox.hpp"
#include "container/viewport.hpp"
#include "renderer/lookup_font.hpp"
#include "widgets/rect.hpp"
#include "widgets/text.hpp"
#include "renderer/sdl_renderer.hpp"

using namespace cppreact;

int main() {
  sdl_renderer r({1000, 700}, "cppreact Dashboard");

  auto font_title = r.load_font(lookup_font("UbuntuMono Nerd Font"), 26);
  auto font_heading = r.load_font(lookup_font("UbuntuMono Nerd Font"), 17);
  auto font_body = r.load_font(lookup_font("UbuntuMono Nerd Font"), 14);
  auto font_stat = r.load_font(lookup_font("UbuntuMono Nerd Font"), 22);
  auto font_label = r.load_font({"./tests/NotoSans-Variable.ttf"}, 13);

  auto stat_card = [&](color bg, const char* title, const char* value, const char* subtitle) {
    return rect({.col = bg, .radius = 8, .sizing = {GROW(), FIT()}, .padding = PAD(16)},
      vbox({.sizing = {GROW(), FIT()}, .spacing = 4}, {
        text(font_body, title, {.col = {200, 200, 220}}),
        text(font_stat, value, {.col = {255, 255, 255}}),
        text(font_label, subtitle, {.col = {160, 160, 180}}),
      })
    );
  };

  auto activity_item = [&](const char* text_str, const char* time_str) {
    return hbox({.sizing = {GROW(), FIT()}, .padding = PAD(12), .spacing = 12}, {
      rect({.col = {50, 50, 80}, .radius = 4, .sizing = {FIXED(4), GROW()}}),
      vbox({.sizing = {GROW(), FIT()}, .spacing = 2}, {
        text(font_body, text_str, {.col = {200, 200, 220}}),
        text(font_label, time_str, {.col = {140, 140, 160}})
      })
    });
  };

  auto ui = stack({.sizing = {GROW(), GROW()}}, {
    vbox({.sizing = {GROW(), GROW()}, .spacing = 0}, {
      rect({.col = {25, 25, 45}, .radius = 0, .sizing = {GROW(), FIT()}, .padding = PAD(24)},
        text(font_title, "Analytics Dashboard", {.col = {230, 230, 250}})
      ),
      rect({.col = {30, 30, 50}, .radius = 0, .sizing = {GROW(), GROW()}, .padding = PAD(20)},
        vbox({.sizing = {GROW(), GROW()}, .spacing = 20}, {
          hbox({.sizing = {GROW(), FIT()}, .spacing = 16}, {
            stat_card({40, 50, 80}, "Total Users",  "12,847", "+12% this week"),
            stat_card({50, 40, 70}, "Active Now",   "1,423",  "384 sessions"),
            stat_card({40, 60, 60}, "Revenue",      "$48,290","+8.2% vs last month"),
            stat_card({60, 50, 50}, "Bounce Rate",  "32.4%",  "-2.1% improvement"),
          }),
          rect({.col = {35, 35, 55}, .radius = 8, .sizing = {GROW(), FIT()}, .padding = PAD(16)},
            text(font_heading, "Recent Activity", {.col = {210, 210, 230}})
          ),
          scroll({.sizing = {GROW(), GROW()}, .scroll_y = 0},
            vbox({.sizing = {GROW(), GROW()}, .spacing = 5}, {
              activity_item("User john_doe signed up", "2 min ago"),
              rect({.col = {40, 40, 60}, .radius = 0, .sizing = {GROW(), 1}}),
              activity_item("New order #4582 placed", "15 min ago"),
              rect({.col = {40, 40, 60}, .radius = 0, .sizing = {GROW(), 1}}),
              activity_item("Server deployment completed", "1 hour ago"),
              rect({.col = {40, 40, 60}, .radius = 0, .sizing = {GROW(), 1}}),
              activity_item("Error rate dropped to 0.02%", "2 hours ago"),
              rect({.col = {40, 40, 60}, .radius = 0, .sizing = {GROW(), 1}}),
              activity_item("Database backup completed", "4 hours ago"),
              rect({.col = {40, 40, 60}, .radius = 0, .sizing = {GROW(), 1}}),
              activity_item("New feature release v2.1.0", "6 hours ago"),
            })
          ),
        })
      ),
    })
  });

  r.run(ui);
  return 0;
}
