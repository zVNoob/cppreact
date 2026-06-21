/**
 * @file image.hpp
 * @brief Image widget displaying a texture with optional aspect-ratio maintenance
 */
#pragma once
#include "../internal/component.hpp"
#include "../internal/arena.hpp"
#include "../internal/texture.hpp"
#include <algorithm>
#include <cstdint>

namespace cppreact {
  namespace _config {
    #define CPPREACT_IMAGE_CONFIG \
      blend_mode blend = NONE; \
      bool keep_aspect_ratio = false; \

    /**
     * @brief Full image configuration including element properties
     */
    struct image_config {
      CPPREACT_IMAGE_CONFIG;
      CPPREACT_ELEMENT_CONFIG;
    };

    /**
     * @brief Image-only configuration (no element properties)
     */
    struct image_specific_config {
      CPPREACT_IMAGE_CONFIG;
    };

    /**
     * @brief Converts a full image_config to image_specific_config
     * @param cfg Source configuration
     * @return Extracted image-specific configuration
     */
    inline image_specific_config to_image_specific_config(image_config cfg) {
      image_specific_config rc;
      rc.blend = cfg.blend;
      rc.keep_aspect_ratio = cfg.keep_aspect_ratio;
      return rc;
    }
  }
  namespace _detail {
    /**
     * @brief Render command that draws a textured quad
     */
    class image_render_command : public render_command {
      public:
        texture* txr; ///< The texture to render
        blend_mode blend;             ///< Blend mode for compositing
      image_render_command(bounding_box box, texture* texture, blend_mode blend) : render_command(box), txr(texture), blend(blend) {}
    };
    /**
     * @brief Image component that displays a texture, optionally maintaining aspect ratio
     */
    class image : public component {
      texture* _txr; ///< The texture to display
      _config::image_config _cscfg;  ///< Configuration for the image
      public:
      image(texture* tex,_config::image_config cfg, std::source_location loc) : component(to_element_config(cfg), loc), _txr(tex), _cscfg(cfg) {}
      /**
       * @brief Produces the render command for this image
       * @param render_box The parent render bounds
       * @return Vector containing the image_render_command, or empty if clipped away
       */
      std::vector<render_command*> on_render(bounding_box render_box) override {
        auto clipped = clip(render_box, _box);
        if (clipped.width <= 0 || clipped.height <= 0) return {};
        return {_storage::allocate<image_render_command>(clipped, _txr, _cscfg.blend)};
      }
      protected:
      void on_fit_x() override {
        _config = to_element_config(_cscfg);
        if (!_cscfg.keep_aspect_ratio) _config.sizing.x.min = std::max(_config.sizing.x.min, _txr->width());
        element::on_fit_x();
      }
      void on_fit_y() override {
        if (_cscfg.keep_aspect_ratio) {
          // Scale height proportionally to the texture aspect ratio
          _config.sizing.y = FIXED(std::clamp(uint16_t(_box.width * _txr->width() / _txr->height()), _cscfg.sizing.y.min, _cscfg.sizing.y.max));
        }
        else _config.sizing.y.min = std::max(_config.sizing.y.min, _txr->height());
        element::on_fit_y();
      }
      void on_child_grow_y() override {
        if (_cscfg.keep_aspect_ratio) {
          // Scale width proportionally to the texture aspect ratio
          _box.width = std::clamp(uint16_t(_box.height * _txr->width() / _txr->height()), _cscfg.sizing.x.min, _cscfg.sizing.x.max);
        }
        element::on_child_grow_y();
      }
    };
  }
  /**
   * @brief Allocates and returns a new image widget
   * @param txr The texture to display
   * @param cfg Configuration (sizing, blend, aspect-ratio flag)
   * @param loc Source location for debugging
   * @return Pointer to the allocated image component
   */
  inline _detail::image* image(texture* txr, _config::image_config cfg = {.sizing = {GROW(), GROW()}}, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::image>(txr, cfg, loc);
  }
}
