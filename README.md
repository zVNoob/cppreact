<div align="center">

# cppreact

**A C++20 immediate-mode UI library** — flexbox layout, HarfBuzz text shaping, hooks-style state, color emoji, arena allocation, pluggable GPU backends.

</div>

## Quick Start

```cpp
#include "container/stack.hpp"
#include "container/vbox.hpp"
#include "container/hbox.hpp"
#include "container/func.hpp"
#include "widgets/button_simple.hpp"
#include "widgets/rect.hpp"
#include "widgets/text.hpp"
#include "widgets/button_full.hpp"
#include "renderer/sdl_renderer.hpp"
#include "renderer/lookup_font.hpp"

using namespace cppreact;

int main() {
  sdl_renderer r({480, 360}, "cppreact Counter");

  auto font_display = r.load_font(lookup_font("Arial"), 72);
  auto font_button = r.load_font(lookup_font("Noto Sans"), 40);
  auto font_title  = r.load_font({"./NotoSans-Variable.ttf"}, 16);

  auto make_button = [&](const char* label, color col, auto action) {
    auto lambda = [action](uint16_t, std::pair<uint16_t, uint16_t>) { action(); };
    return button({.sizing = {GROW(), GROW()}, .on_mouse_down = lambda},
        rect({.col = col, .radius = 8, .sizing = {GROW(), GROW()}},
          text(font_button, label, {.col = {255, 255, 255},
                .sizing = {FIT(), FIT()},
                .alignment = {ALIGN_CENTER, ALIGN_CENTER}})));
  };

  auto ui = func([&](state& s) {
    auto count = s.get<int>(0);
    return stack({.sizing = {GROW(), GROW()}}, {
      vbox({.sizing = {GROW(), GROW()}, .padding = PAD(24), .spacing = 12}, {
        text(font_title, "Current Count", {.col = {150, 150, 180}}),
        text(font_display, std::to_string(count), {.col = {255, 255, 255}}),
        hbox({.spacing = 8}, {
          make_button("-",  {60, 30, 30}, [=]() { count = count - 1; }),
          make_button("Reset", {40, 40, 40}, [=]() { count = 0; }),
          make_button("+",  {30, 60, 30}, [=]() { count = count + 1; }),
        }),
      }),
    });
  });

  r.run(ui);
}
```

## How It Works

The widget tree is rebuilt every frame (immediate mode) using a fast arena allocator. Widgets that need persistent state use a hooks-style `state` system — analogous to React's `useState`.

```
Every frame:
  1. Reset arena + state cursors
  2. Rebuild widget tree (arena-allocate)
  3. Layout: update → fit → grow → position
  4. Render: collect commands → dispatch to GPU
  5. Events: process mouse/keyboard/scroll (reverse z-order)
```

### Layout

Three containers: `vbox`, `hbox`, `stack`. Per-axis sizing with `GROW` / `FIT` / `FIXED`:

| Helper | Effect |
|--------|--------|
| `FIXED(n)` | Fixed `n` pixels |
| `FIT(mn, mx)` | Fit to content |
| `GROW(mn, mx, w)` | Fill remaining space |

### State

```cpp
func([&](state& s) {
  auto count = s.get<int>(0);   // useState equivalent
  count = count + 1;            // triggers re-render
  return text(font, "Count: " + std::to_string(count));
});
```

### Ownership

`owner<T>` wraps `unique_ptr<T>` with implicit `T*` conversion. Resource loaders produce `owner<T>`; widgets borrow `T*`.

```cpp
auto font = r.load_font(lookup_font("Noto Sans"), 20);  // owner<font>
text(font, "Hello", {});                                  // font* via implicit cast
```

## Widgets

| Factory | What |
|---------|------|
| `hbox`, `vbox`, `stack` | Layout containers |
| `func(lambda)` | Stateful component |
| `viewport(cfg, child)` | Scrollable viewport with clip |
| `scroll(cfg, child)` | `func` wrapping viewport + mouse-wheel scroll |
| `scrolling(cfg, lambda)` | Mouse-wheel scroll input region |
| `text(font, str, cfg)` | Text label with HarfBuzz shaping + word-wrap |
| `input(font, cfg)` | Single-line text input (extends `text`) |
| `rect(cfg)` / `rect(cfg, children)` | Filled rounded rectangle |
| `border(cfg, child)` | Border-only rounded rectangle |
| `image(texture*, cfg)` | Image with aspect-ratio control |
| `button(cfg, lambda)` | Simple click/hold |
| `button(cfg)` / `button(cfg, children)` | Full button with hover/enter/leave |

## Font System

**`font`** — a prioritized collection of `single_font` instances for fallback.

- **Text rendering**: decodes UTF-8, groups codepoints into HarfBuzz runs by best font match, shapes each run, splits into words, and computes bounding boxes
- **Color emoji**: prefers color fonts (checked via `FT_HAS_COLOR`) during run formation; skips color modulation at render time so emoji textures display true color
- **Font loading**: `r.load_font(vector<path>, size)` creates a `font` from one or more file paths. `lookup_font("Family Name")` uses fontconfig to find installed fonts
- **Glyph caching**: each `single_font` caches rasterized glyphs by glyph ID

## Backends

- **SDL2** — `sdl_renderer({width, height}, title)`
- **SFML** — `sfml_renderer({width, height}, title)`

Both implement the abstract `renderer` API. Add new backends by implementing `on_load_texture`, `on_rect_cmd`, `on_image_cmd`, `on_text_cmd`, and `run`.

## Build

```sh
apt install libsdl2-dev libsdl2-image-dev libfreetype-dev libharfbuzz-dev libfontconfig-dev

make test       # build + run unit tests
make counter    # interactive counter demo
make dashboard  # analytics dashboard demo
```

## Project Structure

```
cppreact/
  container/   Layout containers (vbox, hbox, stack, viewport, func)
  widgets/     Visual + interactive widgets (text, input, rect, image, button, scrolling)
  internal/    Core: element, font, state, arena, owner, property, texture
  renderer/    Backends (SDL2, SFML) + abstract base + fontconfig lookup
  examples/    Demo applications
  tests/       Test suite
```

## License

MIT
