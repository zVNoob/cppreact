<div align="center">

# cppreact - C++20 ReactJS

**A C++20 immediate-mode UI library** — flexbox-like layout, HarfBuzz text shaping, hooks-style state, arena allocation, pluggable GPU backends.

</div>

## Quick Start

```cpp
#include <filesystem>
#include "container/vbox.hpp"
#include "container/stack.hpp"
#include "container/func.hpp"
#include "widgets/text.hpp"
#include "widgets/rect.hpp"
#include "input/button_full.hpp"
#include "renderer/sdl_renderer.hpp"
#include "renderer/font_lookup.hpp"

using namespace cppreact;
namespace fs = std::filesystem;

int main() {
  sdl_renderer r({400, 300}, "cppreact Demo");

  auto font = r.load_font(lookup_font("Noto Sans"), 20);
  auto title = r.load_font({fs::path("./tests/NotoSans-Variable.ttf")}, 28);

  auto make_button = [&](const char* label, color col, auto action) {
    auto lambda = [action](uint16_t mask, std::pair<uint16_t, uint16_t> position) { action(); };
    return button({.sizing = {GROW(), GROW()}, .on_mouse_down = lambda}, 
        rect({.col = col, .radius = 8, .sizing = {GROW(), GROW()}},
          text(font, label, {.col = {255, 255, 255}, .blend = NONE, .sizing = {GROW(), GROW()},})
        )
    );
  };

  r.run(
    stack({.sizing = {GROW(), GROW()}}, {
      vbox({.sizing = {GROW(), GROW()}, .spacing = 8, .padding = PAD(16)}, {
        text(title, "Counter", {.col = {200, 180, 255}}),

        func([&](state& s) {
          auto count = s.get<int>(0);
          return
            vbox({.spacing = 4}, {
              text(font, "Count: " + std::to_string(count), {.col = {255, 255, 255}}),
              hbox({.spacing = 8}, {
                make_button("Reset", {200, 180, 255}, [=] { count = 0; }),
                make_button("+1", {200, 150, 100}, [=] { count = count + 1; }),
              })
            });
        }),
      })
    })
  );
  return 0;
}
```

## How It Works

cppreact rebuilds the widget tree every frame (immediate mode) using a fast arena allocator. Widgets that need persistent state across frames use a hooks-style `state` system — analogous to React's `useState` — rather than retaining objects.

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
| `FIT(min, max)` | Fit to content |
| `GROW(min, max, weight)` | Fill remaining space |

### State

```cpp
func([&](state& s) {
  auto count = s.get<int>(0);    // useState equivalent
  count = count + 1;             // triggers re-render
  return text(font, "Count: " + std::to_string(count));
});
```

### Ownership

`owner<T>` wraps `unique_ptr<T>` with implicit `T*` conversion. Resource loaders produce `owner<T>`; widgets borrow `T*`.

```cpp
auto font = r.load_font({...}, 20);  // owner<font>
text(font, "...", {});                // font* via implicit conversion
```

## Widgets

| Factory | What |
|---------|------|
| `hbox`, `vbox`, `stack` | Layout containers |
| `func(lambda)` | Stateful component |
| `viewport(child)` | Scrollable region |
| `text(font, str, cfg)` | Text label with shaping + word wrap |
| `rect(cfg)` / `rect(cfg, children)` | Filled rounded rectangle |
| `border(cfg, children)` | Border-only rounded rectangle |
| `image(texture*, cfg)` | Image with aspect ratio |
| `button(cfg, lambda)` | Simple click/hold |
| `button(cfg)` / `button(cfg, children)` | Full button with hover/enter/leave |
| `scroll_wheel(cfg, func)` | Scroll input region |

## Backends

- **SDL2** — `sdl_renderer({width, height}, title)`
- **SFML** — `sfml_renderer({width, height}, title)`

Both implement the abstract `renderer` API — add new backends by implementing `on_load_texture`, `on_rect_cmd`, `on_image_cmd`, `on_text_cmd`, and `run`.

## Build

```sh
apt install libsdl2-dev libsdl2-image-dev libfreetype-dev libharfbuzz-dev libfontconfig-dev

make test       # build + run tests
make counter    # interactive counter demo
make dashboard  # analytics dashboard demo
make gallery    # image gallery demo
make docs       # Doxygen documentation
```

## Project Structure

```
cppreact/
  container/   Layout containers
  input/       Interactive widgets
  internal/    Core: element, font, state, arena, owner,
  widgets/     Visual widgets
  renderer/    Backends + abstract base
  examples/    Demo applications
  tests/       Test suite
```

## License

MIT
