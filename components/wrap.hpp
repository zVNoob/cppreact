#ifndef _CPPREACT_WRAP_HPP
#define _CPPREACT_WRAP_HPP

#include "component.hpp"
#include <cstdint>

namespace cppreact {
  // Wrap the child components, like flex CSS attribute
  class wrap : public component {
  uint16_t spacing;
  public:
    wrap(uint16_t spacing,layout_config config, std::initializer_list<component*> children = std::initializer_list<component*>()) :
      component(config, children),spacing(spacing) {}
  protected:
    void on_fit_along() override {
      if (tree.parent) {
        if (tree.parent->get_config().direction == LEFT_TO_RIGHT) {
          if (config.sizing.x.mode == _GROW)
            box.width = 0;
        }
        else if (config.sizing.y.mode == _GROW) box.height = 0;
      } else if (config.sizing.x.mode == _GROW) box.width = 0;
    }
    void on_wrap() override {
    }
    void on_child_grow_across() override {
      component::on_child_grow_across();
      if (config.direction == LEFT_TO_RIGHT) on_wrap_left_to_right();
      else on_wrap_top_to_bottom();
    }
    
    void on_child_pos() override {
      for (component *i = tree.begin; i; i = i->next_sibling()) {
        i->box.x += box.x;
        i->box.y += box.y;
      }
    }
  private:
  inline void on_wrap_left_to_right() {
    uint16_t x = config.padding.left;
    uint16_t y = config.padding.top;
    uint16_t max_y = 0;
    for (component *i = tree.begin; i; i = i->next_sibling()) {
      if (x + i->box.width + config.padding.right > box.width) {
        x = config.padding.left;
        y += max_y + spacing;
        max_y = 0;
      }
      if (i->get_config().alignment.x == ALIGN_X_LEFT) {
        i->box.x = x;
        i->box.y = y;
        x += i->box.width + config.child_gap;
      }
      if (i->get_config().alignment.x == ALIGN_X_RIGHT) {
        uint16_t len = 0;
        component* j = i;
        for (; j; j = j->next_sibling()) {
          len += j->box.width + config.child_gap;
          if (j->get_config().alignment.x != ALIGN_X_RIGHT) {
            len -= j->box.width + config.child_gap;
            break;
          }
          if (len > box.width - config.padding.right) {
            len -= j->box.width + config.child_gap;
            break;
          }
        }
        if (i == j) {
          uint16_t p1 = (box.width - config.padding.right);
          uint16_t p2 = i->box.width;
          i->box.x = (p1 > p2)? p1 - p2:0;
          i->box.x = std::max(config.padding.left,i->box.x);
          i->box.y = y;
        } else for (component* k = i; k != j; k = k->next_sibling()) {
          k->box.x = box.width - len;
          k->box.y = y;
          len -= k->box.width + config.child_gap;
        }
        x = box.width;
      }
      if (i->get_config().alignment.x == ALIGN_X_CENTER) {
        uint16_t len = 0;
        component* j = i;
        for (; j; j = j->next_sibling()) {
          len += j->box.width + config.child_gap;
          if (j->get_config().alignment.x != ALIGN_X_CENTER) {
            len -= j->box.width + config.child_gap;
            break;
          }
          if (len > box.width - config.padding.right) {
            len -= j->box.width + config.child_gap;
            break;
          }
        }
        if (i == j) {
          uint16_t p1 = (box.width - config.padding.right) / 2;
          uint16_t p2 = i->box.width / 2;
          i->box.x = (p1 > p2)?p1-p2:0;
          i->box.x = std::max(config.padding.left,i->box.x);
          i->box.y = y;
        } else for (component* k = i; k != j; k = k->next_sibling()) {
          k->box.x = std::max((uint16_t)(box.width / 2 - len / 2),x);
          len -= k->box.width + config.child_gap;
          x = k->box.x + k->box.width + config.child_gap;
          k->box.y = y;
        }

      }
      max_y = std::max(max_y, i->box.height);
    }
    box.height = y + max_y + config.padding.bottom;
  }
  inline void on_wrap_top_to_bottom() {
    uint16_t x = config.padding.left;
    uint16_t y = config.padding.top;
    uint16_t max_x = 0;
    for (component *i = tree.begin; i; i = i->next_sibling()) {
      if (y + i->box.height + config.padding.bottom > box.height) {
        y = config.padding.top;
        x += max_x + spacing;
        max_x = 0;
      }
      if (i->get_config().alignment.y == ALIGN_Y_TOP) {
        i->box.x = x;
        i->box.y = y;
        y += i->box.height + config.child_gap;
      }
      if (i->get_config().alignment.y == ALIGN_Y_BOTTOM) {
        uint16_t len = 0;
        component* j = i;
        for (; j; j = j->next_sibling()) {
          len += j->box.height + config.child_gap;
          if (j->get_config().alignment.y != ALIGN_Y_BOTTOM) {
            len -= j->box.height + config.child_gap;
            break;
          }
          if (len > box.height - config.padding.bottom) {
            len -= j->box.height + config.child_gap;
            break;
          }
        }
        if (i == j) {
          uint16_t p1 = (box.height - config.padding.bottom);
          uint16_t p2 = i->box.height;
          i->box.y = (p1 > p2)? p1 - p2:0;
          i->box.y = std::max(config.padding.top,i->box.y);
          i->box.x = x;
        } else for (component* k = i; k != j; k = k->next_sibling()) {
          k->box.y = box.height - len;
          k->box.x = x;
          len -= k->box.height + config.child_gap;
        }
        y = box.height;
      }
      if (i->get_config().alignment.y == ALIGN_Y_CENTER) {
        uint16_t len = 0;
        component* j = i;
        for (; j; j = j->next_sibling()) {
          len += j->box.height + config.child_gap;
          if (j->get_config().alignment.y != ALIGN_Y_CENTER) {
            len -= j->box.height + config.child_gap;
            break;
          }
          if (len > box.height - config.padding.bottom) {
            len -= j->box.height + config.child_gap;
            break;
          }
        }
        if (i == j) {
          uint16_t p1 = (box.height - config.padding.bottom) / 2;
          uint16_t p2 = i->box.height / 2;
          i->box.y = (p1 > p2)?p1-p2:0;
          i->box.y = std::max(config.padding.top,i->box.y);
          i->box.x = x;
        } else for (component* k = i; k != j; k = k->next_sibling()) {
          k->box.y = std::max((uint16_t)(box.height / 2 - len / 2),y);
          len -= k->box.height + config.child_gap;
          y = k->box.y + k->box.height + config.child_gap;
          k->box.x = x; 
        }
      }
    }
  }

  };
}

#endif
