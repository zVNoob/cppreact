/**
 * @file button_simple.hpp
 * @brief Simple button component with click/hold detection
 */
#pragma once

#include "../internal/component.hpp"
#include "../internal/arena.hpp"
#include <cstdint>
#include <functional>
#include <utility>

namespace cppreact {
  const uint16_t LCLICK = 1 << 0; ///< Left mouse button bit
  const uint16_t RCLICK = 1 << 1; ///< Right mouse button bit
  const uint16_t MCLICK = 1 << 2; ///< Middle mouse button bit
  const uint16_t HOVER  = 1 << 3; ///< Hover state bit
  typedef std::function<void(uint16_t, std::pair<uint16_t, uint16_t>)> button_lambda; ///< Callback receiving (event_bits, position)
  namespace _config {
    #define CPPREACT_BUTTON_CONFIG \
      uint16_t event_mask = LCLICK; \

    /**
     * @brief Full button configuration including element properties
     */
    struct button_config {
      CPPREACT_BUTTON_CONFIG;
      CPPREACT_ELEMENT_CONFIG;
    };
    /**
     * @brief Button-only configuration (no element properties)
     */
    struct button_specific_config {
      CPPREACT_BUTTON_CONFIG;
    };
    /**
     * @brief Converts a full button_config to button_specific_config
     * @param cfg Source configuration
     * @return Extracted button-specific configuration
     */
    inline button_specific_config to_button_specific_config(button_config cfg) {
      button_specific_config cscfg;
      cscfg.event_mask = cfg.event_mask;
      return cscfg;
    }
    /**
     * @brief Extracts button_config from a generic config that has event_mask, alignment, margin, and sizing
     * @tparam T Config type with the required fields
     * @param cfg Source configuration
     * @return Basic button configuration
     */
    template<typename T>
    inline button_config to_button_config(T cfg) {
      button_config bc;
      bc.event_mask = cfg.event_mask;
      bc.alignment = cfg.alignment;
      bc.margin = cfg.margin;
      bc.sizing = cfg.sizing;
      return bc;
    }
  }
  namespace _detail {
    /**
     * @brief Render command that binds a button callback to a region
     */
    class button_render_command : public render_command {
      public:
      button_lambda func;     ///< The button event handler
      uint16_t event_mask;    ///< Bitmask of events to listen for
      button_render_command(bounding_box box, button_lambda onhold, uint16_t event_mask) : render_command(box), func(onhold), event_mask(event_mask) {}
    };
    /**
     * @brief Simple button component that emits events on click/hold
     */
    class button_simple : public component {
      button_lambda _func;                     ///< Stored button callback
      _config::button_specific_config _cscfg;  ///< Button-specific configuration
    public:
      button_simple(_config::button_config cfg, button_lambda func, std::source_location loc) : 
        component(to_element_config(cfg), loc), _func(func), _cscfg(to_button_specific_config(cfg)) {}
      /**
       * @brief Produces a render command that registers the button handler
       * @param render_box The parent render bounds
       * @return Vector containing the button_render_command, or empty if clipped away
       */
      std::vector<render_command*> on_render(bounding_box render_box) override {
        auto clipped = clip(render_box, _box);
        if (clipped.width <= 0 || clipped.height <= 0) return {};
        return {_storage::allocate<button_render_command>(clipped, _func, _cscfg.event_mask)};
      }
    };
  }
  /**
   * @brief Allocates and returns a new simple button widget
   * @param cfg Configuration (event_mask plus element properties)
   * @param onhold Callback receiving (event_bits, position)
   * @param loc Source location for debugging
   * @return Pointer to the allocated button_simple component
   */
  inline _detail::button_simple* button(_config::button_config cfg, button_lambda onhold, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::button_simple>(cfg, onhold, loc);
  }
}
