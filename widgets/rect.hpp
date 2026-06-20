/**
 * @file rect.hpp
 * @brief Colored rectangle widget with rounded corners, border, and shadow support
 */
#pragma once

#include "../internal/component.hpp"
#include "../internal/arena.hpp"
#include "../internal/container.hpp"
#include "../container/stack.hpp"
#include "../internal/texture.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace cppreact {

  namespace _config {

    #define CPPREACT_RECT_CONFIG \
      color col = {0, 0, 0, 255}; \
      uint16_t radius = 0; \
      blend_mode blend = NONE; \

    /**
     * @brief Full rectangle configuration including element properties
     */
    struct rect_config {
      CPPREACT_RECT_CONFIG;
      CPPREACT_ELEMENT_CONFIG;
    };
    /**
     * @brief Rectangle-only configuration (no element properties)
     */
    struct rect_specific_config {
      CPPREACT_RECT_CONFIG;
    };
    /**
     * @brief Converts a full rect_config to rect_specific_config
     * @param cfg Source configuration
     * @return Extracted rect-specific configuration
     */
    inline rect_specific_config to_rect_specific_config(rect_config cfg) {
      rect_specific_config rc;
      rc.col = cfg.col;
      rc.radius = cfg.radius;
      rc.blend = cfg.blend;
      return rc;
    }
  }

  namespace _detail {
    /**
     * @brief Render command that draws a (possibly rounded) rectangle
     */
    class rect_render_command : public render_command {
    public:
      color col;                            ///< Fill color
      uint16_t radius;                      ///< Corner radius in pixels
      blend_mode blend;                     ///< Blend mode
      bool fill;                            ///< true = filled rect, false = border only
      std::pair<uint16_t, uint16_t> original_size; ///< Unclipped dimensions for border scaling
      rect_render_command(bounding_box box, color col, uint16_t radius, blend_mode blend, bool fill, std::pair<uint16_t, uint16_t> original_size) : render_command(box), col(col), radius(radius), blend(blend), fill(fill), original_size(original_size) {}
    };
    /**
     * @brief Rectangle component that renders a colored rect with optional rounded corners
     */
    class rect : public component {
    private:
      _config::rect_specific_config cfg; ///< Rectangle-specific configuration
      bool fill;                         ///< true = filled, false = border-only
    public:
      rect(_config::rect_config cfg, bool fill, std::source_location loc) : 
        component(to_element_config(cfg), loc), fill(fill), cfg(to_rect_specific_config(cfg)) {}
      /**
       * @brief Produces the render command for this rectangle
       * @param render_box The parent render bounds
       * @return Vector containing the rect_render_command, or empty if clipped away
       */
      std::vector<render_command*> on_render(bounding_box render_box) override {
        auto clipped = clip(render_box, _box);
        if (clipped.width <= 0 || clipped.height <= 0) return {};
        return {_storage::allocate<rect_render_command>(
            clipped, 
            cfg.col, 
            cfg.radius, 
            cfg.blend, 
            fill,
            std::make_pair(uint16_t(_box.width), uint16_t(_box.height)))};
      }
    };
  }
  /**
   * @brief Allocates and returns a new rect widget
   * @param cfg Configuration (color, radius, blend, sizing, etc.)
   * @param fill true for filled rect, false for border-only
   * @param loc Source location for debugging
   * @return Pointer to the allocated rect component
   */
  inline _detail::rect* rect(_config::rect_config cfg = {.sizing = {GROW(), GROW()}}, bool fill = true, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::rect>(cfg, fill, loc);
  }
  namespace _config { 
    /**
     * @brief Configuration combining rect styling with container layout properties
     */
    struct rect_container_config {
      CPPREACT_RECT_CONFIG;
      CPPREACT_ELEMENT_CONFIG;
      CPPREACT_CONTAINER_CONFIG;
    };
    /**
     * @brief Extracts rect_config from a rect_container_config
     * @param cfg Source container configuration
     * @return Basic rect configuration
     */
    inline rect_config to_rect_config(rect_container_config cfg) {
      rect_config rc;
      rc.alignment = cfg.alignment;
      rc.margin = cfg.margin;
      rc.sizing = cfg.sizing;
      rc.col = cfg.col;
      rc.radius = cfg.radius;
      rc.blend = cfg.blend;
      return rc;
    }
  }
  /**
   * @brief Allocates a styled rectangle with inner content (filled rect behind a stack of children)
   * @param cfg Configuration combining rect, element, and container properties
   * @param children Child widget(s) to place inside the rect
   * @param loc Source location for debugging
   * @return Pointer to the stack component containing the rect and children
   */
  inline _detail::stack* rect(_config::rect_container_config cfg, _detail::identifiable* children, std::source_location loc = std::source_location::current()) {
    _config::rect_config rc = to_rect_config(cfg);
    rc.sizing = {GROW(), GROW()};
    rc.alignment = {ALIGN_BEGIN, ALIGN_BEGIN};
    rc.margin = PAD();
    _config::container_config scfg = to_container_config(cfg);
    scfg.padding = PAD();
    scfg.spacing = 0;
    _config::container_config ccfg = {.sizing = {GROW(), GROW()}, .margin = PAD(), .alignment = {ALIGN_BEGIN, ALIGN_BEGIN}, .padding = cfg.padding, .spacing = cfg.spacing};
    return stack(scfg, {rect(rc, true, loc) ,stack(ccfg, {children}, loc)}, loc);
  }
  /**
   * @brief Allocates a bordered region with inner content (unfilled rect outline behind a stack of children)
   * @param cfg Configuration combining rect, element, and container properties
   * @param children Child widget(s) to place inside the border
   * @param loc Source location for debugging
   * @return Pointer to the stack component containing the border rect and children
   */
  inline _detail::stack* border(_config::rect_container_config cfg, _detail::identifiable* children, std::source_location loc = std::source_location::current()) {
    _config::rect_config rc = to_rect_config(cfg);
    rc.sizing = {GROW(), GROW()};
    rc.alignment = {ALIGN_BEGIN, ALIGN_BEGIN};
    rc.margin = PAD();
    _config::container_config scfg = to_container_config(cfg);
    scfg.padding = PAD();
    scfg.spacing = 0;
    _config::container_config ccfg = {.sizing = {GROW(), GROW()}, .margin = PAD(), .alignment = {ALIGN_BEGIN, ALIGN_BEGIN}, .padding = cfg.padding, .spacing = cfg.spacing};
    return stack(scfg, {rect(rc, false, loc) ,stack(ccfg, {children}, loc)}, loc);
  }
}
