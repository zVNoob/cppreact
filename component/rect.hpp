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

  enum {RECT_RENDER_ID = 1};
  class rect : public component {
    color c;
    protected:
    std::list<render_command> on_layout() override {
      std::list<render_command> res = {{.box = box,.id = RECT_RENDER_ID,.data = &c}};
      std::list<render_command> temp = std::move(component::on_layout());
      res.insert(res.end(),temp.begin(),temp.end());
      return res;
    }
    public:
    rect(color col,
         layout_config config,
         std::initializer_list<component*> children = std::initializer_list<component*>()) :
          c(col),component(config,children) {
    }


  };
}

#endif
