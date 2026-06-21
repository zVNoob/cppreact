/** @file
 *  @brief Horizontal box layout container. */

#pragma once

#include "../internal/container.hpp"
#include "../internal/arena.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace cppreact {
  namespace _detail {
    /** @brief Horizontal box layout that arranges children in a row.
     *
     *  Children are arranged from left to right. The box height matches
     *  the tallest child plus padding; width is the sum of all child
     *  widths plus spacing and padding. */
    class hbox : public container {
    public:
      /** @brief Construct an hbox with configuration, children, and source location.
       *  @param cfg Container configuration
       *  @param children Child elements to lay out
       *  @param loc Source location for diagnostics */
      hbox(_config::container_config cfg, std::initializer_list<identifiable*> children, std::source_location loc) :
        container(cfg, children, loc) {}
      /** @brief Compute the fit width as the sum of child widths plus spacing and padding. */
      void on_fit_x() override {
        int32_t width = -_container_config.spacing + _container_config.padding.left + _container_config.padding.right;
        for (auto child : elements()) {
          child->on_fit_x();
          int32_t child_width = box(*child).width + child->element_config().margin.left + child->element_config().margin.right;
          width += child_width + _container_config.spacing;
        }
        _box.width = std::clamp(width, int32_t(_config.sizing.x.min), int32_t(_config.sizing.x.max));
      }
      /** @brief Compute the fit height as the maximum child height plus padding. */
      void on_fit_y() override {
        int32_t height = 0;
        for (auto child : elements()) {
          child->on_fit_y();
          int32_t child_height = box(*child).height + child->element_config().margin.top + child->element_config().margin.bottom;
          height = std::max(height, child_height);
        }
        height += _container_config.padding.top + _container_config.padding.bottom;
        _box.height = uint16_t(std::clamp(height, int32_t(_config.sizing.y.min), int32_t(_config.sizing.y.max)));
      }
      /** @brief Grow children horizontally using iterative weighted distribution.
       *
       *  Remaining space is distributed proportionally by weight in
       *  multiple passes, with each pass capped to the smallest gap
       *  between scaled child widths to maintain proportional balance. */
      void on_child_grow_x() override {
        if (elements().size() == 0) return;
        // Iteratively distribute remaining width proportional to weight
        while (1) {
          int32_t remaining_width = _box.width - _container_config.padding.left - _container_config.padding.right;
          remaining_width -= _container_config.spacing * (elements().size() - 1);
          float total_weight = 0.0f;
          uint16_t smallest = UINT16_MAX;
          uint16_t second_smallest = UINT16_MAX;
          uint16_t width_to_add = UINT16_MAX;
          for (auto child : elements()) {
            total_weight += child->element_config().sizing.x.weight;
            remaining_width -= box(*child).width + child->element_config().margin.left + child->element_config().margin.right;
            if (child->element_config().sizing.x.weight == 0.0f) continue;
            uint16_t scaled_width = box(*child).width * child->element_config().sizing.x.weight;
            uint16_t scaled_width_cap = child->element_config().sizing.x.max * child->element_config().sizing.x.weight;
            if (width_to_add < scaled_width_cap) width_to_add = scaled_width_cap;
            // Find the two smallest scaled widths to cap the increment
            if (scaled_width < smallest) {
              second_smallest = smallest;
              smallest = scaled_width;
            } else if (scaled_width > smallest) {
              second_smallest = std::min(second_smallest, scaled_width);
              width_to_add = std::min(width_to_add, uint16_t(second_smallest - smallest));
            }
          }
          if (total_weight == 0.0f) break;
          width_to_add = std::min(width_to_add, uint16_t(remaining_width / total_weight));
          if (width_to_add == 0) break;
          for (auto child : elements()) {
            box(*child).width += width_to_add * child->element_config().sizing.x.weight;
          }
        }
        for (auto child : elements()) 
          child->on_child_grow_x();
      }
      /** @brief Grow children vertically proportional to their weight. */
      void on_child_grow_y() override {
        for (auto child : elements()) {
          float weight = std::clamp(child->element_config().sizing.y.weight, 0.0f, 1.0f);
          int32_t available_height = _box.height - _container_config.padding.top - _container_config.padding.bottom;
          available_height -= child->element_config().margin.top + child->element_config().margin.bottom;
          uint16_t new_height = std::max<int32_t>(available_height * weight, 0);
          box(*child).height = std::clamp(new_height, box(*child).height, child->element_config().sizing.y.max);
          child->on_child_grow_y();
        }
      }
    private:
      struct pos_chunk_data {
        float alignment; ///< Average alignment of elements in this chunk
        int32_t len; ///< Total width of all elements in this chunk
        int32_t start; ///< X position where this chunk begins
        std::vector<std::pair<int32_t,_detail::element*>> elements; ///< Pairs of (width, element pointer)
      };
    public:
      /** @brief Position children horizontally using multi-pass chunk-based layout.
       *
       *  Groups children into chunks by overlapping alignment expectations,
       *  then positions chunks left-to-right and right-to-left to resolve
       *  overlaps, and finally positions individual elements within each chunk. */
      void on_child_pos_x() override {
        std::vector<pos_chunk_data> chunks;
        chunks.reserve(elements().size());
        int32_t available_width = _box.width - _container_config.padding.left - _container_config.padding.right;

        // 1st pass: Determine chunks
        for (auto child: elements()) {
          int32_t width = box(*child).width + child->element_config().margin.left + child->element_config().margin.right;
          int32_t expected_pos = available_width * std::clamp(child->element_config().alignment.x, 0.0f, 1.0f);
          if (chunks.size() > 0) {
            auto& last = chunks.back();
            if (last.len + last.start >= expected_pos) {
              last.len += width + _container_config.spacing;
              last.elements.push_back({width, child});
              continue;
            }
          }
          chunks.push_back({.alignment = child->element_config().alignment.x, .len = width, .start = expected_pos, .elements = {{width, child}}});
        }
        // Compute metadata
        for (auto& chunk : chunks) {
          float total_alignment = 0.0f;
          for (auto child : chunk.elements) {
            total_alignment += child.second->element_config().alignment.x;
          }
          chunk.alignment = total_alignment / chunk.elements.size();
        }
        // 2nd pass: Position chunks left-to-right
        int32_t current_x = _container_config.padding.left;
        for (auto& chunk : chunks) {
          int32_t actual_expected_pos = int32_t(chunk.alignment * (available_width - chunk.len));
          chunk.start = std::max(current_x, actual_expected_pos);
          current_x = chunk.start + chunk.len + _container_config.spacing;
        }
        // 3rd pass: Position chunks right-to-left, resolve overlaps only
        for (auto i = chunks.rbegin(); i != chunks.rend(); i++) {
          int32_t right_edge = _box.width - _container_config.padding.right;
          if (i->start + i->len > right_edge) {
            i->start = right_edge - i->len;
            continue;
          }
          if (i != chunks.rbegin()) if (i->start + i->len + _container_config.spacing > i[-1].start) {
            i->start = i[-1].start - i->len - _container_config.spacing;
            continue;
          }
          break;
        }
        for (auto& chunk : chunks)
          chunk.start = std::max(chunk.start, int32_t(_container_config.padding.left));
        // 4th pass: Position children
        for (auto& chunk : chunks) {
          current_x = chunk.start;
          for (auto child : chunk.elements) {
            box(*child.second).x = _box.x + current_x + child.second->element_config().margin.left;
            current_x += child.first + _container_config.spacing;
            child.second->on_child_pos_x();
          }
        }
      }
      /** @brief Position children vertically using alignment weights. */
      void on_child_pos_y() override {
        for (auto child : elements()) {
          int32_t available_height = _box.height - _container_config.padding.top - _container_config.padding.bottom;
          available_height -= child->element_config().margin.top + child->element_config().margin.bottom;
          available_height -= box(*child).height;
          available_height = std::max(0, available_height);
          box(*child).y = _box.y + 
                          _container_config.padding.top + child->element_config().margin.top + 
                          available_height * std::clamp(child->element_config().alignment.y, 0.0f, 1.0f);
          child->on_child_pos_y();
        }
      }
    };
  }
  /** @brief Allocate a horizontal box layout in arena storage.
   *  @param cfg Container configuration
   *  @param children Child elements to lay out horizontally
   *  @param loc Source location for diagnostics
   *  @return Pointer to the allocated hbox */
  inline _detail::hbox* hbox(_config::container_config cfg = {}, std::initializer_list<_detail::identifiable*> children = {}, std::source_location loc = std::source_location::current()) {
    return _storage::allocate<_detail::hbox>(cfg, children, loc);
  }
}
