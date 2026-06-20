/** @file
 *  @brief Base container class with child management and layout.
 */

#pragma once

#include "component.hpp"
#include "element.hpp"
#include "identifiable.hpp"
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace cppreact {

  namespace _config {

    #define CPPREACT_CONTAINER_CONFIG \
      layout_padding padding = PAD(); \
      uint16_t spacing = 0;

    /** @brief Configuration combining element and container properties. */
    struct container_config {
      CPPREACT_ELEMENT_CONFIG;
      CPPREACT_CONTAINER_CONFIG;
    };

    /** @brief Container-specific config (padding, spacing) without element fields. */
    struct container_specific_config {
      CPPREACT_CONTAINER_CONFIG;
    };

    /** @brief Extract container-specific fields from a full container_config. */
    inline container_specific_config to_container_specific_config(container_config cfg) {
      container_specific_config cscfg;
      cscfg.spacing = cfg.spacing;
      cscfg.padding = cfg.padding;
      return cscfg;
    }

    /** @brief Copy all config fields from a typed config into a container_config. */
    template<typename T>
    inline container_config to_container_config(T cfg) {
      container_config cc;
      cc.alignment = cfg.alignment;
      cc.margin = cfg.margin;
      cc.sizing = cfg.sizing;
      cc.spacing = cfg.spacing;
      cc.padding = cfg.padding;
      return cc;
    }

  }

  namespace _detail {

    /** @brief Base container that manages child elements and delegates layout/render. */
    class container : public component {
    private:
      std::vector<identifiable*> _children; ///< Child identifiable objects
      std::vector<element*> _elements;      ///< Cached element pointers for layout
    protected:
      /** @brief Access the cached element list. */
      const std::vector<element*>& elements() {return _elements;}
      _config::container_specific_config _container_config; ///< Container-specific layout config
    public:
      /** @brief Construct a container with config, children, and source location. */
      container(_config::container_config cfg, std::initializer_list<identifiable*> children, std::source_location loc) :
        component(to_element_config(cfg), loc), _children(children), _container_config(to_container_specific_config(cfg)) {
          for (auto& child : _children)
            child->set_parent(this);
          _elements.reserve(_children.size());
        }
      /** @brief Update pass — updates the container and all children. */
      void on_update() override {
        element::on_update();
        _elements.clear();
        for (auto& child : _children) {
          element& e = child->get();
          e.on_update();
          _elements.push_back(&e);
        }
      }
      /** @brief Collect render commands from all visible children, clipped to @p render_box. */
      std::vector<render_command *> on_render(bounding_box render_box = {0, 0, 0, 0, UINT16_MAX, UINT16_MAX}) override {
        std::vector<render_command*> commands;
        for (auto& e : _elements) {
          auto clipped = intersect(render_box, box(*e));
          if (clipped.width <= 0 || clipped.height <= 0)
            continue;
          auto child_commands = e->on_render(clipped);
          commands.insert(commands.end(), child_commands.begin(), child_commands.end());
        }
        return commands;
      }
    };
  }
}
