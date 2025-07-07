#ifndef _CPPREACT_ABSOLUTE_HPP
#define _CPPREACT_ABSOLUTE_HPP

#include "component.hpp"
namespace cppreact {
  // Absolute position component, for fixed positioning pop-up
  class absolute : public component {
    uint16_t x,y;
    uint16_t width,height;
    public:
    absolute(uint16_t x, uint16_t y, layout_config config, 
             std::initializer_list<component*> children = {}) : 
      x(x),y(y),component(config,children) {}
    protected:
    void on_fit_across() override {
      if (config.direction == LEFT_TO_RIGHT) {
        width = box.width;
        if (width < config.sizing.x.min) width = config.sizing.x.min;
        box.width = 0;
      } else {
        height = box.height;
        if (height < config.sizing.y.min) height = config.sizing.y.min;
        box.height = 0;
      }
    }
    void on_fit_along() override {
      if (config.direction == LEFT_TO_RIGHT) {
        height = box.height;
        if (height < config.sizing.y.min) height = config.sizing.y.min;
        box.height = 0;
      } else {
        width = box.width;
        if (width < config.sizing.x.min) width = config.sizing.x.min;
        box.width = 0;
      }
    }
    void on_child_grow_across() override {
      if (config.direction == LEFT_TO_RIGHT) {
        box.width = width;
        component::on_child_grow_across();
      } else {
        box.height = height;
        component::on_child_grow_across();
      }
    }
    void on_child_grow_along() override {
      if (config.direction == LEFT_TO_RIGHT) {
        box.height = height;
        component::on_child_grow_along();
      } else {
        box.width = width;
        component::on_child_grow_along();
      }
    }
    void on_child_pos() override {
        box.x = x;
        box.y = y;
        component::on_child_pos();
    }
  };
}

#endif
