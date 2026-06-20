/** @file
 *  @brief Simple component base combining identifiable and element.
 */

#pragma once

#include "identifiable.hpp"
#include "element.hpp"

namespace cppreact {
  namespace _detail {

    /** @brief Base class merging identifiable (hashed ID) with element (layout/render).
     *
     *  Components are the primary building blocks: they are uniquely
     *  identifiable and participate in the element layout pipeline.
     */
    class component : public identifiable, public element {
    public:
      component(_config::element_config cfg, std::source_location loc) :
        identifiable(loc),element(cfg) {}
      virtual ~component() {}
      /** @brief Return this component as the underlying element. */
      element& get() override {return *this;};
    };
  }
}
