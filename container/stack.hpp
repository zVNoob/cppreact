/** @file
 *  @brief Stack layout container. */

#pragma once

#include "../internal/arena.hpp"
#include "../internal/container.hpp"
#include <algorithm>
#include <cstdint>
#include <unistd.h>

namespace cppreact {
  namespace _detail {
    /** @brief Stack layout that layers children on top of each other.
     *
     *  Each child occupies the full available area. The box is sized
     *  to the largest child in each dimension. Children are positioned
     *  using per-child alignment values within the available space. */
    class stack : public container {
    public:
      /** @brief Construct a stack with configuration, children, and source location.
       *  @param cfg Container configuration
       *  @param children Child elements to stack
       *  @param loc Source location for diagnostics */
      stack(_config::container_config cfg, std::initializer_list<identifiable*> children, std::source_location loc) :
        container(cfg, children, loc) {}
      /** @brief Compute the fit width as the maximum child width plus padding. */
      void on_fit_x() override {
        for (auto e : elements()) {
          e->on_fit_x();
          int32_t width = box(*e).width + e->element_config().margin.left + e->element_config().margin.right;
          if (width > int32_t(_box.width))
            _box.width = std::min<uint16_t>(width, _config.sizing.x.max);
        }
        _box.width += _container_config.padding.left + _container_config.padding.right;
      }
      /** @brief Compute the fit height as the maximum child height plus padding. */
      void on_fit_y() override {
        for (auto e : elements()) {
          e->on_fit_y();
          int32_t height = box(*e).height + e->element_config().margin.top + e->element_config().margin.bottom;
          if (height > int32_t(_box.height))
            _box.height = std::min<uint16_t>(height, _config.sizing.y.max);
        }
        _box.height += _container_config.padding.top + _container_config.padding.bottom;
      }
      /** @brief Grow children horizontally proportional to their weight. */
      void on_child_grow_x() override {
        for (auto e : elements()) {
          int32_t width = _box.width * std::clamp(e->element_config().sizing.x.weight, 0.0f, 1.0f);
          width = std::min(width, int32_t(_box.width - 
                _container_config.padding.left - _container_config.padding.right - 
                e->element_config().margin.left - e->element_config().margin.right));
          box(*e).width = uint16_t(std::clamp<int32_t>(width, box(*e).width, e->element_config().sizing.x.max));
          e->on_child_grow_x();
        }
      }
      /** @brief Grow children vertically proportional to their weight. */
      void on_child_grow_y() override {
        for (auto e : elements()) {
          float weight = std::max(std::min(e->element_config().sizing.y.weight, 1.0f), 0.0f);
          int32_t height = _box.height * weight;
          height = std::min(height, int32_t(_box.height - 
                _container_config.padding.top - _container_config.padding.bottom - 
                e->element_config().margin.top - e->element_config().margin.bottom));
          box(*e).height = uint16_t(std::clamp<int32_t>(height, box(*e).height, e->element_config().sizing.y.max));
          e->on_child_grow_y();
        }
      }
      /** @brief Position children horizontally using alignment weights. */
      void on_child_pos_x() override {
        for (auto e : elements()) {
          int32_t actual_width = box(*e).width + e->element_config().margin.left + e->element_config().margin.right;
          int32_t actual_available_width = _box.width - _container_config.padding.left - _container_config.padding.right;
          box(*e).x = _box.x +
                      _container_config.padding.left + e->element_config().margin.left + 
                      (actual_available_width - actual_width) * std::clamp(e->element_config().alignment.x, 0.0f, 1.0f);
          e->on_child_pos_x();
        }
      }
      /** @brief Position children vertically using alignment weights. */
      void on_child_pos_y() override {
        for (auto e : elements()) {
          int32_t actual_height = box(*e).height + e->element_config().margin.top + e->element_config().margin.bottom;
          int32_t actual_available_height = _box.height - _container_config.padding.top - _container_config.padding.bottom;
          box(*e).y = _box.y +
                      _container_config.padding.top + e->element_config().margin.top + 
                      (actual_available_height - actual_height) * std::clamp(e->element_config().alignment.y, 0.0f, 1.0f);
          e->on_child_pos_y();
        }
      }
    };
  }
  /** @brief Allocate a stack layout in arena storage.
   *  @param cfg Container configuration
   *  @param children Child elements to stack
   *  @param loc Source location for diagnostics
   *  @return Pointer to the allocated stack */
  inline _detail::stack* stack(_config::container_config cfg = {}, std::initializer_list<_detail::identifiable*> children = {}, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::stack>(cfg, children, loc);
  }
}
