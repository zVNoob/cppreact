#ifndef _CPPREACT_RECT_HPP
#define _CPPREACT_RECT_HPP

#include "component.hpp"
#include <cstdint>

namespace cppreact {
  struct color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
  };
  enum _BlendMode {BLEND_NONE,BLEND_ADD,BLEND_MULTIPLY};
  // A color box
  class rect : public component {
  public:
    color col;
    _BlendMode blend = BLEND_NONE;
    rect(color col,layout_config config,std::initializer_list<component*> children = {}) : 
      col(col),component(config,children) {
    }
  };
}

#endif
