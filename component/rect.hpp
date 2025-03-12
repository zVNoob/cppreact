#ifndef _CPPREACT_RECT_HPP
#define _CPPREACT_RECT_HPP
#include "component.hpp"
#include <cstdint>
#include <initializer_list>

namespace cppreact {
  struct color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
  };
  struct color_ref {
    uint8_t* r;
    uint8_t* g;
    uint8_t* b;
    uint8_t* a;
  };
  enum {RECT_RENDER_ID = 1000};
  class rect : public component {
    color c;
    color_ref r;
    bool use_ref;
    protected:
    std::list<render_command> on_layout() override {
      if (use_ref) c = {*r.r,*r.g,*r.b,*r.a};
      return {{.box = box,.id = RECT_RENDER_ID,.data = &c}};
    }
    public:
    rect(color col,
         layout_config config,
         std::initializer_list<component*> children = std::initializer_list<component*>()) :
          c(col),component(config,children) {
      use_ref = false;
    }
    rect(color_ref col,
         layout_config config,
         std::initializer_list<component*> children = std::initializer_list<component*>()) :
          r(col), component(config,children) {
      use_ref = true;
    }

  };
}

#endif
