/** @file
 *  @brief Font system using FreeType + HarfBuzz for text rasterization and shaping.
 */

#pragma once

#include "texture.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cppreact {
  namespace _detail {

    /** @brief Get the singleton FreeType library instance (initialised on first call). */
    static FT_Library ft_library() {
      static FT_Library lib = nullptr;
      if (!lib) FT_Init_FreeType(&lib);
      return lib;
    }

    /** @brief Metrics for a single rasterized glyph. */
    struct glyph_metrics {
      int16_t bearing_x = 0;  ///< Horizontal bearing (left side bearing)
      int16_t bearing_y = 0;  ///< Vertical bearing (top side bearing)
      int16_t advance_x = 0;  ///< Horizontal advance width
      uint16_t width = 0;     ///< Glyph bitmap width
      uint16_t height = 0;    ///< Glyph bitmap height
    };

    /** @brief Represents a single font face at a given size with bold/italic variants.
     *
     *  Wraps a FreeType face and corresponding HarfBuzz font objects.
     *  Provides glyph rasterization, metric calculation, and caching.
     */
    class single_font {
      FT_Face _face = nullptr;      ///< FreeType face handle
      hb_face_t* _hb_face = nullptr;///< HarfBuzz face handle
      hb_font_t* _hb_font = nullptr;///< HarfBuzz font handle
      int _size;        ///< Requested font size in pixels
      bool _bold;       ///< Whether bold style is active
      bool _italic;     ///< Whether italic style is active
      float _scale = 1.0f; ///< Scale factor for bitmap-only fonts

      /** @brief A rasterized glyph with its metrics and texture. */
      struct cached_glyph {
        glyph_metrics metrics;               ///< Glyph metrics
        std::unique_ptr<cppreact::texture> txr; ///< Rendered glyph texture
      };
      std::map<uint32_t, cached_glyph> _cache; ///< Glyph ID -> cached glyph

    public:
      /** @brief Load a font from a file path.
       *  @param path Filesystem path to the font file.
       *  @param size Requested pixel size.
       *  @param bold Optional override for bold style (auto-detected if omitted).
       *  @param italic Optional override for italic style (auto-detected if omitted).
       */
      single_font(std::filesystem::path path, int size, std::optional<bool> bold = std::nullopt, std::optional<bool> italic = std::nullopt)
        : _size(size), _bold(false), _italic(false) {
        if (FT_New_Face(ft_library(), path.c_str(), 0, &_face) == 0) {
          if (_face->units_per_EM == 0 && _face->num_fixed_sizes > 0) {
            FT_Select_Size(_face, 0);
            auto sh = _face->available_sizes[0].height;
            if (sh > 0) _scale = float(size) / float(sh);
          } else {
            FT_Set_Pixel_Sizes(_face, 0, size);
          }
          _bold = bold.value_or(_face->style_flags & FT_STYLE_FLAG_BOLD);
          _italic = italic.value_or(_face->style_flags & FT_STYLE_FLAG_ITALIC);
          _hb_face = hb_ft_face_create(_face, nullptr);
          _hb_font = hb_ft_font_create(_face, nullptr);
        }
      }
      /** @brief Destroy the font, releasing FreeType and HarfBuzz resources. */
      virtual ~single_font() {
        if (_hb_font) hb_font_destroy(_hb_font);
        if (_hb_face) hb_face_destroy(_hb_face);
        if (_face) FT_Done_Face(_face);
      }
      single_font(const single_font&) = delete;
      single_font& operator=(const single_font&) = delete;
      single_font(single_font&& other) noexcept
        : _face(other._face), _hb_face(other._hb_face), _hb_font(other._hb_font),
          _size(other._size), _bold(other._bold), _italic(other._italic),
          _scale(other._scale), _cache(std::move(other._cache)) {
        other._face = nullptr;
        other._hb_face = nullptr;
        other._hb_font = nullptr;
      }

      /** @brief Check if this font has a glyph for the given codepoint with the requested styles.
       *  @param codepoint Unicode codepoint.
       *  @param bold Whether bold is required.
       *  @param italic Whether italic is required.
       */
      bool has_glyph(uint32_t codepoint, bool bold, bool italic) const {
        if (!_face) return false;
        if (!bold && _bold) return false;
        if (!italic && _italic) return false;
        uint32_t gid = FT_Get_Char_Index(_face, codepoint);
        if (!gid) return false;
        if (_scale < 1.0f && FT_Load_Glyph(_face, gid, FT_LOAD_COLOR) == 0)
          if (_face->glyph->metrics.horiAdvance == 0) return false;
        return true;
      }

      /** @brief Get the glyph index for a codepoint. */
      uint32_t glyph_index(uint32_t codepoint) const {
        return _face ? FT_Get_Char_Index(_face, codepoint) : 0;
      }

      /** @brief Look up or rasterize a glyph, caching the result.
       *  @param glyph_id The glyph index to rasterize.
       *  @return The cached or freshly rasterized glyph.
       */
      cached_glyph& get_or_rasterize(uint32_t glyph_id) {
        auto it = _cache.find(glyph_id);
        if (it != _cache.end()) return it->second;

        cached_glyph cg = {calc_metrics(glyph_id), render_glyph(glyph_id)};
        _cache.emplace(glyph_id, std::move(cg));
        return _cache[glyph_id];
      }

      hb_font_t* hb_font() const { return _hb_font; }
      int size() const { return _size; }
      bool bold() const { return _bold; }
      bool italic() const { return _italic; }
      float advance_scale() const { return _scale; }
      int ascender() const { return _face ? _face->size->metrics.ascender >> 6 : 0; }
      int descender() const { return _face ? _face->size->metrics.descender >> 6 : 0; }

    protected:
      /** @brief Create a texture from raw pixel data. Implemented by renderer backends. */
      virtual std::unique_ptr<texture> get_texture(std::pair<uint16_t,uint16_t>, std::vector<uint8_t>) = 0;

    private:
      /** @brief Compute glyph metrics from FreeType without rendering. */
      glyph_metrics calc_metrics(uint32_t glyph_id) {
        glyph_metrics m = {};
        if (!_face) return m;
        if (FT_Load_Glyph(_face, glyph_id, FT_HAS_COLOR(_face) ? FT_LOAD_COLOR : FT_LOAD_NO_BITMAP)) return m;
        m.bearing_x = int16_t((_face->glyph->metrics.horiBearingX >> 6) * _scale);
        m.bearing_y = int16_t((_face->glyph->metrics.horiBearingY >> 6) * _scale);
        m.advance_x = int16_t((_face->glyph->metrics.horiAdvance >> 6) * _scale);
        m.width = uint16_t((_face->glyph->metrics.width >> 6) * _scale);
        m.height = uint16_t((_face->glyph->metrics.height >> 6) * _scale);
        return m;
      }

      /** @brief Render a glyph to a texture, handling BGRA and grayscale bitmaps. */
      std::unique_ptr<texture> render_glyph(uint32_t glyph_id) {
        if (!_face) return nullptr;
        int load_flags = FT_LOAD_RENDER;
        if (FT_HAS_COLOR(_face)) load_flags |= FT_LOAD_COLOR;
        if (FT_Load_Glyph(_face, glyph_id, load_flags)) return nullptr;
        auto& bm = _face->glyph->bitmap;
        if (!bm.buffer || !bm.width || !bm.rows) return nullptr;
        std::vector<uint8_t> rgba(bm.width * bm.rows * 4);
        if (bm.pixel_mode == FT_PIXEL_MODE_BGRA) {
          for (FT_Int y = 0; y < bm.rows; y++) {
            auto* src = bm.buffer + y * bm.pitch;
            for (FT_Int x = 0; x < bm.width; x++) {
              size_t d = (size_t(y) * bm.width + x) * 4;
              uint8_t b = src[x*4], g = src[x*4+1], r = src[x*4+2], a = src[x*4+3];
              // Un-premultiply alpha
              if (a) { r = uint8_t(unsigned(r) * 255 / a); g = uint8_t(unsigned(g) * 255 / a); b = uint8_t(unsigned(b) * 255 / a); }
              rgba[d] = r; rgba[d+1] = g; rgba[d+2] = b; rgba[d+3] = a;
            }
          }
        } else {
          // Greyscale bitmap — write white with alpha from the pixel value
          for (FT_Int y = 0; y < bm.rows; y++) {
            auto* row = bm.buffer + y * bm.pitch;
            for (FT_Int x = 0; x < bm.width; x++) {
              size_t d = (size_t(y) * bm.width + x) * 4;
              rgba[d] = rgba[d+1] = rgba[d+2] = 255;
              rgba[d+3] = row[x];
            }
          }
        }
        // Scale bitmap down for bitmap-only fonts (e.g., CBDT color emoji)
        if (_scale < 1.0f) {
          uint16_t sw = uint16_t(bm.width * _scale);
          uint16_t sh = uint16_t(bm.rows * _scale);
          if (sw == 0) sw = 1;
          if (sh == 0) sh = 1;
          std::vector<uint8_t> scaled(sw * sh * 4);
          for (uint16_t y = 0; y < sh; y++) {
            float sy = float(y) / _scale;
            uint16_t syi = uint16_t(sy);
            if (syi >= bm.rows) syi = bm.rows - 1;
            for (uint16_t x = 0; x < sw; x++) {
              float sx = float(x) / _scale;
              uint16_t sxi = uint16_t(sx);
              if (sxi >= bm.width) sxi = bm.width - 1;
              size_t sd = (size_t(y) * sw + x) * 4;
              size_t dd = (size_t(syi) * bm.width + sxi) * 4;
              scaled[sd] = rgba[dd];
              scaled[sd+1] = rgba[dd+1];
              scaled[sd+2] = rgba[dd+2];
              scaled[sd+3] = rgba[dd+3];
            }
          }
          return get_texture({sw, sh}, scaled);
        }
        return get_texture({bm.width, bm.rows}, rgba);
      }
    };

    /** @brief A positioned glyph output with texture and screen offset. */
    struct glyph_output {
      cppreact::texture* txr; ///< Glyph texture
      int16_t offset_x; ///< Horizontal screen offset
      int16_t offset_y; ///< Vertical screen offset
    };

    /** @brief A word consisting of positioned glyphs, with bounding box and separator info. */
    struct word_output {
      std::vector<glyph_output> glyphs;     ///< Glyphs in this word
      uint16_t bounding_width = 0;          ///< Word bounding width
      uint16_t bounding_height = 0;         ///< Word bounding height
      uint8_t separator_count = 0;          ///< Number of separators before this word
    };

    /** @brief A sentence (line of text) with words and overall bounding box. */
    struct sentence_output {
      std::vector<word_output> words;  ///< Words in this sentence
      uint16_t bounding_width = 0;     ///< Overall bounding width
      uint16_t bounding_height = 0;    ///< Overall bounding height
    };

    /** @brief Multi-font text engine using HarfBuzz shaping and FreeType rasterization.
     *
     *  Holds a collection of single_fonts, selects the best font per
     *  glyph, performs Unicode text shaping, produces positioned glyph
     *  output grouped into words and sentences.
     */
    class font {
      std::vector<std::shared_ptr<single_font>> _fonts; ///< Available font faces
      std::string _separator; ///< Characters treated as word separators
      bool _bold;   ///< Default bold style
      bool _italic; ///< Default italic style

      /** @brief A glyph resolved to a specific font and glyph index. */
      struct font_glyph {
        single_font* font = nullptr; ///< The font containing this glyph
        uint32_t glyph_id = 0;       ///< Glyph index within that font
      };

      /** @brief Find the best font for a codepoint, falling back to a "missing glyph" box (U+25A1). */
      font_glyph get_glyph(uint32_t codepoint) {
        for (auto& f : _fonts)
          if (f->has_glyph(codepoint, _bold, _italic)) {
            uint32_t gid = f->glyph_index(codepoint);
            if (gid) return {f.get(), gid};
          }
        for (auto& f : _fonts)
          if (f->has_glyph(0x25A1, _bold, _italic)) {
            uint32_t gid = f->glyph_index(0x25A1);
            if (gid) return {f.get(), gid};
          }
        return {};
      }

      /** @brief Decode a single UTF-8 codepoint, advancing the index.
       *  @param s The input string view.
       *  @param i Reference to the current byte index (updated in place).
       *  @return The decoded Unicode codepoint, or 0xFFFD on error.
       */
      static uint32_t utf8_decode(std::string_view s, size_t& i) {
        auto c = (unsigned char)s[i];
        if (c < 0x80) { i += 1; return c; }
        if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) { i += 2; return ((c & 0x1F) << 6) | (s[i-1] & 0x3F); }
        if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) { i += 3; return ((c & 0x0F) << 12) | ((s[i-2] & 0x3F) << 6) | (s[i-1] & 0x3F); }
        if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) { i += 4; return ((c & 0x07) << 18) | ((s[i-3] & 0x3F) << 12) | ((s[i-2] & 0x3F) << 6) | (s[i-1] & 0x3F); }
        i += 1; return 0xFFFD;
      }

    public:
      /** @brief Construct a multi-font collection.
       *  @param fonts List of font faces in priority order.
       *  @param bold Default bold style flag.
       *  @param italic Default italic style flag.
       *  @param separator Characters treated as word separators.
       */
      font(std::vector<std::shared_ptr<single_font>>&& fonts, bool bold = false, bool italic = false, std::string separator = " ")
        : _fonts(fonts), _bold(bold), _italic(italic), _separator(separator) {}

      /** @brief Measure the width of a space character. */
      uint16_t space_width() {
        auto fg = get_glyph(0x20);
        if (fg.font) return fg.font->get_or_rasterize(fg.glyph_id).metrics.advance_x;
        return 8;
      }

      /** @brief Rasterize a UTF-8 string into positioned words with full shaping.
       *
       *  Decodes to codepoints, groups consecutive codepoints by best-font match,
       *  shapes each run with HarfBuzz, splits into words by separators,
       *  and computes bounding boxes.
       *
       *  @param text The UTF-8 input string.
       *  @return A sentence_output with per-word and overall bounding boxes.
       */
      sentence_output rasterize(std::string_view text) {
        sentence_output out;
        if (_fonts.empty() || text.empty()) return out;

        // 1. Decode to codepoints
        std::vector<uint32_t> cps;
        for (size_t i = 0; i < text.size();)
          cps.push_back(utf8_decode(text, i));

        // 2. Group consecutive codepoints by best font
        struct run { single_font* font; size_t start; size_t count; };
        std::vector<run> runs;
        for (size_t i = 0; i < cps.size();) {
          single_font* best = nullptr;
          for (auto& f : _fonts)
            if (f->has_glyph(cps[i], _bold, _italic)) { best = f.get(); break; }
          if (!best) { i++; continue; }
          size_t start = i;
          while (i < cps.size() && best->has_glyph(cps[i], _bold, _italic)) i++;
          runs.push_back({best, start, i - start});
        }

        // 3. Shape each run, accumulate positioned glyphs
        struct pos { uint32_t cp; single_font* font; uint32_t glyph_id; int16_t x, y; int16_t adv; };
        std::vector<pos> positioned;
        int16_t cursor_x = 0;
        for (auto& r : runs) {
          std::string u8;
          for (size_t j = 0; j < r.count; j++) {
            auto cp = cps[r.start + j];
            if (cp < 0x80) u8 += (char)cp;
            else if (cp < 0x800) { u8 += (char)(0xC0 | (cp >> 6)); u8 += (char)(0x80 | (cp & 0x3F)); }
            else if (cp < 0x10000) { u8 += (char)(0xE0 | (cp >> 12)); u8 += (char)(0x80 | ((cp >> 6) & 0x3F)); u8 += (char)(0x80 | (cp & 0x3F)); }
            else { u8 += (char)(0xF0 | (cp >> 18)); u8 += (char)(0x80 | ((cp >> 12) & 0x3F)); u8 += (char)(0x80 | ((cp >> 6) & 0x3F)); u8 += (char)(0x80 | (cp & 0x3F)); }
          }
          auto* buf = hb_buffer_create();
          hb_buffer_add_utf8(buf, u8.c_str(), -1, 0, -1);
          hb_buffer_guess_segment_properties(buf);
          hb_shape(r.font->hb_font(), buf, nullptr, 0);
          unsigned n;
          auto* infos = hb_buffer_get_glyph_infos(buf, &n);
          auto* pos_ = hb_buffer_get_glyph_positions(buf, &n);
          float asc = r.font->advance_scale();
          for (unsigned i = 0; i < n; i++) {
            size_t idx = r.start + infos[i].cluster;
            if (idx >= cps.size()) idx = cps.size() - 1;
            int16_t adv = int16_t(((double)pos_[i].x_advance / 64) * asc);
            positioned.push_back({cps[idx], r.font, uint32_t(infos[i].codepoint),
              int16_t(cursor_x + ((double)pos_[i].x_offset / 64) * asc), int16_t(((double)pos_[i].y_offset / 64) * asc),
              adv});
            cursor_x += adv;
          }
          hb_buffer_destroy(buf);
        }

        // Fallback per-codepoint naive advance if shaping produced nothing
        if (positioned.empty())
          for (auto cp : cps) {
            auto fg = get_glyph(cp);
            if (fg.font) {
              auto& e = fg.font->get_or_rasterize(fg.glyph_id);
              positioned.push_back({cp, fg.font, fg.glyph_id, cursor_x, 0, int16_t(e.metrics.advance_x)});
              cursor_x += e.metrics.advance_x;
            }
          }

        // 4. Split into words, rasterize, compute offsets
        word_output cur;
        int16_t word_origin = 0;
        uint8_t consec_sep = 0;
        for (auto& p : positioned) {
          bool sep = false;
          for (char s : _separator) if (p.cp == (uint32_t)(unsigned char)s) { sep = true; break; }
          if (sep) {
            consec_sep++;
            if (!cur.glyphs.empty()) {
              for (auto& g : cur.glyphs) g.offset_x -= word_origin;
              out.words.push_back(cur);
              cur = word_output();
            }
            continue;
          }
          if (consec_sep && !out.words.empty())
            out.words.back().separator_count = consec_sep;
          consec_sep = 0;
          if (cur.glyphs.empty()) word_origin = p.x;
          auto& e = p.font->get_or_rasterize(p.glyph_id);
          if (e.txr)
            cur.glyphs.push_back({e.txr.get(), int16_t(p.x + e.metrics.bearing_x), int16_t(p.y - e.metrics.bearing_y)});
        }
        if (!cur.glyphs.empty()) {
          if (consec_sep)
            cur.separator_count = consec_sep;
          for (auto& g : cur.glyphs) g.offset_x -= word_origin;
          out.words.push_back(cur);
        }

        // 5. Compute per-word and sentence bounding boxes
        uint16_t sep_adv = 0;
        if (!_separator.empty()) {
          auto fg = get_glyph((unsigned char)_separator[0]);
          if (fg.font) sep_adv = fg.font->get_or_rasterize(fg.glyph_id).metrics.advance_x;
        }
        int16_t sentence_right = 0;
        int16_t global_min_y = 0;
        for (auto& w : out.words) {
          sentence_right += int16_t(w.separator_count) * sep_adv;
          int16_t w_max_r = 0, w_min_y = 0, w_max_b = 0;
          for (auto& g : w.glyphs) {
            int wd = g.txr ? g.txr->width() : 0;
            int ht = g.txr ? g.txr->height() : 0;
            w_max_r = std::max<int16_t>(w_max_r, g.offset_x + wd);
            w_min_y = std::min<int16_t>(w_min_y, g.offset_y);
            w_max_b = std::max<int16_t>(w_max_b, g.offset_y + ht);
          }
          global_min_y = std::min(global_min_y, w_min_y);
          w.bounding_width = uint16_t(std::max<int16_t>(0, w_max_r));
          sentence_right += w.bounding_width;
        }
        for (auto& w : out.words) {
          int16_t w_max_b = 0;
          for (auto& g : w.glyphs) {
            g.offset_y -= global_min_y;
            w_max_b = std::max<int16_t>(w_max_b, int16_t(g.offset_y + (g.txr ? g.txr->height() : 0)));
          }
          w.bounding_height = uint16_t(std::max<int16_t>(0, w_max_b));
        }
        out.bounding_width = uint16_t(std::max<int16_t>(0, sentence_right));
        for (auto& w : out.words)
          out.bounding_height = std::max(out.bounding_height, w.bounding_height);

        return out;
      }
    };

  }
}
