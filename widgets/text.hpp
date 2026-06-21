/**
 * @file text.hpp
 * @brief Text rendering widget using the font system
 */
#pragma once
#include "../internal/component.hpp"
#include "../internal/arena.hpp"
#include "../internal/font.hpp"
#include <cstdint>
#include <vector>

namespace cppreact {

  namespace _config {
    /**
     * @brief Configuration structure for text rendering
     */
    struct text_config {
      color col = {255, 255, 255, 255}; ///< Text color
      blend_mode blend = NONE;          ///< Blend mode
      CPPREACT_ELEMENT_CONFIG;
    };
  }

  namespace _detail {
    /**
     * @brief Render command that draws a single glyph texture
     */
    class text_render_command : public render_command {
      public:
      texture* glyphs;     ///< The glyph texture to draw
      color col = {255, 255, 255, 255};    ///< Tint color for the glyph
      bool is_color = false; ///< Whether this glyph comes from a color font
      text_render_command(texture* glyphs, bounding_box box, color col, bool is_color = false) : render_command(box), glyphs(glyphs), col(col), is_color(is_color) {}
    };
    /**
     * @brief Text component that rasterizes and lays out glyphs from a font
     */
    class text : public component {
      protected:
      /**
       * @brief A single placed glyph with its screen position
       */
      struct placed_glyph { texture* txr; int16_t x; int16_t y; bool is_color = false; };
      _config::text_config _cfg;               ///< Text configuration
      font* _f;                                ///< The font to use (borrowed, caller keeps owner alive)
      std::string _text;                       ///< The text string to render
      _detail::sentence_output _rasterized;    ///< Rasterized glyph data from the font
      std::vector<placed_glyph> _placed;       ///< Laid-out glyph positions
      uint16_t _minimum_width = 0;             ///< Minimum width derived from word bounding widths
      struct line_extent { uint16_t width; size_t glyph_start; size_t glyph_count; };
      std::vector<line_extent> _lines;         ///< Per-line extents for alignment
    public:
      text(font* f, std::string_view text, _config::text_config cfg, std::source_location loc) :
        component(to_element_config(cfg), loc), _cfg(cfg), _f(f), _text(text) {}

      /**
       * @brief Rasterizes the text if not already done, then applies config
       */
      void on_update() override {
        if (!_f) return;
        // Rasterize on first update, tracking minimum width per word
        if (_minimum_width == 0) {
          _rasterized = _f->rasterize(_text);
          for (auto& word : _rasterized.words)
            _minimum_width = std::max(_minimum_width, word.bounding_width);
        }
        element::on_update();
        _config = to_element_config(_cfg);
      }

      void on_fit_x() override {
        if (_minimum_width > _config.sizing.x.min)
          _config.sizing.x.min = _minimum_width;
        if (_rasterized.bounding_width > _config.sizing.x.max)
        _config.sizing.x.max = _rasterized.bounding_width;
        element::on_fit_x();
      }

      /**
       * @brief Performs word-wrap layout and sets the computed height
       */
      void on_fit_y() override {
        _placed.clear();
        _lines.clear();
        // If no font or no rasterized words, zero height
        if (!_f || _rasterized.words.empty()) {
          _config.sizing.y = FIXED(uint16_t(std::max(0, _f->ascender() - _f->descender())));
          element::on_fit_y();
          return;
        }
        int16_t cur_x = 0;
        int16_t cur_y = 0;
        uint16_t line_h = uint16_t(std::max<int>(_rasterized.bounding_height, _f->ascender() - _f->descender()));
        uint16_t space_w = _f->space_width();
        size_t line_start = 0;
        for (auto& word : _rasterized.words) {
          // Wrap to next line if word doesn't fit
          if (cur_x > 0 && cur_x + word.bounding_width > _box.width) {
            _lines.push_back({uint16_t(cur_x), line_start, _placed.size() - line_start});
            line_start = _placed.size();
            cur_x = 0;
            cur_y += line_h;
          }
          for (auto& g : word.glyphs)
              _placed.push_back({g.txr, int16_t(cur_x + g.offset_x), int16_t(cur_y + g.offset_y), g.is_color});
          cur_x += word.bounding_width + word.separator_count * space_w;
        }
        _lines.push_back({uint16_t(cur_x), line_start, _placed.size() - line_start});
        _config.sizing.y = FIXED(uint16_t(std::max<int>(line_h, cur_y + line_h)));
        element::on_fit_y();
      }

      /**
       * @brief Positions glyphs horizontally based on alignment
       */
      void on_child_pos_x() override {
        if (_placed.empty() || _cfg.alignment.x == ALIGN_BEGIN) return component::on_child_pos_x();
        for (auto& ln : _lines) {
          if (ln.width >= _box.width) continue;
          int16_t offset = int16_t((int(_box.width) - int(ln.width)) * _cfg.alignment.x);
          if (offset <= 0) continue;
          for (size_t i = ln.glyph_start; i < ln.glyph_start + ln.glyph_count; i++)
            _placed[i].x += offset;
        }
      }

      /**
       * @brief Produces render commands for each visible glyph
       * @param render_box The parent render bounds
       * @return Vector of text_render_command for visible glyphs
       */
      std::vector<render_command*> on_render(bounding_box render_box) override {
        std::vector<render_command*> cmds;
        for (auto& g : _placed) {
          if (!g.txr) throw std::runtime_error("Missing texture");
          bounding_box gbox = {_box.x + g.x, _box.y + g.y, 0, 0,
                               uint16_t(g.txr->width()), uint16_t(g.txr->height())};
          auto clipped = clip(render_box, gbox);
          if (clipped.width <= 0 || clipped.height <= 0) continue;
          cmds.push_back(_storage::allocate<text_render_command>(
            g.txr, clipped, _cfg.col, g.is_color
          ));
        }
        return cmds;
      }
    };
  }

  /**
   * @brief Allocates and returns a new text widget
   * @param f The font to render with (borrowed, caller must keep the owner alive)
   * @param text The string to display
   * @param cfg Text configuration (color, blend, sizing)
   * @param loc Source location for debugging
   * @return Pointer to the allocated text component
   */
  inline _detail::text* text(_detail::font* f, std::string_view text, _config::text_config cfg = {.sizing = {GROW(), FIT()}}, std::source_location loc = std::source_location::current()) {
    cfg.sizing = {GROW(), FIT()};
    return _storage::allocate<_detail::text>(f, text, cfg, loc);
  }

}
