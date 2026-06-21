#pragma once

/** @file sdl_renderer.hpp
 *  @brief SDL2 concrete renderer implementation */

#include "SDL_blendmode.h"
#include "SDL_keycode.h"
#include "sized_renderer.hpp"
#include "../internal/font.hpp"
#include "../internal/ime.hpp"
#include "../widgets/rect.hpp"
#include "../widgets/text.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cppreact {
  /** @brief SDL2-backed texture wrapping an SDL_Texture handle */
  class sdl_texture : public texture {
    SDL_Texture* _texture = nullptr; ///< Underlying SDL texture handle
  public:
    SDL_Texture* get_sdl_texture() const { return _texture; }
    /** @brief Construct an sdl_texture from an existing SDL_Texture
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @param txr SDL texture handle (ownership is transferred) */
    sdl_texture(int width, int height, SDL_Texture* txr) : texture(width, height), _texture(txr) {}
    /** @brief Free the underlying SDL texture */
    ~sdl_texture() { SDL_DestroyTexture(_texture); }
    /** @brief Read the colour of a single pixel
     * @param x X coordinate
     * @param y Y coordinate
     * @return Colour at (x, y), or default-initialised on failure */
    color get_pixel(int x, int y) override {
      void* raw;
      int pitch;
      if (SDL_LockTexture(_texture, nullptr, &raw, &pitch) != 0) return {};
      auto* px = static_cast<uint8_t*>(raw);
      size_t idx = size_t(y) * pitch + x * 4;
      color c = {};
      if (idx + 3 < size_t(pitch) * height())
        c = {px[idx], px[idx+1], px[idx+2], px[idx+3]};
      SDL_UnlockTexture(_texture);
      return c;
    }
    /** @brief Copy the full pixel data into a vector
     * @return Vector of interleaved RGBA bytes */
    std::vector<uint8_t> get_pixels() override {
      void* raw;
      int pitch;
      if (SDL_LockTexture(_texture, nullptr, &raw, &pitch) != 0) return {};
      auto* src = static_cast<uint8_t*>(raw);
      std::vector<uint8_t> result;
      result.reserve(size_t(pitch) * height());
      // Copy each row, accounting for possible stride padding in the SDL texture
      for (int y = 0; y < height(); y++) {
        auto* row = src + y * pitch;
        result.insert(result.end(), row, row + width() * 4);
      }
      SDL_UnlockTexture(_texture);
      return result;
    }
  };

  /** @brief SDL2-backed single-font face that creates sdl_texture instances */
  class sdl_single_font : public _detail::single_font {
    SDL_Renderer* _r; ///< SDL renderer used for texture creation
  public:
    /** @param r SDL renderer handle
     * @param path Font file path
     * @param size Font size in pixels */
    sdl_single_font(SDL_Renderer* r, std::string path, int size)
      : single_font(path, size), _r(r) {}
  protected:
    /** @brief Create an sdl_texture from rendered glyph pixels
     * @return Shared pointer to the created texture */
    std::unique_ptr<texture> get_texture(std::pair<uint16_t,uint16_t> size, std::vector<uint8_t> pixels) override {
      auto txr = SDL_CreateTexture(_r, SDL_PIXELFORMAT_RGBA32,
                                    SDL_TEXTUREACCESS_STREAMING, size.first, size.second);
      if (!txr) return nullptr;
      SDL_UpdateTexture(txr, nullptr, pixels.data(), size.first * 4);
      SDL_SetTextureBlendMode(txr, SDL_BLENDMODE_BLEND);
      return std::make_unique<sdl_texture>(size.first, size.second, txr);
    }
  };

  /** @brief Concrete SDL2 renderer with event loop and rendering commands */
  class sdl_renderer : public sized_renderer {
    SDL_Window* _window = nullptr;   ///< SDL window handle
    SDL_Renderer* _renderer = nullptr; ///< SDL renderer handle
    bool _running = false;             ///< Whether the main loop is still running
    bool is_shift_pressed = false;     ///< Whether the Shift key is currently pressed

    /** @brief Initialise SDL subsystems, create window and accelerated renderer
     * @param title Window title
     * @return true on success */
    bool init(std::string title) {
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
        return false;
      auto [w, h] = get_size();
      _window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
      if (!_window) return false;
      _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
      return _renderer != nullptr;
    }
  public:
    /** @brief Create an SDL window and initialise the renderer
     * @param size Window dimensions (width, height)
     * @param title Window title
     * @param loc Source location for debugging
     * @throws std::runtime_error if SDL initialisation fails */
    sdl_renderer(std::pair<uint16_t, uint16_t> size, std::string title = "cppreact", std::source_location loc = std::source_location::current()) :
      sized_renderer(size, loc) {
        if (!init(title)) throw std::runtime_error("Failed to initialize SDL");
      }
    /** @brief Shut down the renderer and quit SDL */
    ~sdl_renderer() { 
      shutdown();      
      SDL_Quit();
    }

    /** @brief Create an sdl_texture from raw RGBA pixel data
      * @return owner<texture> wrapping the new SDL texture */
    owner<texture> get_texture(uint16_t width, uint16_t height, std::vector<uint8_t> pixels) override {
      auto txr = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA32,
                                    SDL_TEXTUREACCESS_STREAMING, width, height);
      SDL_UpdateTexture(txr, nullptr, pixels.data(), width * 4);
      SDL_SetTextureBlendMode(txr, SDL_BLENDMODE_BLEND);
      return owner<texture>(std::make_unique<sdl_texture>(width, height, txr));
    }

    /** @brief Release all SDL resources and clear the texture cache */
    void shutdown() {
      if (!_renderer) return;
      clear_font_cache();
      _running = false;
      if (_renderer) SDL_DestroyRenderer(_renderer);
      if (_window) SDL_DestroyWindow(_window);
      _renderer = nullptr;
      _window = nullptr;
    }

    /** @brief Enter the SDL event loop
     *
     * Polls SDL events (quit, resize, mouse, keyboard, text input, IME),
     * updates internal state, clears the screen, runs one layout/render
     * frame, and presents the result.
     * @param root Root widget of the UI tree */
    void run(_detail::identifiable* root) override {
      _running = true;
      uint16_t mouse_mask = 0;
      int mouse_x = 0, mouse_y = 0;
      while (_running) {
        int32_t scroll_delta_x = 0, scroll_delta_y = 0;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
          if (event.type == SDL_QUIT)
            _running = false;
          else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            set_size(event.window.data1 < 1 ? 1 : event.window.data1,
                     event.window.data2 < 1 ? 1 : event.window.data2);
            SDL_RenderSetViewport(_renderer, NULL);
          }
          else if (event.type == SDL_MOUSEMOTION) {
            mouse_x = event.motion.x;
            mouse_y = event.motion.y;
          }
          else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) mouse_mask |= LCLICK;
            if (event.button.button == SDL_BUTTON_RIGHT) mouse_mask |= RCLICK;
            if (event.button.button == SDL_BUTTON_MIDDLE) mouse_mask |= MCLICK;
          }
          else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) mouse_mask &= ~LCLICK;
            if (event.button.button == SDL_BUTTON_RIGHT) mouse_mask &= ~RCLICK;
            if (event.button.button == SDL_BUTTON_MIDDLE) mouse_mask &= ~MCLICK;
          }
          else if (event.type == SDL_MOUSEWHEEL) {
            scroll_delta_x += event.wheel.x;
            scroll_delta_y += event.wheel.y;
          }
          else if (event.type == SDL_TEXTINPUT) {
            set_editing("", 0);
            // Decode UTF-8 bytes, normalise to uppercase, emit as key press/release
            for (const char* p = event.text.text; *p; ) {
              unsigned char c = (unsigned char)*p;
              uint32_t cp;
              if (c < 0x80) { cp = c; p += 1; }
              else if ((c & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) { cp = ((c & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
              else if ((c & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) { cp = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; }
              else if ((c & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) { cp = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); p += 4; }
              else { p++; continue; }
              //if (cp >= 'a' && cp <= 'z') cp -= 32;
              set_key(keycode(cp), true, true);
              set_key(keycode(cp), false, true);
            }
          }
          else if (event.type == SDL_TEXTEDITING) {
            set_editing(event.edit.text, event.edit.start);
          }
          else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            // if (event.key.repeat) continue;
            auto sym = event.key.keysym.sym;
            uint32_t sdl = uint32_t(sym);
            keycode kc = keycode::UNKNOWN;
            if (sdl == SDLK_LSHIFT || sdl == SDLK_RSHIFT) 
              is_shift_pressed = event.type == SDL_KEYDOWN;
            // Map SDL key symbols to internal keycodes
            if (sdl == 8)      kc = keycode::BACKSPACE;
            else if (sdl == 9)  kc = keycode::TAB;
            else if (sdl == 13) kc = keycode::ENTER;
            else if (sdl == 27) kc = keycode::ESCAPE;
            else if (sdl == 127) kc = keycode::DEL;
            else if (sdl >= 'a' && sdl <= 'z') {if (is_shift_pressed) kc = keycode(sdl - 32);}
            else if (sdl >= 0x20 && sdl <= 0x10FFFF) kc = keycode(sdl);
            else if (sdl >= 0x40000000) kc = keycode((sdl & 0xFFFF) + 0x110000);
            if (kc != keycode::UNKNOWN)
              set_key(kc, event.type == SDL_KEYDOWN, false);
          }
        }
        set_cursor(mouse_mask, mouse_x, mouse_y);
        set_scroll(scroll_delta_x, scroll_delta_y);
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);
        run_once(root);
        SDL_RenderPresent(_renderer);
      }
    }

  protected:
    /** @brief Load an image file as an sdl_texture
     * @param path File path to the image
     * @return Shared pointer to the texture, or nullptr on failure */
    virtual std::unique_ptr<texture> on_load_texture(const std::string& path) override {
      SDL_Surface* surface = IMG_Load(path.c_str());
      if (!surface) return nullptr;

      SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
      if (!rgba) return nullptr;

      SDL_Texture* txr = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGBA32,
                                            SDL_TEXTUREACCESS_STREAMING, rgba->w, rgba->h);
      if (!txr) { SDL_FreeSurface(rgba); return nullptr; }
      SDL_UpdateTexture(txr, nullptr, rgba->pixels, rgba->pitch);
      int w = rgba->w, h = rgba->h;
      SDL_FreeSurface(rgba);

      return std::make_unique<sdl_texture>(w, h, txr);
    }
    /** @brief Create an sdl_single_font for the given font file
     * @param path Font file path
     * @param size Font size in pixels
     * @return Shared pointer to the new single-font face */
    std::shared_ptr<_detail::single_font> on_load_single_font(const std::string& path, int size) override {
      return std::make_shared<sdl_single_font>(_renderer, path, size);
    }
    /** @brief Fill a rounded rectangle with clipping support
     *
     * Renders the visible portion of a rounded rectangle.  The original
     * (unclipped) extents are determined by (offset_x, offset_y) and
     * (original_width, original_height).  When @p fill is false only the
     * edges and corners are drawn, producing a stroked appearance.
     * @param r SDL renderer
     * @param x,y,w,h Visible (clipped) rectangle
     * @param radius Corner radius in pixels
     * @param ox,oy,ow,oh Original (unclipped) rectangle extents
     * @param fill true to fill centre, false for outline only */
    void fill_rounded_rect(SDL_Renderer* r, int x, int y, int w, int h, int radius,
                           int ox, int oy, int ow, int oh, bool fill) {
      radius = std::min({radius, ow / 2, oh / 2});
      if (radius <= 0) {
        SDL_Rect rr = {x, y, w, h};
        SDL_RenderFillRect(r, &rr);
        return;
      }
      int orig_left = x - ox;
      int orig_top = y - oy;
      int orig_right = orig_left + ow;
      int orig_bottom = orig_top + oh;
      int vis_right = x + w;
      int vis_bottom = y + h;

      // Center
      if (fill) {
        int cx0 = std::max(x, orig_left + radius);
        int cy0 = std::max(y, orig_top + radius);
        int cx1 = std::min(vis_right, orig_right - radius);
        int cy1 = std::min(vis_bottom, orig_bottom - radius);
        if (cy0 < cy1) {
          SDL_Rect cr = {cx0, cy0, cx1 - cx0, cy1 - cy0};
          SDL_RenderFillRect(r, &cr);
        }
      }

      // Left flat
      int lx0 = x;
      int lx1 = std::min(vis_right, orig_left + radius);
      if (lx0 < lx1) {
        int ly0 = std::max(y, orig_top + radius);
        int ly1 = std::min(vis_bottom, orig_bottom - radius);
        if (ly0 < ly1) {
          SDL_Rect lr = {lx0, ly0, lx1 - lx0, ly1 - ly0};
          SDL_RenderFillRect(r, &lr);
        }
      }

      // Right flat
      int rx0 = std::max(x, orig_right - radius);
      int rx1 = vis_right;
      if (rx0 < rx1) {
        int ry0 = std::max(y, orig_top + radius);
        int ry1 = std::min(vis_bottom, orig_bottom - radius);
        if (ry0 < ry1) {
          SDL_Rect rr = {rx0, ry0, rx1 - rx0, ry1 - ry0};
          SDL_RenderFillRect(r, &rr);
        }
      }

      // Top flat
      int ty0 = y;
      int ty1 = std::min(vis_bottom, orig_top + radius);
      if (ty0 < ty1) {
        int tx0 = std::max(x, orig_left + radius);
        int tx1 = std::min(vis_right, orig_right - radius);
        if (tx0 < tx1) {
          SDL_Rect tr = {tx0, ty0, tx1 - tx0, ty1 - ty0};
          SDL_RenderFillRect(r, &tr);
        }
      }

      // Bottom flat
      int by0 = std::max(y, orig_bottom - radius);
      int by1 = vis_bottom;
      if (by0 < by1) {
        int bx0 = std::max(x, orig_left + radius);
        int bx1 = std::min(vis_right, orig_right - radius);
        if (bx0 < bx1) {
          SDL_Rect br = {bx0, by0, bx1 - bx0, by1 - by0};
          SDL_RenderFillRect(r, &br);
        }
      }

      // Top-left corner
      int tl_y0 = std::max(y, orig_top);
      int tl_y1 = std::min(vis_bottom, orig_top + radius);
      int tl_x0 = std::max(x, orig_left);
      int tl_x1 = std::min(vis_right, orig_left + radius);
      if (tl_x0 < tl_x1 && tl_y0 < tl_y1) {
        for (int row = tl_y0; row < tl_y1; row++) {
          int i = row - orig_top;
          int dy = radius - i;
          int dx = int(sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_left + radius - dx, tl_x0);
          int l1 = std::min(orig_left + radius, tl_x1);
          if (l0 <= l1) SDL_RenderDrawLine(r, l0, row, l1, row);
        }
      }

      // Top-right corner
      int tr_y0 = std::max(y, orig_top);
      int tr_y1 = std::min(vis_bottom, orig_top + radius);
      int tr_x0 = std::max(x, orig_right - radius);
      int tr_x1 = std::min(vis_right, orig_right);
      if (tr_x0 < tr_x1 && tr_y0 < tr_y1) {
        for (int row = tr_y0; row < tr_y1; row++) {
          int i = row - orig_top;
          int dy = radius - i;
          int dx = int(sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_right - radius, tr_x0);
          int l1 = std::min(orig_right - radius + dx, tr_x1);
          if (l0 <= l1) SDL_RenderDrawLine(r, l0, row, l1, row);
        }
      }

      // Bottom-left corner
      int bl_y0 = std::max(y, orig_bottom - radius);
      int bl_y1 = std::min(vis_bottom, orig_bottom);
      int bl_x0 = std::max(x, orig_left);
      int bl_x1 = std::min(vis_right, orig_left + radius);
      if (bl_x0 < bl_x1 && bl_y0 < bl_y1) {
        for (int row = bl_y0; row < bl_y1; row++) {
          int i = orig_bottom - 1 - row;
          int dy = radius - 1 - i;
          int dx = int(sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_left + radius - dx, bl_x0);
          int l1 = std::min(orig_left + radius, bl_x1);
          if (l0 <= l1) SDL_RenderDrawLine(r, l0, row, l1, row);
        }
      }

      // Bottom-right corner
      int br_y0 = std::max(y, orig_bottom - radius);
      int br_y1 = std::min(vis_bottom, orig_bottom);
      int br_x0 = std::max(x, orig_right - radius);
      int br_x1 = std::min(vis_right, orig_right);
      if (br_x0 < br_x1 && br_y0 < br_y1) {
        for (int row = br_y0; row < br_y1; row++) {
          int i = orig_bottom - 1 - row;
          int dy = radius - 1 - i;
          int dx = int(sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_right - radius, br_x0);
          int l1 = std::min(orig_right - radius + dx, br_x1);
          if (l0 <= l1) SDL_RenderDrawLine(r, l0, row, l1, row);
        }
      }
    }

    /** @brief Execute a rectangle render command via SDL
     * @param cmd Rectangle render command */
    void on_rect_cmd(_detail::rect_render_command& cmd) override {
      SDL_SetRenderDrawColor(_renderer, cmd.col.r, cmd.col.g, cmd.col.b, cmd.col.a);
      SDL_SetRenderDrawBlendMode(_renderer,
          cmd.blend == ADD ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
      fill_rounded_rect(_renderer, cmd.box.x, cmd.box.y,
                        int(cmd.box.width), int(cmd.box.height),
                        cmd.radius,
                        cmd.box.offset_x, cmd.box.offset_y,
                        cmd.original_size.first, cmd.original_size.second, cmd.fill);
    }
    /** @brief Render an image texture via SDL_RenderCopy
     * @param cmd Image render command with source/destination rectangles */
    void on_image_cmd(_detail::image_render_command& cmd) override {
      SDL_Rect src = {int(cmd.box.offset_x), int(cmd.box.offset_y),
                      int(cmd.box.width), int(cmd.box.height)};
      SDL_Rect dst = {int(cmd.box.x), int(cmd.box.y),
                      int(cmd.box.width), int(cmd.box.height)};
      auto* stxr = static_cast<sdl_texture*>(cmd.txr);
      if (stxr) {
        SDL_SetTextureBlendMode(stxr->get_sdl_texture(),
            cmd.blend == ADD ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(_renderer, stxr->get_sdl_texture(), &src, &dst) != 0)
          fprintf(stderr, "SDL_RenderCopy error: %s\n", SDL_GetError());
      }
    }
    /** @brief Set up SDL text-input callbacks for an IME command
     * @param cmd IME render command receiving start/end callbacks */
    void on_ime_cmd(_detail::ime_render_command& cmd) override {
      SDL_Rect r = {cmd.box.x, cmd.box.y, cmd.box.width, cmd.box.height};
      cmd.callback.start = [this, r]() {
        SDL_SetTextInputRect(&r);
        SDL_StartTextInput();
      };
      cmd.callback.end = [this]() {
        SDL_StopTextInput();
        set_editing("", 0);
      };
    }
    /** @brief Render a glyph texture using SDL colour modulation
     * @param cmd Text render command containing glyph texture and colour */
    void on_text_cmd(_detail::text_render_command& cmd) override {
      SDL_Rect src = {int(cmd.box.offset_x), int(cmd.box.offset_y),
                      int(cmd.box.width), int(cmd.box.height)};
      SDL_Rect dst = {int(cmd.box.x), int(cmd.box.y),
                      int(cmd.box.width), int(cmd.box.height)};
      auto* stxr = static_cast<sdl_texture*>(cmd.glyphs);
      if (stxr) {
        SDL_SetTextureBlendMode(stxr->get_sdl_texture(), SDL_BLENDMODE_BLEND);
        if (!cmd.is_color)
          SDL_SetTextureColorMod(stxr->get_sdl_texture(), cmd.col.r, cmd.col.g, cmd.col.b);
        if (SDL_RenderCopy(_renderer, stxr->get_sdl_texture(), &src, &dst) != 0)
          fprintf(stderr, "SDL_RenderCopy error: %s\n", SDL_GetError());
        // Reset colour modulation to avoid affecting later renders of this texture
        SDL_SetTextureColorMod(stxr->get_sdl_texture(), 255, 255, 255);
      }
    }
  };
}
