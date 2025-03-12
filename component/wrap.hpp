#ifndef _CPPREACT_WRAP_HPP
#define _CPPREACT_WRAP_HPP

#include "component.hpp"
#include <cstdint>

namespace cppreact {
  class wrap : public component {
  uint16_t spacing;
  public:
    wrap(uint16_t spacing,layout_config config, std::initializer_list<component*> children = std::initializer_list<component*>()) :
      component(config, children),spacing(spacing) {}
  protected:
    void on_fit_along() override {
      if (k_tree.parent) {
        if (k_tree.parent->config.direction == LEFT_TO_RIGHT) box.width = 0;
        else box.height = 0;
      } else box.width = 0;
    }
    void on_fit_across() override {
      if (k_tree.parent) {
        if (k_tree.parent->config.direction == LEFT_TO_RIGHT) box.height = 0;
        else box.width = 0;
      } else box.height = 0;
    }
    void on_child_position() override {
      if (config.direction == LEFT_TO_RIGHT) on_child_position_left_to_right();
      else on_child_position_top_to_bottom();
    }
  private:
    inline void on_child_position_left_to_right() {
      uint16_t current_x = config.padding.left;
      uint16_t current_y = config.padding.top;
      uint16_t y_size = 0;
      for (auto i = k_tree.child_begin;i;i = i->k_tree.next_sibling) {
        if ((i->box.width + current_x) > (box.width - config.padding.right)) {
          current_x = config.padding.left;
          current_y += y_size + spacing;
          y_size = 0;
        }
        if (i->config.alignment.x == XLEFT) {
          i->box.x = current_x;
          i->box.y = current_y;
          current_x += i->box.width + config.child_gap;
          y_size = std::max(y_size,i->box.height);
        }
        else if (i->config.alignment.x == XRIGHT) {
          auto j = i;
          for (;j;j = j->k_tree.next_sibling) {
            if (j->config.alignment.x != XRIGHT) break;
            current_x += j->box.width + config.child_gap;
            if (current_x > (box.width - config.padding.right)) break;
          }
          if (j) {
            if (j!=i)
              if (j->k_tree.prev_sibling->config.alignment.x == XRIGHT) 
                j = j->k_tree.prev_sibling;
          }
          else if (k_tree.child_end)
            if (k_tree.child_end->config.alignment.x == XRIGHT)
              j = k_tree.child_end;
          if (j) {
            current_x = box.width - config.padding.right;
            for (auto k = j;k != i->k_tree.prev_sibling;k = k->k_tree.prev_sibling) {
              current_x -= k->box.width;
              k->box.x = current_x;
              k->box.y = current_y;
              current_x -= config.child_gap;
              y_size = std::max(y_size,k->box.height);
            }
          }
          current_x = box.width - config.padding.right;
          i = j;
        } else if (i->config.alignment.x == XCENTER) {
          auto j = i;
          auto temp = current_x;
          for (;j;j = j->k_tree.next_sibling) {
            if (j->config.alignment.x != XCENTER) break;
            current_x += j->box.width + config.child_gap;
            if (current_x > (box.width - config.padding.right)) break;
          }
          if (j) {
            if (j!=i)
              if (j->k_tree.prev_sibling->config.alignment.x == XCENTER) 
                j = j->k_tree.prev_sibling;
          }
          else if (k_tree.child_end)
            if (k_tree.child_end->config.alignment.x == XCENTER)
              j = k_tree.child_end;
          if (j) {
            current_x = std::max(temp,uint16_t(box.width/2 - (current_x - temp) / 2));
            for (auto k = i;k != j->k_tree.next_sibling;k = k->k_tree.next_sibling) {
              k->box.x = current_x;
              k->box.y = current_y;
              current_x += k->box.width +  config.child_gap;
              y_size = std::max(y_size,k->box.height);
            }
          }
          i = j;
        }
      }
    }
    inline void on_child_position_top_to_bottom() {
      uint16_t current_x = config.padding.left;
      uint16_t current_y = config.padding.top;
      uint16_t x_size = 0;
      for (auto i = k_tree.child_begin;i;i = i->k_tree.next_sibling) {
        if ((i->box.height + current_y) > (box.height - config.padding.bottom)) {
          current_x += x_size + spacing;
          current_y = config.padding.top;
          x_size = 0;
        }
        if (i->config.alignment.y == YTOP) {
          i->box.x = current_x;
          i->box.y = current_y;
          current_y += i->box.height + config.child_gap;
          x_size = std::max(x_size,i->box.width);
        }
        else if (i->config.alignment.y == YBOTTOM) {
          auto j = i;
          for (;j;j = j->k_tree.next_sibling) {
            if (j->config.alignment.y != YBOTTOM) break;
            current_y += j->box.height + config.child_gap;
            if (current_y > (box.height - config.padding.right)) break;
          }
          if (j) {
            if (j!=i)
              if (j->k_tree.prev_sibling->config.alignment.y == YBOTTOM) 
                j = j->k_tree.prev_sibling;
          }
          else if (k_tree.child_end)
            if (k_tree.child_end->config.alignment.y == YBOTTOM)
              j = k_tree.child_end;
          if (j) {
            current_y = box.height - config.padding.bottom;
            for (auto k = j;k != i->k_tree.prev_sibling;k = k->k_tree.prev_sibling) {
              current_y -= k->box.height;
              k->box.x = current_x;
              k->box.y = current_y;
              current_x -= config.child_gap;
              x_size = std::max(x_size,k->box.width);
            }
          }
          current_x = box.height - config.padding.bottom;
          i = j;
        } else if (i->config.alignment.y == YCENTER) {
          auto j = i;
          auto temp = current_y;
          for (;j;j = j->k_tree.next_sibling) {
            if (j->config.alignment.y != YCENTER) break;
            current_y += j->box.height + config.child_gap;
            if (current_y > (box.height - config.padding.bottom)) break;
          }
          if (j) {
            if (j!=i)
              if (j->k_tree.prev_sibling->config.alignment.y == YCENTER) 
                j = j->k_tree.prev_sibling;
          }
          else if (k_tree.child_end)
            if (k_tree.child_end->config.alignment.y == YCENTER)
              j = k_tree.child_end;
          if (j) {
            current_y = std::max(temp,uint16_t(box.height/2 - (current_y - temp) / 2));
            for (auto k = i;k != j->k_tree.next_sibling;k = k->k_tree.next_sibling) {
              k->box.x = current_x;
              k->box.y = current_y;
              current_y += k->box.height +  config.child_gap;
              x_size = std::max(x_size,k->box.width);
            }
          }
          i = j;
        }
      }
    }
  };
}

#endif
