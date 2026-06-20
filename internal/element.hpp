/** @file
 *  @brief Core element base classes for layout, sizing, and rendering.
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

namespace cppreact {

/** @brief Layout sizing configuration for a single axis (X or Y).
 *
 *  Controls minimum/maximum size and weight for flex-like layout.
 */
struct layout_sizing_axis {
  uint16_t min = 0;          ///< Minimum size in pixels
  uint16_t max = UINT16_MAX; ///< Maximum size in pixels
  float weight = 0.0f;       ///< Flex weight for distributing space
};

/** @brief Create a fixed-size layout axis. */
inline layout_sizing_axis FIXED(uint16_t size) {return layout_sizing_axis{size, size, 0.0f};}
/** @brief Create a fit-to-content layout axis. */
inline layout_sizing_axis FIT(uint16_t min = 0, uint16_t max = UINT16_MAX) {return layout_sizing_axis{min, max, 0.0f};}
/** @brief Create a growable layout axis with weight. */
inline layout_sizing_axis GROW(uint16_t min, uint16_t max, float weight = 1.0f) {return layout_sizing_axis{min, max, weight};}
/** @brief Create a fully growable layout axis with weight. */
inline layout_sizing_axis GROW(float weight = 1.0f) {return layout_sizing_axis{0, UINT16_MAX, weight};}
/** @brief Create a growable axis with a minimum size. */
inline layout_sizing_axis GROW_MIN(uint16_t min = 0, float weight = 1.0f) {return layout_sizing_axis{min, UINT16_MAX, weight};}
/** @brief Create a growable axis with a maximum size. */
inline layout_sizing_axis GROW_MAX(uint16_t max = UINT16_MAX, float weight = 1.0f) {return layout_sizing_axis{0, max, weight};}

/** @brief Padding/margin offsets for the four edges. */
struct layout_padding{
  int16_t left = 0;   ///< Left edge offset
  int16_t top = 0;    ///< Top edge offset
  int16_t right = 0;  ///< Right edge offset
  int16_t bottom = 0; ///< Bottom edge offset
};

/** @brief Convenience builder for uniform padding. */
inline layout_padding PAD(int16_t pad = 0) {return layout_padding{pad, pad, pad, pad};}

const float ALIGN_BEGIN  = 0.0; ///< Align to start edge
const float ALIGN_CENTER = 0.5; ///< Align to center
const float ALIGN_END    = 1.0; ///< Align to end edge

  namespace _config {

    /** @brief Alignment pair (x and y) using fractional values. */
    struct alignment_pair { float x = ALIGN_BEGIN; float y = ALIGN_BEGIN; };
    /** @brief Sizing pair for both axes. */
    struct sizing_pair { layout_sizing_axis x = FIT(); layout_sizing_axis y = FIT(); };

    #define CPPREACT_ELEMENT_CONFIG \
      sizing_pair sizing = {FIT(), FIT()}; \
      layout_padding margin = PAD(); \
      alignment_pair alignment = {ALIGN_BEGIN, ALIGN_BEGIN};

    /** @brief Configuration structure inheriting element layout properties. */
    struct element_config {CPPREACT_ELEMENT_CONFIG;};

    /** @brief Copy element-level config fields from a typed config into a generic element_config. */
    template<typename T>
    element_config to_element_config(T cfg) {
      element_config ec;
      ec.alignment = cfg.alignment;
      ec.margin = cfg.margin;
      ec.sizing = cfg.sizing;
      return ec;
    }
  }

