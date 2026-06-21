/** @file
 *  @brief Abstract texture base class and color/blend-mode types.
 */

#pragma once

#include <cstdint>
#include <vector>
namespace cppreact {

  /** @brief RGBA color with 8-bit channels. */
  struct color {uint8_t r = 0, g = 0, b = 0, a = 255;};

  /** @brief Blend mode for rendering operations. */
  enum blend_mode {
    NONE,     ///< No blending (opaque)
    ADD,      ///< Additive blending
    MULTIPLY  ///< Multiplicative blending
  };

  /** @brief Abstract base class for texture resources. */
  class texture {
    uint16_t _width;  ///< Texture width in pixels
    uint16_t _height; ///< Texture height in pixels
    public:
    texture(int width, int height) : _width(width), _height(height) {}
    virtual ~texture() {}
    uint16_t width() {return _width;}
    uint16_t height() {return _height;}
    /** @brief Get the color of a single pixel. */
    virtual color get_pixel(int x, int y) = 0;
    /** @brief Get raw pixel data for the entire texture. */
    virtual std::vector<uint8_t> get_pixels() = 0;
  };
}
