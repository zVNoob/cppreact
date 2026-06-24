#pragma once

/** @file sfml_renderer.hpp
 *  @brief SFML concrete renderer implementation */

#include "sized_renderer.hpp"
#include "../internal/font.hpp"
#include "../widgets/rect.hpp"
#include "../widgets/text.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cppreact {

  /** @brief SFML-backed texture wrapping an sf::Texture handle */
  class sfml_texture : public texture {
    sf::Texture _texture;
  public:
    sf::Texture& get_sfml_texture() { return _texture; }

    sfml_texture(sf::Texture txr) : texture(txr.getSize().x, txr.getSize().y), _texture(std::move(txr)) {}

    color get_pixel(int x, int y) override {
      auto img = _texture.copyToImage();
      auto c = img.getPixel(x, y);
      return {c.r, c.g, c.b, c.a};
    }

    std::vector<uint8_t> get_pixels() override {
      auto img = _texture.copyToImage();
      auto size = img.getSize();
      std::vector<uint8_t> result;
      result.reserve(size_t(size.x) * size.y * 4);
      for (unsigned y = 0; y < size.y; y++)
        for (unsigned x = 0; x < size.x; x++) {
          auto c = img.getPixel(x, y);
          result.push_back(c.r);
          result.push_back(c.g);
          result.push_back(c.b);
          result.push_back(c.a);
        }
      return result;
    }
  };

  /** @brief SFML-backed single-font face that creates sfml_texture instances */
  class sfml_single_font : public _detail::single_font {
  public:
    sfml_single_font(std::string path, int size)
      : single_font(path, size) {}
  protected:
    std::unique_ptr<texture> get_texture(std::pair<uint16_t,uint16_t> size, std::vector<uint8_t> pixels) override {
      sf::Texture txr;
      txr.create(size.first, size.second);
      txr.update(pixels.data());
      return std::make_unique<sfml_texture>(std::move(txr));
    }
  };

  /** @brief Concrete SFML renderer with event loop and rendering commands */
  class sfml_renderer : public sized_renderer {
    sf::RenderWindow _window;
    bool _running = false;

    /** @brief Map SFML key codes to internal keycode enum values
     *  @param key SFML keyboard key
     *  @return Corresponding internal keycode, or UNKNOWN if unmapped */
    keycode sfml_key_to_keycode(sf::Keyboard::Key key) {
      switch (key) {
        case sf::Keyboard::Enter:     return keycode::ENTER;
        case sf::Keyboard::Escape:    return keycode::ESCAPE;
        case sf::Keyboard::Backspace: return keycode::BACKSPACE;
        case sf::Keyboard::Tab:       return keycode::TAB;
        case sf::Keyboard::Delete:    return keycode::DEL;
        case sf::Keyboard::LControl:  return keycode::LCTRL;
        case sf::Keyboard::RControl:  return keycode::RCTRL;
        case sf::Keyboard::LShift:    return keycode::LSHIFT;
        case sf::Keyboard::RShift:    return keycode::RSHIFT;
        case sf::Keyboard::LAlt:      return keycode::LALT;
        case sf::Keyboard::RAlt:      return keycode::RALT;
        case sf::Keyboard::LSystem:   return keycode::LGUI;
        case sf::Keyboard::RSystem:   return keycode::RGUI;
        case sf::Keyboard::Up:        return keycode::UP;
        case sf::Keyboard::Down:      return keycode::DOWN;
        case sf::Keyboard::Left:      return keycode::LEFT;
        case sf::Keyboard::Right:     return keycode::RIGHT;
        case sf::Keyboard::F1:        return keycode::F1;
        case sf::Keyboard::F2:        return keycode::F2;
        case sf::Keyboard::F3:        return keycode::F3;
        case sf::Keyboard::F4:        return keycode::F4;
        case sf::Keyboard::F5:        return keycode::F5;
        case sf::Keyboard::F6:        return keycode::F6;
        case sf::Keyboard::F7:        return keycode::F7;
        case sf::Keyboard::F8:        return keycode::F8;
        case sf::Keyboard::F9:        return keycode::F9;
        case sf::Keyboard::F10:       return keycode::F10;
        case sf::Keyboard::F11:       return keycode::F11;
        case sf::Keyboard::F12:       return keycode::F12;
        case sf::Keyboard::Numpad0:   return keycode::KP_0;
        case sf::Keyboard::Numpad1:   return keycode::KP_1;
        case sf::Keyboard::Numpad2:   return keycode::KP_2;
        case sf::Keyboard::Numpad3:   return keycode::KP_3;
        case sf::Keyboard::Numpad4:   return keycode::KP_4;
        case sf::Keyboard::Numpad5:   return keycode::KP_5;
        case sf::Keyboard::Numpad6:   return keycode::KP_6;
        case sf::Keyboard::Numpad7:   return keycode::KP_7;
        case sf::Keyboard::Numpad8:   return keycode::KP_8;
        case sf::Keyboard::Numpad9:   return keycode::KP_9;
        case sf::Keyboard::Divide:    return keycode::KP_DIV;
        case sf::Keyboard::Multiply:  return keycode::KP_MUL;
        case sf::Keyboard::Subtract:  return keycode::KP_MINUS;
        case sf::Keyboard::Add:       return keycode::KP_PLUS;
        default: return keycode::UNKNOWN;
      }
    }

  public:
    /** @brief Create an SFML window and initialise the renderer
     *  @param size Window dimensions (width, height)
     *  @param title Window title
     *  @param loc Source location for debugging */
    sfml_renderer(std::pair<uint16_t, uint16_t> size, std::string title = "cppreact", std::source_location loc = std::source_location::current()) :
      sized_renderer(size, loc), _window(sf::VideoMode(size.first, size.second), title) {
      _window.setFramerateLimit(60);
    }

    ~sfml_renderer() {
      shutdown();
    }

    /** @brief Create an sfml_texture from raw RGBA pixel data
     *  @return owner<texture> wrapping the new SFML texture */
    owner<texture> get_texture(uint16_t width, uint16_t height, std::vector<uint8_t> pixels) override {
      sf::Texture txr;
      if (!txr.create(width, height)) return {};
      txr.update(pixels.data());
      return owner<texture>(std::make_unique<sfml_texture>(std::move(txr)));
    }

    /** @brief Release all SFML resources and clear the texture cache */
    void shutdown() {
      _running = false;
      if (_window.isOpen())
        _window.close();
    }

    /** @brief Enter the SFML event loop
     *
     *  Polls SFML events (close, resize, mouse, keyboard, text input),
     *  updates internal state, clears the screen, runs one layout/render
     *  frame, and presents the result.
     *  @param root Root widget of the UI tree */
    void run(_detail::identifiable* root) override {
      _running = true;
      uint16_t mouse_mask = 0;
      int mouse_x = 0, mouse_y = 0;
      while (_running && _window.isOpen()) {
        int32_t scroll_delta_x = 0, scroll_delta_y = 0;
        sf::Event event;
        while (_window.pollEvent(event)) {
          if (event.type == sf::Event::Closed) {
            _running = false;
            _window.close();
          } else if (event.type == sf::Event::Resized) {
            uint16_t w = std::max(1u, event.size.width);
            uint16_t h = std::max(1u, event.size.height);
            set_size(w, h);
            sf::View view(sf::FloatRect(0, 0, w, h));
            _window.setView(view);
          } else if (event.type == sf::Event::MouseMoved) {
            mouse_x = event.mouseMove.x;
            mouse_y = event.mouseMove.y;
          } else if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) mouse_mask |= LCLICK;
            if (event.mouseButton.button == sf::Mouse::Right) mouse_mask |= RCLICK;
            if (event.mouseButton.button == sf::Mouse::Middle) mouse_mask |= MCLICK;
          } else if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Left) mouse_mask &= ~LCLICK;
            if (event.mouseButton.button == sf::Mouse::Right) mouse_mask &= ~RCLICK;
            if (event.mouseButton.button == sf::Mouse::Middle) mouse_mask &= ~MCLICK;
          } else if (event.type == sf::Event::MouseWheelScrolled) {
            if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel)
              scroll_delta_y += int32_t(event.mouseWheelScroll.delta);
            else if (event.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel)
              scroll_delta_x += int32_t(event.mouseWheelScroll.delta);
          } else if (event.type == sf::Event::TextEntered) {
            uint32_t cp = event.text.unicode;
            set_key(keycode(cp), true, true);
            set_key(keycode(cp), false, true);
          } else if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased) {
            auto code = event.key.code;
            keycode kc = keycode::UNKNOWN;
            if (code >= sf::Keyboard::A && code <= sf::Keyboard::Z)
              kc = keycode('A' + (code - sf::Keyboard::A));
            else if (code >= sf::Keyboard::Num0 && code <= sf::Keyboard::Num9)
              kc = keycode('0' + (code - sf::Keyboard::Num0));
            else if (code == sf::Keyboard::Space)
              kc = keycode::SPACE;
            else
              kc = sfml_key_to_keycode(code);
            if (kc != keycode::UNKNOWN)
              set_key(kc, event.type == sf::Event::KeyPressed, false);
          }
        }
        set_cursor(mouse_mask, mouse_x, mouse_y);
        set_scroll(scroll_delta_x, scroll_delta_y);
        _window.clear(sf::Color::Black);
        run_once(root);
        _window.display();
      }
    }

  protected:
    /** @brief Load an image file as an sfml_texture
     *  @param path File path to the image
     *  @return Shared pointer to the texture, or nullptr on failure */
    std::unique_ptr<texture> on_load_texture(const std::string& path) override {
      sf::Texture txr;
      if (!txr.loadFromFile(path)) return nullptr;
      return std::make_unique<sfml_texture>(std::move(txr));
    }

    /** @brief Create an sfml_single_font for the given font file
     *  @param path Font file path
     *  @param size Font size in pixels
     *  @return Shared pointer to the new single-font face */
    std::shared_ptr<_detail::single_font> on_load_single_font(const std::string& path, int size) override {
      return std::make_shared<sfml_single_font>(path, size);
    }

    /** @brief Select an SFML blend mode from the internal blend_mode enum
     *  @param blend Internal blend mode
     *  @return Corresponding SFML blend mode */
    sf::BlendMode to_sfml_blend(blend_mode blend) {
      return blend == ADD ? sf::BlendAdd : sf::BlendAlpha;
    }

    /** @brief Execute a rectangle render command via SFML
     *  @param cmd Rectangle render command */
    void on_rect_cmd(_detail::rect_render_command& cmd) override {
      sf::Color col(cmd.col.r, cmd.col.g, cmd.col.b, cmd.col.a);
      sf::BlendMode bm = to_sfml_blend(cmd.blend);
      sf::RenderStates states(bm);

      int x = cmd.box.x, y = cmd.box.y;
      int w = int(cmd.box.width), h = int(cmd.box.height);
      int ox = cmd.box.offset_x, oy = cmd.box.offset_y;
      int ow = cmd.original_size.first, oh = cmd.original_size.second;
      int radius = std::min({int(cmd.radius), ow / 2, oh / 2});

      if (radius <= 0) {
        sf::RectangleShape rect(sf::Vector2f(w, h));
        rect.setPosition(x, y);
        rect.setFillColor(col);
        _window.draw(rect, states);
        return;
      }

      int orig_left = x - ox;
      int orig_top = y - oy;
      int orig_right = orig_left + ow;
      int orig_bottom = orig_top + oh;
      int vis_right = x + w;
      int vis_bottom = y + h;

      // Center
      if (cmd.fill) {
        int cx0 = std::max(x, orig_left + radius);
        int cy0 = std::max(y, orig_top + radius);
        int cx1 = std::min(vis_right, orig_right - radius);
        int cy1 = std::min(vis_bottom, orig_bottom - radius);
        if (cx0 < cx1 && cy0 < cy1) {
          sf::RectangleShape r(sf::Vector2f(cx1 - cx0, cy1 - cy0));
          r.setPosition(cx0, cy0);
          r.setFillColor(col);
          _window.draw(r, states);
        }
      }

      // Left flat
      {
        int lx0 = x, lx1 = std::min(vis_right, orig_left + radius);
        if (lx0 < lx1) {
          int ly0 = std::max(y, orig_top + radius);
          int ly1 = std::min(vis_bottom, orig_bottom - radius);
          if (ly0 < ly1) {
            sf::RectangleShape r(sf::Vector2f(lx1 - lx0, ly1 - ly0));
            r.setPosition(lx0, ly0);
            r.setFillColor(col);
            _window.draw(r, states);
          }
        }
      }

      // Right flat
      {
        int rx0 = std::max(x, orig_right - radius), rx1 = vis_right;
        if (rx0 < rx1) {
          int ry0 = std::max(y, orig_top + radius);
          int ry1 = std::min(vis_bottom, orig_bottom - radius);
          if (ry0 < ry1) {
            sf::RectangleShape r(sf::Vector2f(rx1 - rx0, ry1 - ry0));
            r.setPosition(rx0, ry0);
            r.setFillColor(col);
            _window.draw(r, states);
          }
        }
      }

      // Top flat
      {
        int ty0 = y, ty1 = std::min(vis_bottom, orig_top + radius);
        if (ty0 < ty1) {
          int tx0 = std::max(x, orig_left + radius);
          int tx1 = std::min(vis_right, orig_right - radius);
          if (tx0 < tx1) {
            sf::RectangleShape r(sf::Vector2f(tx1 - tx0, ty1 - ty0));
            r.setPosition(tx0, ty0);
            r.setFillColor(col);
            _window.draw(r, states);
          }
        }
      }

      // Bottom flat
      {
        int by0 = std::max(y, orig_bottom - radius), by1 = vis_bottom;
        if (by0 < by1) {
          int bx0 = std::max(x, orig_left + radius);
          int bx1 = std::min(vis_right, orig_right - radius);
          if (bx0 < bx1) {
            sf::RectangleShape r(sf::Vector2f(bx1 - bx0, by1 - by0));
            r.setPosition(bx0, by0);
            r.setFillColor(col);
            _window.draw(r, states);
          }
        }
      }

      // Corners — horizontal scanlines
      // Top-left
      {
        int y0 = std::max(y, orig_top), y1 = std::min(vis_bottom, orig_top + radius);
        int x0 = std::max(x, orig_left), x1 = std::min(vis_right, orig_left + radius);
        for (int row = y0; row < y1 && x0 < x1; row++) {
          int i = row - orig_top;
          int dy = radius - i;
          int dx = int(std::sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_left + radius - dx, x0);
          int l1 = std::min(orig_left + radius, x1);
          if (l0 <= l1) {
            sf::Vertex line[] = {
              sf::Vertex(sf::Vector2f(l0, row), col),
              sf::Vertex(sf::Vector2f(l1, row), col)
            };
            _window.draw(line, 2, sf::Lines, states);
          }
        }
      }

      // Top-right
      {
        int y0 = std::max(y, orig_top), y1 = std::min(vis_bottom, orig_top + radius);
        int x0 = std::max(x, orig_right - radius), x1 = std::min(vis_right, orig_right);
        for (int row = y0; row < y1 && x0 < x1; row++) {
          int i = row - orig_top;
          int dy = radius - i;
          int dx = int(std::sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_right - radius, x0);
          int l1 = std::min(orig_right - radius + dx, x1);
          if (l0 <= l1) {
            sf::Vertex line[] = {
              sf::Vertex(sf::Vector2f(l0, row), col),
              sf::Vertex(sf::Vector2f(l1, row), col)
            };
            _window.draw(line, 2, sf::Lines, states);
          }
        }
      }

      // Bottom-left
      {
        int y0 = std::max(y, orig_bottom - radius), y1 = std::min(vis_bottom, orig_bottom);
        int x0 = std::max(x, orig_left), x1 = std::min(vis_right, orig_left + radius);
        for (int row = y0; row < y1 && x0 < x1; row++) {
          int i = orig_bottom - 1 - row;
          int dy = radius - 1 - i;
          int dx = int(std::sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_left + radius - dx, x0);
          int l1 = std::min(orig_left + radius, x1);
          if (l0 <= l1) {
            sf::Vertex line[] = {
              sf::Vertex(sf::Vector2f(l0, row), col),
              sf::Vertex(sf::Vector2f(l1, row), col)
            };
            _window.draw(line, 2, sf::Lines, states);
          }
        }
      }

      // Bottom-right
      {
        int y0 = std::max(y, orig_bottom - radius), y1 = std::min(vis_bottom, orig_bottom);
        int x0 = std::max(x, orig_right - radius), x1 = std::min(vis_right, orig_right);
        for (int row = y0; row < y1 && x0 < x1; row++) {
          int i = orig_bottom - 1 - row;
          int dy = radius - 1 - i;
          int dx = int(std::sqrt(radius * radius - dy * dy));
          int l0 = std::max(orig_right - radius, x0);
          int l1 = std::min(orig_right - radius + dx, x1);
          if (l0 <= l1) {
            sf::Vertex line[] = {
              sf::Vertex(sf::Vector2f(l0, row), col),
              sf::Vertex(sf::Vector2f(l1, row), col)
            };
            _window.draw(line, 2, sf::Lines, states);
          }
        }
      }
    }

    /** @brief Render an image texture via sf::Sprite
     *  @param cmd Image render command with source/destination rectangles */
    void on_image_cmd(_detail::image_render_command& cmd) override {
      auto* stxr = static_cast<sfml_texture*>(cmd.txr);
      if (!stxr) return;
      sf::Sprite sprite(stxr->get_sfml_texture());
      sprite.setTextureRect(sf::IntRect(cmd.box.offset_x, cmd.box.offset_y,
                                        cmd.box.width, cmd.box.height));
      sprite.setPosition(cmd.box.x, cmd.box.y);
      sf::BlendMode bm = to_sfml_blend(cmd.blend);
      _window.draw(sprite, sf::RenderStates(bm));
    }

    /** @brief Render a glyph texture using SFML colour modulation
     *  @param cmd Text render command containing glyph texture and colour */
    void on_text_cmd(_detail::text_render_command& cmd) override {
      auto* stxr = static_cast<sfml_texture*>(cmd.glyphs);
      if (!stxr) return;
      sf::Sprite sprite(stxr->get_sfml_texture());
      sprite.setTextureRect(sf::IntRect(cmd.box.offset_x, cmd.box.offset_y,
                                        cmd.box.width, cmd.box.height));
      sprite.setPosition(cmd.box.x, cmd.box.y);
      if (!cmd.is_color)
        sprite.setColor(sf::Color(cmd.col.r, cmd.col.g, cmd.col.b, 255));
      else
        sprite.setColor(sf::Color(255, 255, 255, 255));
      _window.draw(sprite, sf::RenderStates(sf::BlendAlpha));
    }
  };
}