/** @brief 2D bounding box with position, size, and scroll offsets. */
struct bounding_box {
  int32_t x = 0;        ///< Absolute X position
  int32_t y = 0;        ///< Absolute Y position
  uint16_t offset_x = 0;///< Scroll offset in X
  uint16_t offset_y = 0;///< Scroll offset in Y
  uint16_t width = 0;   ///< Box width
  uint16_t height = 0;  ///< Box height
};



  namespace _detail {

/** @brief Base class for render commands, extended by subclasses to provide rendering information. */
  class render_command {
    public:
      bounding_box box; ///< The bounding box for this render command
      render_command(bounding_box box) : box(box) {}
      virtual ~render_command(){}
  };

/** @brief Base element class providing layout computation and rendering interface.
 *
 *  Handles sizing, alignment, bounding box management, and the render-command pipeline.
 *  Subclasses override layout passes and on_render to produce drawable content.
 */
  class element {
    protected:
      bounding_box _box;              ///< Computed bounding box
      _config::element_config _config;///< Element layout configuration

      /** @brief Access the mutable bounding box of an element. */
      static bounding_box& box(element& e) {return e._box;}
    public:
      /** @brief Construct an element with the given layout configuration. */
      element(_config::element_config cfg) {
        _config = cfg;
      }
      /** @brief Get the element's layout configuration. */
      const _config::element_config& element_config() const {return _config;}
      /** @brief Get the element's computed bounding box. */
      const bounding_box& box() const {return _box;}
      element(const element&) = delete;
      element& operator=(const element&) = delete;
      virtual ~element() {}
    public:
      /** @brief Update pass. Resets the box to minimum size. */
      virtual void on_update() {_box = {0, 0, 0, 0, _config.sizing.x.min, _config.sizing.y.min};};
      /** @brief Fit pass for the X axis. Sets width to minimum. */
      virtual void on_fit_x() {_box.width = _config.sizing.x.min;}
      /** @brief Fit pass for the Y axis. Sets height to minimum. */
      virtual void on_fit_y() {_box.height = _config.sizing.y.min;}
      /** @brief Grow pass for the X axis (distribute remaining space). */
      virtual void on_child_grow_x() {}
      /** @brief Grow pass for the Y axis (distribute remaining space). */
      virtual void on_child_grow_y() {}
      /** @brief Position pass for children along X. */
      virtual void on_child_pos_x() {}
      /** @brief Position pass for children along Y. */
      virtual void on_child_pos_y() {}

      /** @brief Compute the intersection of two bounding boxes.
       *  @return Intersection box, or a zero box if they do not overlap.
       */
      static bounding_box intersect(bounding_box a, bounding_box b) {
        int32_t left = std::max(a.x, b.x);
        int32_t right = std::min(a.x + a.width, b.x + b.width);
        int32_t top = std::max(a.y, b.y);
        int32_t bottom = std::min(a.y + a.height, b.y + b.height);
        if (right <= left || bottom <= top) return {};
        bounding_box r;
        r.x = left;
        r.y = top;
        r.width = uint16_t(right - left);
        r.height = uint16_t(bottom - top);
        r.offset_x = a.offset_x + uint16_t(left - b.x);
        r.offset_y = a.offset_y + uint16_t(top - b.y);
        return r;
      }
      /** @brief Clip box @p a against @p b, resetting the scroll origin to @p b.
       *  @return Clipped box relative to @p b.
       */
      static bounding_box clip(bounding_box a, bounding_box b) {
        int32_t left = std::max(a.x, b.x);
        int32_t right = std::min(a.x + a.width, b.x + b.width);
        int32_t top = std::max(a.y, b.y);
        int32_t bottom = std::min(a.y + a.height, b.y + b.height);
        if (right <= left || bottom <= top) return {};
        bounding_box r;
        r.x = left;
        r.y = top;
        r.width = uint16_t(right - left);
        r.height = uint16_t(bottom - top);
        r.offset_x = uint16_t(left - b.x);
        r.offset_y = uint16_t(top - b.y);
        return r;
      }
      /** @brief Produce render commands for the visible region.
       *  @return List of render commands.
       */
      virtual std::vector<render_command*> on_render(bounding_box) {return {};};
  };
  }   // namespace _detail
} // namespace cppreact
