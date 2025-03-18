#ifndef _CPPREACT_FLOATING_HPP
#define _CPPREACT_FLOATING_HPP

#include "component.hpp"

namespace cppreact {
  enum {FLOAT_BEGIN_ID = 10000,FLOAT_END_ID};
  // Floating component
  class floating : public component {
  uint16_t saved_width = 0;
  uint16_t saved_height = 0;
  public:
    floating(layout_config config, std::initializer_list<component*> children = {}) : component(config,children) {
      //this->config.sizing = {SIZING_FIXED(0), SIZING_FIXED(0)};
    }
  protected:
  inline void on_fit_x(bool along) {
      if (config.sizing.x.mode == _FIXED) box.width = config.sizing.x.min;
      if (config.sizing.x.mode != _FIXED) {
        if (along)
          box.width += config.child_gap * 
            ((k_tree.child_count > 1)?k_tree.child_count - 1:0);
        box.width +=  config.padding.left + config.padding.right;
        if (box.width < config.sizing.x.min) box.width = config.sizing.x.min;
      }
  }
  inline void on_fit_y(bool along) {
      if (config.sizing.y.mode == _FIXED) box.height = config.sizing.y.min;
      if (config.sizing.y.mode != _FIXED) {
        if (along)
          box.height += config.child_gap * 
            ((k_tree.child_count > 1)?k_tree.child_count - 1:0);
        box.height += config.padding.top + config.padding.bottom;
        if (box.height < config.sizing.y.min) box.height = config.sizing.y.min;
      }

  }
  private:
  void on_fit_across() override {
    if (config.direction == LEFT_TO_RIGHT) {
      on_fit_x(true);
      saved_width = box.width;
      box.width = 0;
    }
    else {
      on_fit_y(true);
      saved_height = box.height;
      box.height = 0;
    }
  }
  void on_fit_along() override {
    //component::on_fit_along();
    if (config.direction == LEFT_TO_RIGHT) {
      on_fit_y(false);
      saved_height = box.height;
      box.height = 0;
    }
    else {
      on_fit_x(false);
      saved_width = box.width;
      box.width = 0;
    }
  }
  void on_child_position() override {
    if (k_tree.parent) {
      if (config.alignment.x == ALIGN_X_LEFT) {
        box.x = k_tree.parent->box.x;
      }
      if (config.alignment.x == ALIGN_X_RIGHT) {
        box.x = k_tree.parent->box.x + k_tree.parent->box.width;
      }
      if (config.alignment.x == ALIGN_X_CENTER) {
        box.x = k_tree.parent->box.x + k_tree.parent->box.width / 2 - saved_width / 2;
      }
      if (config.alignment.y == ALIGN_Y_TOP) {
        box.y = k_tree.parent->box.y;
      }
      if (config.alignment.y == ALIGN_Y_BOTTOM) {
        box.y = k_tree.parent->box.y + k_tree.parent->box.height;
      }
      if (config.alignment.y == ALIGN_Y_CENTER) {
        box.y = k_tree.parent->box.y + k_tree.parent->box.height / 2 - saved_height / 2;
      }
    }

    component::on_child_position();
  }
  std::list<render_command> on_layout() override {
    std::list<render_command> res = {{.box = box,.id = FLOAT_BEGIN_ID}};
    res.splice(res.end(),std::move(component::on_layout()));
    res.push_back({.box = box,.id = FLOAT_END_ID});
    return res;
  }
  };
}

#endif
