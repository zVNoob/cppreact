/** @file
 *  @brief Single-line text input widget extending text with cursor, IME, keyboard.
 */

#pragma once

#include "../renderer/renderer.hpp"
#include "internal/keycode.hpp"
#include <cstdint>
#include <set>
#include <string>

namespace cppreact {

  namespace _config {

    struct input_config {
      color text_col = {220, 220, 220, 255};
      color cursor_col = {255, 255, 255, 255};
      uint16_t cursor_width = 2;
      uint16_t max_length = 0;
      blend_mode blend = NONE;
      CPPREACT_ELEMENT_CONFIG;
    };

    inline text_config to_text_config(const input_config& cfg) {
      text_config tc;
      tc.col = cfg.text_col;
      tc.blend = cfg.blend;
      tc.alignment = cfg.alignment;
      tc.margin = cfg.margin;
      tc.sizing = cfg.sizing;
      return tc;
    }
  }

  namespace _detail {

    class input : public text {
      _config::input_config _icfg;
      state* _reg_state;

      static std::string codepoint_to_utf8(uint32_t cp) {
        if (cp < 0x80) return {char(cp)};
        if (cp < 0x800) return {char(0xC0 | (cp >> 6)), char(0x80 | (cp & 0x3F))};
        if (cp < 0x10000) return {char(0xE0 | (cp >> 12)), char(0x80 | ((cp >> 6) & 0x3F)), char(0x80 | (cp & 0x3F))};
        return {char(0xF0 | (cp >> 18)), char(0x80 | ((cp >> 12) & 0x3F)), char(0x80 | ((cp >> 6) & 0x3F)), char(0x80 | (cp & 0x3F))};
      }

      static size_t codepoint_start(const std::string& s, size_t pos) {
        if (pos >= s.size()) return s.size();
        if ((uint8_t(s[pos]) & 0xC0) != 0x80) return pos;
        while (pos > 0 && (uint8_t(s[--pos]) & 0xC0) == 0x80);
        return pos;
      }

      static size_t next_codepoint(const std::string& s, size_t pos) {
        if (pos >= s.size()) return s.size();
        size_t i = pos + 1;
        while (i < s.size() && (uint8_t(s[i]) & 0xC0) == 0x80) ++i;
        return i;
      }

    public:
      input(font* f, _config::input_config cfg, std::source_location loc) :
        text(f, "", _config::to_text_config(cfg), loc), _icfg(cfg) {
        _reg_state = &_storage::current_registry.get(id()).second;
      }

      std::string content() {
        _reg_state->reset();
        auto content = _reg_state->get<std::string>("");
        return content;
      }

      void on_update() override {
        element::on_update();
        _config = to_element_config(_cfg);

        _reg_state->reset();
        auto content = _reg_state->get<std::string>("");
        auto cursor = _reg_state->get<int>(0);
        auto key_handle = _reg_state->get<std::shared_ptr<_detail::key>>(nullptr);
        auto ime_cb = _reg_state->get<ime_callback>(ime_callback{});
        auto processed = _reg_state->get<std::set<uint32_t>>({});
        auto composition = _reg_state->get<std::string>("");
        auto composition_cursor = _reg_state->get<int>(0);

        processed->clear();

        // Update text content and re-rasterize each frame
        _text = content;
        if (_f) {
          _rasterized = _f->rasterize(_text);
          _minimum_width = 0;
          for (auto& word : _rasterized.words)
            _minimum_width = std::max(_minimum_width, word.bounding_width);
        }

        auto* renderer = _storage::current_registry.current_renderer;
        bool focused = renderer && renderer->get_focus_id() == id();

        if (focused) {
          if (key_handle.operator->()->get() == nullptr) {
            auto handle = renderer->register_key({},
              [content, cursor, processed](std::vector<keycode> keys, bool down, bool is_ime) mutable {
                if (!down) return;
                auto kc = keys[0];
                uint32_t code = uint32_t(kc);

                if (code >= 0x20 && code <= 0x10FFFF) {
                  if (!is_ime) return;
                  processed->insert(code);
                  std::string ch = codepoint_to_utf8(code);
                  int cur = int(cursor);
                  content->insert(size_t(cur), ch);
                  cursor = cur + int(ch.size());
                } else {
                  switch (kc) {
                    case keycode::BACKSPACE:
                      if (int(cursor) > 0) {
                        size_t start = codepoint_start(content, size_t(int(cursor)) - 1);
                        content->erase(start, size_t(int(cursor)) - start);
                        cursor = int(start);
                      }
                      break;
                    case keycode::DEL:
                      if (int(cursor) < int(content->size())) {
                        size_t next = next_codepoint(content, size_t(int(cursor)));
                        content->erase(size_t(int(cursor)), next - size_t(int(cursor)));
                      }
                      break;
                    case keycode::LEFT:
                      if (int(cursor) > 0)
                        cursor = int(codepoint_start(content, size_t(int(cursor)) - 1));
                      break;
                    case keycode::RIGHT:
                      if (int(cursor) < int(content->size()))
                        cursor = int(next_codepoint(content, size_t(int(cursor))));
                      break;
                    case keycode::ENTER:
                      break;
                    default: break;
                  }
                }
              }
            );
            key_handle = handle;
          }
          ime_cb->start();
        } else {
          ime_cb->end();
          if (key_handle.operator->()->get() != nullptr) {
            auto kh = *key_handle.operator->();
            renderer->unregister_key(kh);
            key_handle = std::shared_ptr<_detail::key>(nullptr);
          }
        }
      }

