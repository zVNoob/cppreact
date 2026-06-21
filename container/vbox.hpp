/** @file
 *  @brief Vertical box layout container. */

#pragma once

#include "../internal/container.hpp"
#include "../internal/arena.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace cppreact {
  namespace _detail {
    /** @brief Vertical box layout that stacks children vertically.
     *
     *  Children are arranged from top to bottom. The box width matches
     *  the widest child plus padding; height is the sum of all child
     *  heights plus spacing and padding. */
    class vbox : public container {
    public:
      /** @brief Construct a vbox with configuration, children, and source location.
       *  @param cfg Container configuration
       *  @param children Child elements to lay out
       *  @param loc Source location for diagnostics */
      vbox(_config::container_config cfg, std::initializer_list<identifiable*> children, std::source_location loc) :
        container(cfg, children, loc) {}
      /** @brief Compute the fit width as the maximum child width plus padding. */
      void on_fit_x() override {
        int32_t width = 0;
        for (auto child : elements()) {
          child->on_fit_x();
          int32_t child_width = box(*child).width + child->element_config().margin.left + child->element_config().margin.right;
          width = std::max(width, child_width);
        }
        width += _container_config.padding.left + _container_config.padding.right;
        _box.width = uint16_t(std::clamp<int32_t>(width, _config.sizing.x.min, _config.sizing.x.max));
      }
      /** @brief Compute the fit height as the sum of child heights plus spacing and padding. */
      void on_fit_y() override {
        int32_t height = -_container_config.spacing + _container_config.padding.top + _container_config.padding.bottom;
        for (auto child : elements()) {
          child->on_fit_y();
          int32_t child_height = box(*child).height + child->element_config().margin.top + child->element_config().margin.bottom;
          height += child_height + _container_config.spacing;
        }
        _box.height = uint16_t(std::clamp<int32_t>(height, _config.sizing.y.min, _config.sizing.y.max));
      }
      /** @brief Grow children horizontally proportional to their weight. */
      void on_child_grow_x() override {
        for (auto child : elements()) {
          float weight = std::clamp(child->element_config().sizing.x.weight, 0.0f, 1.0f);
          int32_t available_width = _box.width - _container_config.padding.left - _container_config.padding.right;
          available_width -= child->element_config().margin.left + child->element_config().margin.right;
          uint16_t new_width = uint16_t(std::max<int32_t>(available_width * weight, 0));
          box(*child).width = std::clamp(new_width, box(*child).width, child->element_config().sizing.x.max);
          child->on_child_grow_x();
        }
      }
      /** @brief Grow children vertically using iterative weighted distribution.
       *
       *  Remaining space is distributed proportionally by weight in
       *  multiple passes, with each pass capped to the smallest gap
       *  between scaled child heights to maintain proportional balance. */
      void on_child_grow_y() override {
        if (elements().size() == 0) return;
        // Iteratively distribute remaining height proportional to weight
        while (1) {
          int32_t remaining_height = _box.height - _container_config.padding.top - _container_config.padding.bottom;
          remaining_height -= _container_config.spacing * (elements().size() - 1);
          float total_weight = 0.0f;
          uint16_t smallest = UINT16_MAX;
          uint16_t second_smallest = UINT16_MAX;
          uint16_t height_to_add = UINT16_MAX;
          for (auto child : elements()) {
            total_weight += child->element_config().sizing.y.weight;
            remaining_height -= int32_t(box(*child).height) + child->element_config().margin.top + child->element_config().margin.bottom;
            if (child->element_config().sizing.y.weight == 0.0f) continue;
            uint16_t scaled_height = box(*child).height * child->element_config().sizing.y.weight;
            uint16_t scaled_height_cap = child->element_config().sizing.y.max * child->element_config().sizing.y.weight;
            if (height_to_add < scaled_height_cap) height_to_add = scaled_height_cap;
            // Find the two smallest scaled heights to cap the increment
            if (scaled_height < smallest) {
              second_smallest = smallest;
              smallest = scaled_height;
            } else if (scaled_height > smallest) {
              second_smallest = std::min(second_smallest, scaled_height);
              height_to_add = std::min(height_to_add, uint16_t(second_smallest - smallest));
            }
          }
          if (total_weight == 0.0f) break;
          height_to_add = std::min(height_to_add, uint16_t(remaining_height / total_weight));
          if (height_to_add == 0) break;
          for (auto child : elements()) {
            box(*child).height += height_to_add * child->element_config().sizing.y.weight;
          }
        }
        for (auto child : elements()) 
          child->on_child_grow_y();
      }
    private:
      struct pos_chunk_data {
        float alignment; ///< Average alignment of elements in this chunk
        int32_t len; ///< Total height of all elements in this chunk
        int32_t start; ///< Y position where this chunk begins
        std::vector<std::pair<int32_t,_detail::element*>> elements; ///< Pairs of (height, element pointer)
      };
    public:
      /** @brief Position children horizontally using alignment weights. */
      void on_child_pos_x() override {
        for (auto child : elements()) {
          int32_t available_width = _box.width - _container_config.padding.left - _container_config.padding.right;
          available_width -= child->element_config().margin.left + child->element_config().margin.right;
          available_width -= box(*child).width;
          available_width = std::max(0, available_width);
          box(*child).x = _box.x + 
                          _container_config.padding.left + child->element_config().margin.left + 
                          available_width * std::clamp(child->element_config().alignment.x, 0.0f, 1.0f);
          child->on_child_pos_x();
        }
      }
      /** @brief Position children vertically using multi-pass chunk-based layout.
       *
       *  Groups children into chunks by overlapping alignment expectations,
       *  then positions chunks top-to-bottom and bottom-to-top to resolve
       *  overlaps, and finally positions individual elements within each chunk. */
      void on_child_pos_y() override {
        std::vector<pos_chunk_data> chunks;
        chunks.reserve(elements().size());
        int32_t available_height = _box.height - _container_config.padding.top - _container_config.padding.bottom;

        // 1st pass: Determine chunks
        for (auto child: elements()) {
          int32_t height = box(*child).height + child->element_config().margin.top + child->element_config().margin.bottom;
          int32_t expected_pos = available_height * std::clamp(child->element_config().alignment.y, 0.0f, 1.0f);
          if (chunks.size() > 0) {
            auto& last = chunks.back();
            if (last.len + last.start >= expected_pos) {
              last.len += height + _container_config.spacing;
              last.elements.push_back({height, child});
              continue;
            }
          }
          chunks.push_back({.alignment = child->element_config().alignment.y, .len = height, .start = expected_pos, .elements = {{height, child}}});
        }
        // Compute metadata
        for (auto& chunk : chunks) {
          float total_alignment = 0.0f;
          for (auto child : chunk.elements) {
            total_alignment += child.second->element_config().alignment.y;
          }
          chunk.alignment = total_alignment / chunk.elements.size();
        }
        // 2nd pass: Position chunks top-to-bottom
        int32_t current_y = _container_config.padding.top;
        for (auto& chunk : chunks) {
          int32_t actual_expected_pos = int32_t(chunk.alignment * (available_height - chunk.len));
          chunk.start = std::max(current_y, actual_expected_pos);
          current_y = chunk.start + chunk.len + _container_config.spacing;
        }
        // 3rd pass: Position chunks bottom-to-top, resolve overlaps only
        for (auto i = chunks.rbegin(); i != chunks.rend(); i++) {
          int32_t bottom_edge = _box.height - _container_config.padding.bottom;
          if (i->start + i->len > bottom_edge) {
            i->start = bottom_edge - i->len;
            continue;
          }
          if (i != chunks.rbegin()) if (i->start + i->len + _container_config.spacing > i[-1].start) {
            i->start = i[-1].start - i->len - _container_config.spacing;
            continue;
          }
          break;
        }
        for (auto& chunk : chunks)
          chunk.start = std::max(chunk.start, int32_t(_container_config.padding.top));
        // 4th pass: Position children
        for (auto& chunk : chunks) {
          current_y = chunk.start;
          for (auto child : chunk.elements) {
            box(*child.second).y = _box.y + current_y + child.second->element_config().margin.top;
            current_y += child.first + _container_config.spacing;
            child.second->on_child_pos_y();
          }
        }
      }
    };
  }
  /** @brief Allocate a vertical box layout in arena storage.
   *  @param cfg Container configuration
   *  @param children Child elements to lay out vertically
   *  @param loc Source location for diagnostics
   *  @return Pointer to the allocated vbox */
  inline _detail::vbox* vbox(_config::container_config cfg = {}, std::initializer_list<_detail::identifiable*> children = {}, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::vbox>(cfg, children, loc);
  }
}
