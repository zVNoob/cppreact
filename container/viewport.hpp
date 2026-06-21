/** @file
 *  @brief Scrollable viewport container. */

#pragma once

#include "../internal/container.hpp"
#include "../internal/arena.hpp"
#include "../internal/state.hpp"
#include "../widgets/scrolling.hpp"
#include "stack.hpp"
#include "func.hpp"
#include <cstdint>

namespace cppreact {
  namespace _config {
    #define CPPREACT_SCROLL_CONFIG \
      int32_t scroll_x = 0; \
      int32_t scroll_y = 0; \
      uint32_t* content_width_limit = nullptr; \
      uint32_t* content_height_limit = nullptr; \

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
      sc.content_width_limit = cfg.content_width_limit;
      sc.content_height_limit = cfg.content_height_limit;
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
      void on_fit_x() override {
        for (auto e : elements()) {
          e->on_fit_x();
        }
      }
      void on_fit_y() override {
        for (auto e : elements()) {
          e->on_fit_y();
        }
      }
      /** @brief Construct a viewport with scroll configuration, a single child, and source location.
       *  @param cfg Scroll configuration including scroll offsets
       *  @param children The single child element
       *  @param loc Source location for diagnostics */
      viewport(_config::scroll_config cfg, identifiable* children, std::source_location loc) :
        stack(to_container_config(cfg), {children}, loc), _scroll_cfg(to_scroll_specific_config(cfg)) {
        }
      /** @brief Position children horizontally with scroll offset applied. */
      void on_child_pos_x() override {
        if (elements().empty()) { return; }
        if (_scroll_cfg.content_width_limit) {
          int32_t temp = box(*elements().front()).width + 
            elements().front()->element_config().margin.left + 
            elements().front()->element_config().margin.right - 
            _box.width;
          *_scroll_cfg.content_width_limit = std::max(0, temp);
        }
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
        if (elements().empty()) { return; }
        if (_scroll_cfg.content_height_limit) {
          int32_t temp = box(*elements().front()).height + 
            elements().front()->element_config().margin.top + 
            elements().front()->element_config().margin.bottom - 
            _box.height;
          *_scroll_cfg.content_height_limit = std::max(0, temp);
        }
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

  /** @brief Allocate a scrollable viewport with built-in mouse-wheel scroll handling.
   *
   *  Wraps a viewport inside a func that maintains scroll offsets in per-instance
   *  state. A scroll handler overlay captures wheel events and updates the offsets
   *  automatically.
   *  @param cfg Scroll configuration (initial scroll offsets, sizing, etc.)
   *  @param children The single child element to scroll
   *  @param loc Source location for diagnostics
   *  @return Pointer to the allocated func component */
  inline _detail::func* scroll(_config::scroll_config cfg, _detail::identifiable* children, std::source_location loc = std::source_location::current()) {
    return func([cfg, children, loc](state& s, _detail::func*) mutable -> _detail::element* {
      auto scroll_x = s.get<int32_t>(cfg.scroll_x);
      auto scroll_y = s.get<int32_t>(cfg.scroll_y);
      const uint32_t& content_width = s.get<uint32_t>(100);
      const uint32_t& content_height = s.get<uint32_t>(100);

      cfg.scroll_x = scroll_x;
      cfg.scroll_y = scroll_y;
      cfg.content_width_limit = &const_cast<uint32_t&>(content_width);
      cfg.content_height_limit = &const_cast<uint32_t&>(content_height);

      auto vcfg = cfg;
      vcfg.sizing = {GROW(), GROW()};
      vcfg.margin = {0, 0, 0, 0};
      vcfg.alignment = {0, 0};
      auto vp = viewport(vcfg, children, loc);
      auto sc = scrolling({.sizing = {GROW(), GROW()}},
        [scroll_x, scroll_y, content_width, content_height](int32_t dx, int32_t dy) {
          scroll_x = std::clamp((int32_t)(scroll_x - dx * 10), 0, (int32_t)content_width);
          scroll_y = std::clamp((int32_t)(scroll_y - dy * 10), 0, (int32_t)content_height);
        }, loc
      );

      auto scfg = cfg;
      scfg.padding = {0, 0, 0, 0};
      scfg.spacing = 0;
      return stack(_config::to_container_config(scfg), {vp, sc}, loc);
    }, loc);
  }
}