      void on_fit_y() override {
        _placed.clear();
        _lines.clear();
        // Single-line layout (no word-wrapping)
        int16_t cur_x = 0;
        int16_t cur_y = 0;
        uint16_t space_w = _f->space_width();
        uint16_t line_h = _rasterized.bounding_height;
        for (auto& word : _rasterized.words) {
          for (auto& g : word.glyphs)
            _placed.push_back({g.txr, int16_t(cur_x + g.offset_x), int16_t(cur_y + g.offset_y)});
          cur_x += word.bounding_width + word.separator_count * space_w;
        }
        _lines.push_back({uint16_t(cur_x), 0, _placed.size()});
        _config.sizing.y = FIXED(_f->height());
        element::on_fit_y();
      }

      std::vector<render_command*> on_render(bounding_box render_box) override {
        auto clipped = clip(render_box, _box);
        if (clipped.width <= 0 || clipped.height <= 0) return {};

        _reg_state->reset();
        auto content = _reg_state->get<std::string>("");
        auto cursor = _reg_state->get<int>(0);
        auto key_handle = _reg_state->get<std::shared_ptr<_detail::key>>(nullptr);
        auto ime_cb = _reg_state->get<ime_callback>(ime_callback{});
        auto processed = _reg_state->get<std::set<uint32_t>>({});
        auto composition = _reg_state->get<std::string>("");
        auto composition_cursor = _reg_state->get<int>(0);

        auto* renderer = _storage::current_registry.current_renderer;
        bool focused = renderer && renderer->get_focus_id() == id();

        std::vector<render_command*> cmds;

        // 1. IME render command (must be first)
        cmds.push_back(_storage::allocate<ime_render_command>(
          *ime_cb.operator->(),
          [=](std::string text, int cursor) {
            composition = text;
            composition_cursor = cursor;
          },
          clipped
        ));

        // 3. Text glyphs via parent text::on_render
        auto text_cmds = text::on_render(render_box);
        cmds.insert(cmds.end(), text_cmds.begin(), text_cmds.end());

        // Compute cursor X position inside padding
        int cursor_x = _box.x;
        if (int(cursor) > 0 && !_text.empty()) {
          std::string before = _text.substr(0, size_t(int(cursor)));
          auto before_r = _f->rasterize(before);
          cursor_x = _box.x + before_r.bounding_width;
        }

        // 4. Composition text (if any, only when focused)
        std::string comp = composition;
        if (!comp.empty() && focused) {
          auto comp_r = _f->rasterize(comp);
          for (auto& word : comp_r.words) {
            for (auto& g : word.glyphs) {
              if (!g.txr) continue;
              bounding_box gbox = {cursor_x + g.offset_x, _box.y + g.offset_y, 0, 0,
                                   uint16_t(g.txr->width()), uint16_t(g.txr->height())};
              auto clipped_g = clip(render_box, gbox);
              if (clipped_g.width <= 0 || clipped_g.height <= 0) continue;
              cmds.push_back(_storage::allocate<text_render_command>(
                g.txr, clipped_g, _cfg.col
              ));
            }
          }
        }

        // 5. Cursor (only when focused)
        if (focused) {
          uint16_t line_h = _rasterized.bounding_height > 0 ? _rasterized.bounding_height : uint16_t(16);
          bounding_box cursor_box = {cursor_x, _box.y, 0, 0, _icfg.cursor_width, line_h};
          auto clipped_c = clip(render_box, cursor_box);
          if (clipped_c.width > 0 && clipped_c.height > 0) {
            cmds.push_back(_storage::allocate<rect_render_command>(
              clipped_c, _icfg.cursor_col, 0, NONE, true,
              std::make_pair(_icfg.cursor_width, line_h)
            ));
          }
        }

        // 6. Click-to-focus button (last so it's topmost in z-order)
        cmds.push_back(_storage::allocate<button_render_command>(clipped,
          [this](uint16_t bits, std::pair<uint16_t, uint16_t>) {
            if (bits & LCLICK) {
              auto* r = _storage::current_registry.current_renderer;
              if (r) r->set_focus(this);
            }
          },
          LCLICK
        ));

        return cmds;
      }
    };
  }

  inline _detail::input* input(_detail::font* f, _config::input_config cfg = {.sizing = {GROW(), GROW()}}, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::input>(f, cfg, loc);
  }
}
