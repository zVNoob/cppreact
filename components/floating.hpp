#ifndef _CPPREACT_FLOATING_HPP
#define _CPPREACT_FLOATING_HPP

#include "component.hpp"
#include <initializer_list>

namespace cppreact {
  // Floating position component, suitable for relative pop-up
  class floating : public component {
    uint16_t saved_width = 0;
    uint16_t saved_height = 0;
  public:
    floating(_LayoutAlignmentX align_x, _LayoutAlignmentY align_y, _LayoutDirection direction,
             std::initializer_list<component*> child = {}) : component({},child) {
      config.alignment.x = align_x;
      config.alignment.y = align_y;
      config.direction = direction;
    }
    floating(std::initializer_list<component*> child = {}) : component({},child) {
      config.alignment.x = ALIGN_X_LEFT;
      config.alignment.y = ALIGN_Y_BOTTOM;
      config.direction = TOP_TO_BOTTOM;
    }
  protected:
    void on_fit_across() override {
      component::on_fit_across();
      if (config.direction == LEFT_TO_RIGHT) {
        saved_width = box.width;
        box.width = 0;
      }
      else {
        saved_height = box.height;
        box.height = 0;
      }
    }
    void on_fit_along() override {
      component::on_fit_along();
      if (config.direction == LEFT_TO_RIGHT) {
        saved_height = box.height;
        box.height = 0;
      }
      else {
        saved_width = box.width;
        box.width = 0;
      }
    }
    void on_wrap() override {
      box.width = saved_width;
      box.height = saved_height;
      component::on_wrap();
      box.width = 0;
      box.height = 0;
    }
    void on_child_pos() override {
      if (tree.parent) {
        if (config.alignment.x == ALIGN_X_LEFT) {
          box.x = tree.parent->box.x;
        }
        if (config.alignment.x == ALIGN_X_RIGHT) {
          box.x = tree.parent->box.x + tree.parent->box.width;
        }
        if (config.alignment.x == ALIGN_X_CENTER) {
          box.x = tree.parent->box.x + tree.parent->box.width / 2 - saved_width / 2;
        }
        if (config.alignment.y == ALIGN_Y_TOP) {
          box.y = tree.parent->box.y;
        }
        if (config.alignment.y == ALIGN_Y_BOTTOM) {
          box.y = tree.parent->box.y + tree.parent->box.height;
        }
        if (config.alignment.y == ALIGN_Y_CENTER) {
          box.y = tree.parent->box.y + tree.parent->box.height / 2 - saved_height / 2;
        }
    }

    component::on_child_pos();
  }
  };
}

#endif
