#ifndef _CPPREACT_TEXT_HPP
#define _CPPREACT_TEXT_HPP

#include <cstdint>
#include <istream>
#include <map>

#include "component.hpp"
#include "image.hpp"
#include "rect.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

namespace cppreact {
  struct text_render_data {
    int16_t x;
    int16_t y;
    uint32_t codepoint;
    texture* tex;
  };
  struct text_cache {
    texture tex;
    int16_t offset_x;
    int16_t advance_x;
    int16_t offset_y;
    int16_t advance_y;
  };
  // Represents a font, with shaping & bitmap rendering support
  class font {
    //uint16_t size;
    std::map<uint32_t, text_cache> cache;
    
    std::string font_data;
    stbtt_fontinfo font_info;
    
    hb_font_t* hb_font_info;

    int ascent;
    int descent;
    int lineGap;
    float_t scale;
    uint16_t size;
    public:
    font(std::istream& font_stream, uint16_t size) : size(size) {
      std::string font_data;
      font_stream.seekg(0, std::ios::end);
      font_data.resize(font_stream.tellg());
      font_stream.seekg(0, std::ios::beg);
      font_stream.read(&font_data[0], font_data.size());
      this->font_data = font_data;
      stbtt_InitFont(&font_info, (unsigned char*)this->font_data.data(), 0);

      scale = stbtt_ScaleForPixelHeight(&font_info, size);

      stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &lineGap);
      
      ascent = (ascent * scale);
      descent = (descent * scale);

      hb_blob_t* blob = hb_blob_create((const char*)this->font_data.data(), this->font_data.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr);
      hb_face_t* face = hb_face_create(blob, 0);
      hb_font_t* hb_font = hb_font_create(face);
      hb_font_set_scale(hb_font, size, size);
      hb_blob_destroy(blob);
      hb_face_destroy(face);

      //hb_ot_font_set_funcs(hb_font);

      this->hb_font_info = hb_font;

    }
    text_render_data render(uint32_t glyph) {
      uint32_t ch = glyph;
      text_render_data data;
      if (cache.find(ch) == cache.end()) {
        int width,height,xoff,yoff;
        unsigned char* img = stbtt_GetGlyphBitmap(&font_info, 0, scale,ch, &width, &height, &xoff, &yoff);
        texture tex;
        tex.set_size(width, height);
        
        for (int j = 0;j < height;j++) {
          for (int k = 0;k < width;k++) {
            tex[j][k] = {img[(j * width + k)],img[(j * width + k)],img[(j * width + k)],img[(j * width + k)]};
          }
        }
          
        stbtt_FreeBitmap(img, nullptr);

        text_cache cache_data;
        cache_data.tex = tex;
        cache_data.offset_x = xoff;
        cache_data.offset_y = yoff;
        cache[ch] = cache_data;
      }

      text_cache& c = cache[ch];
      data.tex = &c.tex;
      data.x = c.offset_x;
      data.y = c.offset_y;
      data.codepoint = ch;
      return data;
    }
    // std::vector<text_render_data> render(std::wstring text) {
    //   std::vector<text_render_data> result;
    //   int x = 0;
    //   for (size_t i = 0;i < text.length();i++) {
    //     uint32_t ch = text[i];
    //     text_render_data data = render(ch);
    //     data.x = x;
    //     x += cache[ch].advance_x;
    //     if ( i < text.length() - 1) {
    //       x += stbtt_GetCodepointKernAdvance(&font_info, ch, text[i+1]) * scale;
    //     }
    //     result.push_back(data);
    //   }
    //   return result;
    // }
    std::vector<text_render_data> render(std::string text) {
      std::vector<text_render_data> result;
      hb_buffer_t* buffer = hb_buffer_create();
      hb_buffer_add_utf8(buffer, text.c_str(), text.length(), 0, text.length());
      hb_buffer_guess_segment_properties(buffer);
      hb_shape(hb_font_info, buffer, nullptr, 0);
      unsigned int num_glyphs;
      hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &num_glyphs);
      hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &num_glyphs);
      hb_position_t x = 0;
      hb_position_t y = 0;
      for (unsigned int i = 0;i < num_glyphs;i++) {
        text_render_data data = render(infos[i].codepoint);
        int xoff = data.x;
        int yoff = data.y;
        data.x += x + positions[i].x_offset + xoff / 2;
        data.y += y + positions[i].y_offset + ascent; //y + positions[i].y_offset;
        x += positions[i].x_advance;
        y += positions[i].y_advance;
        result.push_back(data);
      }
      hb_buffer_destroy(buffer);
      return result;
    }
    std::vector<text_render_data> render(std::wstring text) {
      std::vector<text_render_data> result;
      hb_buffer_t* buffer = hb_buffer_create();
      hb_buffer_add_utf32(buffer, (uint32_t*)text.c_str(), text.length(), 0, text.length());
      hb_buffer_guess_segment_properties(buffer);
      hb_shape(hb_font_info, buffer, nullptr, 0);
      unsigned int num_glyphs;
      hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &num_glyphs);
      hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &num_glyphs);
      hb_position_t x = 0;
      hb_position_t y = 0;
      for (unsigned int i = 0;i < num_glyphs;i++) {
        text_render_data data = render(infos[i].codepoint);
        data.x = x + positions[i].x_offset;
        data.y = std::max((int)y + positions[i].y_offset - size,0) + size; //y + positions[i].y_offset;
        x += positions[i].x_advance;
        y += positions[i].y_advance;
        result.push_back(data);
      }
      hb_buffer_destroy(buffer);
      return result;
    }
  };
  // An Unicode-shaped text component
  class text : public component {
    font* f;
    layout_config orig_config;
    public:
    std::vector<text_render_data> render_output;
    text(font& f, std::string text, color col = {255,255,255,255},layout_config config = {}) : 
      component(config), f(&f), t(text), col(col) {
      orig_config = config;
      
    }
    color col;
    std::string t;
    void on_init_layout() override {
      render_output = f->render(t);
      config = orig_config;
      if (config.sizing.x.mode == _FIT) config.sizing.x.min = render_output.back().x + render_output.back().tex->size().first;
      if (config.sizing.y.mode == _FIT) config.sizing.y.min = render_output.back().y + render_output.back().tex->size().second;
      component::on_init_layout();
    }
  };
}

#endif
