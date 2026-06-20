/**
 * @file button_full.hpp
 * @brief Full-featured button with visual styling via stack/rect composition
 */
#pragma once

#include "button_simple.hpp"
#include "../internal/registry.hpp"
#include "../container/stack.hpp"

namespace cppreact {
  namespace _config {
    #define CPPREACT_BUTTON_FULL_CONFIG \
      button_lambda on_mouse_down = nullptr; \
      button_lambda on_mouse_up = nullptr; \
      button_lambda on_mouse_enter = nullptr; \
      button_lambda on_mouse_leave = nullptr; \
      button_lambda on_mouse_move = nullptr; \

    /**
     * @brief Full button configuration including per-event callbacks and element properties
     */
    struct button_full_config {
      CPPREACT_BUTTON_CONFIG;
      CPPREACT_ELEMENT_CONFIG;
      CPPREACT_BUTTON_FULL_CONFIG;
    };
    /**
     * @brief Button-full-only configuration (no element properties)
     */
    struct button_full_specific_config {
      CPPREACT_BUTTON_CONFIG;
      CPPREACT_BUTTON_FULL_CONFIG;
    };
    /**
     * @brief Converts a full button_full_config to button_full_specific_config
     * @param cfg Source configuration
     * @return Extracted button-full-specific configuration
     */
    inline button_full_specific_config to_button_full_specific_config(button_full_config cfg) {
      button_full_specific_config cscfg;
      cscfg.event_mask = cfg.event_mask;
      cscfg.on_mouse_down = cfg.on_mouse_down;
      cscfg.on_mouse_up = cfg.on_mouse_up;
      cscfg.on_mouse_enter = cfg.on_mouse_enter;
      cscfg.on_mouse_leave = cfg.on_mouse_leave;
      cscfg.on_mouse_move = cfg.on_mouse_move;
      return cscfg;
    }
  }
  namespace _detail {
    /**
     * @brief Full-featured button component with per-event callbacks and hover/click state tracking
     */
    class button_full : public component {
      _config::button_full_specific_config _cscfg; ///< Button configuration with all callbacks
      state* _reg_state;                            ///< Persistent state from the registry
    public:
      button_full(_config::button_full_config cfg, std::source_location loc) :
        component(to_element_config(cfg), loc), _cscfg(to_button_full_specific_config(cfg)) {
        _reg_state = &_storage::current_registry.get(id()).second;
      }

      /**
       * @brief Resets state and fires on_mouse_leave if hovering ended without a callback
       */
      void on_update() override {
        element::on_update();
        _reg_state->reset();
        auto cb_fired = _reg_state->get<bool>(false);
        auto was_hovered = _reg_state->get<bool>(false);
        auto tracking = _reg_state->get<std::pair<uint16_t, std::pair<uint16_t, uint16_t>>>(
          {0, {0, 0}});

        // If no callback fired this frame but we were hovered, the mouse left
        if (!cb_fired && was_hovered) {
          if (_cscfg.on_mouse_leave) _cscfg.on_mouse_leave(0, {0, 0});
          was_hovered = false;
        }
        cb_fired = false;
      }
      /**
       * @brief Produces a render command with a lambda that dispatches per-event callbacks
       * @param render_box The parent render bounds
       * @return Vector containing the button_render_command, or empty if clipped away
       */
      std::vector<render_command*> on_render(bounding_box render_box) override {
        auto clipped = clip(render_box, _box);
        if (clipped.width <= 0 || clipped.height <= 0) return {};
        return {_storage::allocate<button_render_command>(clipped,
          [this](uint16_t bits, std::pair<uint16_t, uint16_t> pos) {
            _reg_state->reset();
            auto cb_fired = _reg_state->get<bool>(true);
            auto was_hovered = _reg_state->get<bool>(false);
            auto tracking = _reg_state->get<std::pair<uint16_t, std::pair<uint16_t, uint16_t>>>(
              {0, {0, 0}});
            uint16_t last_bits = tracking->first;
            auto last_pos = tracking->second;

            bool hover_now = bits & HOVER;
            bool hover_before = last_bits & HOVER;

            // Fire on_mouse_enter when hovering begins
            if (hover_now && !hover_before && _cscfg.on_mouse_enter)
              _cscfg.on_mouse_enter(bits, pos);

            // Detect new presses and releases
            uint16_t changed = bits ^ last_bits;
            uint16_t pressed = changed & bits;
            uint16_t released = changed & last_bits;

            if (pressed & (LCLICK | RCLICK | MCLICK) && _cscfg.on_mouse_down)
              _cscfg.on_mouse_down(bits, pos);
            if (released & (LCLICK | RCLICK | MCLICK) && _cscfg.on_mouse_up)
              _cscfg.on_mouse_up(bits, pos);

            // Fire on_mouse_move when the mouse moves while hovering
            if (hover_now && pos != last_pos && _cscfg.on_mouse_move)
              _cscfg.on_mouse_move(bits, pos);

            tracking->first = bits;
            tracking->second = pos;
            cb_fired = true;
            was_hovered = hover_now;
          },
          _cscfg.event_mask | HOVER)};
      }
    };
  }
  /**
   * @brief Allocates and returns a new full-featured button widget
   * @param cfg Configuration (event callbacks, event_mask, element properties)
   * @param loc Source location for debugging
   * @return Pointer to the allocated button_full component
   */
  inline _detail::button_full* button(_config::button_full_config cfg,
    std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::button_full>(cfg, loc);
  }
  namespace _config { 
    /**
     * @brief Configuration combining button_full callbacks with container layout properties
     */
    struct button_full_container_config {
      CPPREACT_BUTTON_CONFIG;
      CPPREACT_ELEMENT_CONFIG;
      CPPREACT_CONTAINER_CONFIG;
      CPPREACT_BUTTON_FULL_CONFIG;
    };
    /**
     * @brief Extracts button_full_config from a button_full_container_config
     * @param cfg Source container configuration
     * @return Basic button_full configuration
     */
    inline button_full_config to_button_full_config(button_full_container_config cfg) {
      button_full_config cscfg;
      cscfg.alignment = cfg.alignment;
      cscfg.margin = cfg.margin;
      cscfg.sizing = cfg.sizing;
      cscfg.event_mask = cfg.event_mask;
      cscfg.on_mouse_down = cfg.on_mouse_down;
      cscfg.on_mouse_up = cfg.on_mouse_up;
      cscfg.on_mouse_enter = cfg.on_mouse_enter;
      cscfg.on_mouse_leave = cfg.on_mouse_leave;
      cscfg.on_mouse_move = cfg.on_mouse_move;
      return cscfg;
    }
  }
  /**
   * @brief Allocates a full-featured button with children inside it (button + stack composition)
   * @param cfg Configuration combining button_full, element, and container properties
   * @param children Child widget(s) to place inside the button
   * @param loc Source location for debugging
   * @return Pointer to the stack containing the button and children
   */
  inline _detail::stack* button(_config::button_full_container_config cfg, _detail::identifiable* children, std::source_location loc = std::source_location::current()) {
    _config::button_full_config rc = _config::to_button_full_config(cfg);
    rc.sizing = {GROW(), GROW()};
    rc.alignment = {ALIGN_BEGIN, ALIGN_BEGIN};
    rc.margin = PAD();
    return stack(to_container_config(cfg), {button(rc, loc) ,children}, loc);
  }
  
}
