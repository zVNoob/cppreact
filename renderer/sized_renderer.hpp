#pragma once

/** @file sized_renderer.hpp
 *  @brief Sized renderer variant with fixed dimensions */

#include "renderer.hpp"
#include <cstdint>

namespace cppreact {
  /** @brief Renderer with fixed window dimensions
   *
   * Wraps renderer::run_once with a fixed-size container configuration.
   * Subclasses must implement run() for the platform-specific event loop. */
  class sized_renderer : public renderer {
    std::pair<uint16_t, uint16_t> _size; ///< Fixed render-area dimensions (width, height)
  public:
    /** @brief Construct a sized renderer with the given dimensions
     * @param size Width and height of the render area
     * @param loc Source location for debugging */
    sized_renderer(std::pair<uint16_t, uint16_t> size, std::source_location loc = std::source_location::current()) :
      renderer(loc) {
      _size = size;
    }
    /** @brief Run the main event loop
     * @param root Root widget of the UI tree */
    virtual void run(_detail::identifiable* root) = 0;
  protected:
    /** @brief Execute a single frame with the fixed-size configuration
     * @param root Root widget to update and render */
    void run_once(_detail::identifiable* root) {
      renderer::run_once(root, {.sizing = {FIXED(_size.first), FIXED(_size.second)}});
    }
    void set_size(uint16_t width, uint16_t height) {
      _size = {width, height};
    }
    std::pair<uint16_t, uint16_t> get_size() {
      return _size;
    } 
  };
}
