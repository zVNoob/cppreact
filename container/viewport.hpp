/** @file
 *  @brief Scrollable viewport container. */

#pragma once

#include "../internal/container.hpp"
#include "../internal/arena.hpp"
#include "stack.hpp"
#include <cstdint>

namespace cppreact {
  namespace _config {
    #define CPPREACT_SCROLL_CONFIG \
      int32_t scroll_x = 0; \
      int32_t scroll_y = 0; \

    /** @brief Full configuration for a scrollable viewport, including scroll offsets. */
    struct scroll_config {
      CPPREACT_ELEMENT_CONFIG;
      CPPREACT_CONTAINER_CONFIG;
      CPPREACT_SCROLL_CONFIG;
    };
    /** @brief Convert a scroll_config to a container_config, discarding scroll fields.
     *  @param cfg The scroll configuration
     *  @return Container configuration with alignment, margin, sizing, spacing, and padding */
    inline container_config to_container_config(scroll_config cfg) {
      container_config cc;
      cc.alignment = cfg.alignment;
      cc.margin = cfg.margin;
      cc.sizing = cfg.sizing;
      cc.spacing = cfg.spacing;
      cc.padding = cfg.padding;
      return cc;
    }
    /** @brief Subset of scroll_config containing only the scroll offset fields. */
    struct scroll_specific_config {
      CPPREACT_SCROLL_CONFIG;
    };
    /** @brief Extract the scroll-specific fields from a scroll_config.
     *  @param cfg The scroll configuration
     *  @return Scroll-specific configuration with scroll_x and scroll_y */
    inline scroll_specific_config to_scroll_specific_config(scroll_config cfg) {
      scroll_specific_config sc;
      sc.scroll_x = cfg.scroll_x;
      sc.scroll_y = cfg.scroll_y;
      return sc;
    }
  }
  namespace _detail {
    /** @brief Scrollable viewport that clips its content and applies scroll offsets.
     *
     *  Inherits from stack for layout but applies scroll_x and scroll_y
     *  offsets during child positioning and clips the rendered area to
     *  the viewport bounds. */
    class viewport : public stack {
    private:
      _config::scroll_specific_config _scroll_cfg; ///< Scroll offset configuration
    public:
      /** @brief Construct a viewport with scroll configuration, a single child, and source location.
       *  @param cfg Scroll configuration including scroll offsets
       *  @param children The single child element
       *  @param loc Source location for diagnostics */
      viewport(_config::scroll_config cfg, identifiable* children, std::source_location loc) :
        stack(to_container_config(cfg), {children}, loc), _scroll_cfg(to_scroll_specific_config(cfg)) {}
      /** @brief Position children horizontally with scroll offset applied. */
      void on_child_pos_x() override {
        for (auto e : elements()) {
          int32_t w = box(*e).width + e->element_config().margin.left + e->element_config().margin.right;
          int32_t aw = _box.width - _container_config.padding.left - _container_config.padding.right;
          box(*e).x = _box.x + _container_config.padding.left + e->element_config().margin.left
                      + int32_t((aw - w) * std::clamp(e->element_config().alignment.x, 0.0f, 1.0f));
          box(*e).x -= _scroll_cfg.scroll_x;
          e->on_child_pos_x();
        }
      }
      /** @brief Position children vertically with scroll offset applied. */
      void on_child_pos_y() override {
        for (auto e : elements()) {
          int32_t h = box(*e).height + e->element_config().margin.top + e->element_config().margin.bottom;
          int32_t ah = _box.height - _container_config.padding.top - _container_config.padding.bottom;
          box(*e).y = _box.y + _container_config.padding.top + e->element_config().margin.top
                      + int32_t((ah - h) * std::clamp(e->element_config().alignment.y, 0.0f, 1.0f));
          box(*e).y -= _scroll_cfg.scroll_y;
          e->on_child_pos_y();
        }
      }
      /** @brief Render the viewport clipped to the intersection of the render box and the viewport bounds.
       *  @param render_box The bounding box to render within
       *  @return List of render commands clipped to the viewport area */
      std::vector<render_command*> on_render(bounding_box render_box) override {
        auto clipped = intersect(render_box, _box);
        return container::on_render(clipped);
      }
    };
  }

  /** @brief Allocate a scrollable viewport in arena storage.
   *  @param cfg Scroll configuration including scroll offsets
   *  @param children The single child element
   *  @param loc Source location for diagnostics
   *  @return Pointer to the allocated viewport */
  inline _detail::viewport* viewport(_config::scroll_config cfg, _detail::identifiable* children, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::viewport>(cfg, children, loc);
  }
}
