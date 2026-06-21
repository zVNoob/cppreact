/**
 * @file scrolling.hpp
 * @brief Scroll wheel input component emitting scroll events
 */
#pragma once

#include "../internal/component.hpp"
#include "../internal/arena.hpp"
#include <cstdint>
#include <functional>

namespace cppreact {
  typedef std::function<void(int32_t, int32_t)> scroll_lambda; ///< Callback receiving (scroll_x, scroll_y) deltas
  namespace _config {
    /**
     * @brief Configuration for scroll-simple component (element properties only)
     */
    struct scroll_simple_config {
      CPPREACT_ELEMENT_CONFIG;
    };
  }
  namespace _detail {
    /**
     * @brief Render command that binds a scroll callback to a region
     */
    class scroll_render_command : public render_command {
      public:
      scroll_lambda func; ///< The scroll event handler
      scroll_render_command(bounding_box box, scroll_lambda func) : render_command(box), func(func) {}
    };
    /**
     * @brief Component that captures scroll wheel events over its region
     */
    class scroll_simple : public component {
      scroll_lambda _func; ///< Stored scroll callback
      public:
      scroll_simple(_config::scroll_simple_config cfg, scroll_lambda func, std::source_location loc) :
        component(to_element_config(cfg), loc), _func(func) {}
      /**
       * @brief Produces a render command that registers the scroll handler
       * @param render_box The parent render bounds
       * @return Vector containing the scroll_render_command, or empty if clipped away
       */
      std::vector<render_command*> on_render(bounding_box render_box) override {
        auto clipped = clip(render_box, _box);
        if (clipped.width <= 0 || clipped.height <= 0) return {};
        return {_storage::allocate<scroll_render_command>(clipped, _func)};
      }
    };
  }
  /**
   * @brief Allocates and returns a new scroll-wheel input widget
   * @param cfg Configuration (element sizing, margin, alignment)
   * @param func Callback receiving scroll delta (x, y)
   * @param loc Source location for debugging
   * @return Pointer to the allocated scroll_simple component
   */
  inline _detail::scroll_simple* scrolling(_config::scroll_simple_config cfg, scroll_lambda func, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::scroll_simple>(cfg, func, loc);
  }
}
