#ifndef CPPREACT_COMPONENT_HPP
#define CPPREACT_COMPONENT_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <list>

#ifdef DEBUG
#include <stdexcept>
#endif

namespace cppreact {
  // Layout Sizing API
  enum _LayoutSizingMode {_FIT,_GROW,_FIXED};
  struct _layout_sizing_axis {
    _LayoutSizingMode mode = _FIT;
    uint16_t min = 0;
    uint16_t max = UINT16_MAX;
    // GROW only field
    // This should be in (0,1] range
    float_t percent = 1;
  };
  struct _layout_sizing {
    _layout_sizing_axis x;
    _layout_sizing_axis y;
  };
  inline _layout_sizing_axis SIZING_FIT() {return {_FIT,0,UINT16_MAX};};
  inline _layout_sizing_axis SIZING_FIT(uint16_t min,uint16_t max) {return {_FIT,min,max};};
  inline _layout_sizing_axis SIZING_FIXED(uint16_t x) {return {_FIXED,x,x};};
  inline _layout_sizing_axis SIZING_GROW(float_t percent = 1) {return {_GROW,0,UINT16_MAX,percent};};
  inline _layout_sizing_axis SIZING_GROW(uint16_t min,uint16_t max,float_t percent = 1) {return {_GROW,min,max,percent};};

  enum _LayoutDirection {LEFT_TO_RIGHT,TOP_TO_BOTTOM};
  enum _LayoutAlignmentX {ALIGN_X_LEFT,ALIGN_X_CENTER,ALIGN_X_RIGHT};
  enum _LayoutAlignmentY {ALIGN_Y_TOP,ALIGN_Y_CENTER,ALIGN_Y_BOTTOM};
  struct layout_config {
    _layout_sizing sizing = {SIZING_FIT(),SIZING_FIT()};
    struct {
      uint16_t left = 0;
      uint16_t right = 0;
      uint16_t top = 0;
      uint16_t bottom = 0;
    } padding;
    uint16_t child_gap = 0;
    _LayoutDirection direction = LEFT_TO_RIGHT;
    struct {
      _LayoutAlignmentX x = ALIGN_X_LEFT;
      _LayoutAlignmentY y = ALIGN_Y_TOP;
    } alignment;
  };
  struct bounding_box {
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
  };
  struct render_command {
    bounding_box box;
    uint32_t id = 0;
    void* data = nullptr;
  };
  typedef std::list<render_command> render_commands;
  enum {CLEARING_ID = 1};
  // Basic building block of CPPReact
  class component {
    public:
    struct {
        component* parent = 0;
        component* next_sibling = 0;
        component* prev_sibling = 0;
        component* child_begin = 0;
        component* child_end = 0;
        uint32_t child_count = 0;
    } k_tree; 
    render_commands layout() {
      bounding_box old_box = box;
      // Init: Reset box
      // Cache the visiting order for later use
      std::list<component*> visiting_order_dfs;
      {
        component* cur = this;
        while (cur) {
          visiting_order_dfs.push_back(cur);
          // Calculate
          cur->on_init_layout();
          // Move to next component
          if (cur->k_tree.child_begin) cur = cur->k_tree.child_begin;
          else { 
            while (cur->k_tree.next_sibling == 0) 
              if (cur->k_tree.parent && cur != this) cur = cur->k_tree.parent; 
              else break;
            cur = cur->k_tree.next_sibling;
          }
          if (cur == this) break;
        }
      }

      // 1st scan: Fit sizing along (reverse BFS)
      // Cache the visiting order for later use
      std::list<component*> visiting_order_rbfs;
      {
        component* cur = this;
        while (cur->k_tree.child_begin) cur = cur->k_tree.child_begin;
        while (cur) {
          visiting_order_rbfs.push_back(cur);
          // Calculate
          cur->on_fit_along();
          // Move to next component
          if (cur == this) break;
          if (cur->k_tree.next_sibling) {
            cur = cur->k_tree.next_sibling;
            while (cur->k_tree.child_begin) cur = cur->k_tree.child_begin;
          } else cur = cur->k_tree.parent;
        }
      }
      // 2nd scan: Grow sizing along (DFS)
      for (auto i : visiting_order_dfs) 
        i->on_child_grow_along();
      // 3rd scan: Fit sizing across (reverse BFS)
      for (auto i : visiting_order_rbfs) 
        i->on_fit_across();
      // 4th scan: Grow sizing across (DFS)
      for (auto i : visiting_order_dfs) 
        i->on_child_grow_across();
      // 5th scan: Positioning & Aligning (DFS)
      for (auto i : visiting_order_dfs) 
        i->on_child_position();
      // 6th scan: Output render commands (DFS)
      render_commands result = {{.box = box,.id = CLEARING_ID}};
      result.splice(result.end(),std::move(this->on_layout()));
      return result;
      
    };
    component(layout_config config,
              std::initializer_list<component*> children = std::initializer_list<component*>()) :
      config(config)
    {
      for (auto i : children) {
        if (i == nullptr)
#ifdef DEBUG
          throw std::runtime_error("Children cannot be nullptr");
#else
          continue;
#endif
        push_back(i);
      }
    }
    virtual ~component() {
      // detach itself from tree
      if (k_tree.next_sibling)
        k_tree.next_sibling->k_tree.prev_sibling = k_tree.prev_sibling;
      if (k_tree.prev_sibling)
        k_tree.prev_sibling->k_tree.next_sibling = k_tree.next_sibling;
      if (k_tree.parent) {
        if (k_tree.parent->k_tree.child_begin == this)
          k_tree.parent->k_tree.child_begin = k_tree.next_sibling;
        if (k_tree.parent->k_tree.child_end == this)
          k_tree.parent->k_tree.child_end = k_tree.prev_sibling;
        k_tree.parent->k_tree.child_count--;
      }
      // clear its children
      while (k_tree.child_begin) {
        component* temp = k_tree.child_begin;
        k_tree.child_begin = k_tree.child_begin->k_tree.next_sibling;
        delete temp;
      }
    }
    
    layout_config config;
    bounding_box box;
  protected:
    virtual void on_init_layout() {
      box = {box.x,box.y,0,0};
    }
    virtual void on_fit_along() {
    // WARN: Fitting DO NOT respect the max value
      if (k_tree.parent) {
        if (k_tree.parent->config.direction == LEFT_TO_RIGHT) on_fit_x(true);
        else on_fit_y(true);
      } else on_fit_x(true);
    }
    virtual void on_fit_across() {
    // WARN: Fitting DO NOT respect the max value
      if (k_tree.parent) {
        if (k_tree.parent->config.direction == LEFT_TO_RIGHT) on_fit_y(false);
        else on_fit_x(false);
      } else on_fit_y(false); 
    }
    virtual void on_child_grow_along() {
      if (config.direction == LEFT_TO_RIGHT) on_grow_x_along();
      else on_grow_y_along();
    }
    virtual void on_child_grow_across() {
      if (config.direction == LEFT_TO_RIGHT) on_grow_y_across();
      else on_grow_x_across();
    }
    virtual void on_child_position() {
      if (config.direction == LEFT_TO_RIGHT) {
        on_pos_x_along();
        on_pos_y_across();
      } else {
        on_pos_y_along();
        on_pos_x_across();
      }
    }
    virtual std::list<render_command> on_layout() {
      std::list<render_command> result;
      component* i = k_tree.child_begin;
      while (i) {
        std::list<render_command> temp = std::move(i->on_layout());
        result.insert(result.end(),temp.begin(),temp.end());
        i = i->k_tree.next_sibling;
      }
      return result;
    }
    private:
    inline void push_back(component* item) {
      item->k_tree.parent = this;
      item->k_tree.prev_sibling = k_tree.child_end;
      if (k_tree.child_begin)
        k_tree.child_end->k_tree.next_sibling = item;
      else k_tree.child_begin = item;
      k_tree.child_end = item;
      k_tree.child_count++;
    }
    inline void on_fit_x(bool along) {
      if (config.sizing.x.mode == _FIXED) box.width = config.sizing.x.min;
      if (config.sizing.x.mode != _FIXED) {
        if (along)
          box.width += config.child_gap * 
            ((k_tree.child_count > 1)?k_tree.child_count - 1:0);
        box.width +=  config.padding.left + config.padding.right;
        if (box.width < config.sizing.x.min) box.width = config.sizing.x.min;
      }
      if (k_tree.parent) {
        if (k_tree.parent->config.sizing.x.mode != _FIXED) {
          if (along) k_tree.parent->box.width += box.width;
          else k_tree.parent->box.width = std::max(k_tree.parent->box.width,box.width);
        }
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
      if (k_tree.parent) {
        if (k_tree.parent->config.sizing.y.mode != _FIXED) {
          if (along) k_tree.parent->box.height += box.height;
          else k_tree.parent->box.height = std::max(k_tree.parent->box.height,box.height);
        }
      }
    }
    struct pos_along_center_component {
      uint16_t length;
      std::list<component*> children;
    };
    inline void on_pos_x_along() {
      // Pre-pass: Initialize & align some components
      std::list<component*> temp_components;
      // {
      //   component* temp = k_tree.child_begin;
      //   if (temp)
      //     while (temp->k_tree.next_sibling) {
      //       if (temp->config.alignment.x == ALIGN_X_RIGHT && 
      //           temp->k_tree.next_sibling->config.alignment.x == ALIGN_X_LEFT) {
      //         component* temp2 = new component({.alignment = {.x = ALIGN_X_CENTER}},{});
      //         temp_components.push_back(temp2);
      //         temp2->k_tree.next_sibling = temp->k_tree.next_sibling;
      //         temp->k_tree.next_sibling->k_tree.prev_sibling = temp2;
      //         temp->k_tree.next_sibling = temp2;
      //         temp2->k_tree.prev_sibling = temp;
      //         temp2->k_tree.parent = temp->k_tree.parent;
      //       }
      //       temp = temp->k_tree.next_sibling;
      //     }
      // }
      component* start = k_tree.child_begin;
      component* end = k_tree.child_end;
      uint16_t left_offset = config.padding.left;
      {
        // Left components
        for (;start;start = start->k_tree.next_sibling) {
          if (start->config.alignment.x != ALIGN_X_LEFT) break;
          if (start->k_tree.prev_sibling)
            start->box.x = start->k_tree.prev_sibling->box.x + 
                         start->k_tree.prev_sibling->box.width + 
                         config.child_gap;
          else start->box.x = box.x + config.padding.left;
          left_offset += start->box.width + config.child_gap;
        }
        // Right components
        for (;end;end = end->k_tree.prev_sibling) {
          if (end->config.alignment.x != ALIGN_X_RIGHT) break;
          if (end->k_tree.next_sibling)
            end->box.x = end->k_tree.next_sibling->box.x - 
                       end->box.width - config.child_gap;
          else end->box.x = box.x + box.width - config.padding.right - end->box.width;
        }
      }
      if (start == nullptr) return;
      if (end == nullptr) return;
      // 1st pass: Center components
      {
        std::list<pos_along_center_component> center_components;
        for (auto i = start;i != end->k_tree.next_sibling;i = i->k_tree.next_sibling) {
          if (i->config.alignment.x == ALIGN_X_CENTER) {
            bool is_connected = false;
            if (i->k_tree.prev_sibling)
              if (i->k_tree.prev_sibling->config.alignment.x == ALIGN_X_CENTER) {
                center_components.back().children.push_back(i);
                center_components.back().length += i->box.width + config.child_gap;
                is_connected = true;
              }
            if (!is_connected)
              center_components.push_back({i->box.width,{i}});
          }
        }
        uint16_t count = 0;
        for (auto& i:center_components) {
          count++;
          uint16_t start_pos = box.width / (center_components.size()+1) * count;
          start_pos = start_pos - i.length / 2;
          for (auto j:i.children) {
            j->box.x = box.x + std::max(start_pos,left_offset);
            start_pos += j->box.width + config.child_gap;
            left_offset += j->box.width + config.child_gap;
          }
        }
      }
      // 2nd pass: Left components
      for (auto i=start;i != end->k_tree.next_sibling;i = i->k_tree.next_sibling) {
        if (i->config.alignment.x == ALIGN_X_LEFT) {
          i->box.x = i->k_tree.prev_sibling->box.x + 
                     i->k_tree.prev_sibling->box.width + 
                     config.child_gap;
        }
      }
      // 3rd pass: Right components
      for (auto i=end;i != start->k_tree.prev_sibling;i = i->k_tree.prev_sibling) {
        if (i->config.alignment.x == ALIGN_X_RIGHT) {
          i->box.x = i->k_tree.next_sibling->box.x - 
                     i->box.width - config.child_gap;
        }
      }
      //for (auto& i:temp_components) delete i;
    }
    inline void on_pos_x_across() {
      for (auto i=k_tree.child_begin;i;i = i->k_tree.next_sibling) {
        switch (i->config.alignment.x) {
          case ALIGN_X_LEFT: 
          i->box.x = box.x + config.padding.left;
          break;
          case ALIGN_X_RIGHT:
          i->box.x = box.x + box.width - config.padding.right - i->box.width;
          break;
          case ALIGN_X_CENTER:
          i->box.x = box.x + box.width / 2 - i->box.width / 2;
          break;
        }
      }
    }
    inline void on_pos_y_along() {
      // Pre-pass: Initialize & align some components
      component* start = k_tree.child_begin;
      component* end = k_tree.child_end;
      uint16_t top_offset = config.padding.top;
      {
        // Left components
        for (;start;start = start->k_tree.next_sibling) {
          if (start->config.alignment.y != ALIGN_Y_TOP) break;
          if (start->k_tree.prev_sibling)
            start->box.y = start->k_tree.prev_sibling->box.y + 
                         start->k_tree.prev_sibling->box.height + 
                         config.child_gap;
          else start->box.y = box.y + config.padding.top;
          top_offset += start->box.height + config.child_gap;
        }
        // Right components
        for (;end;end = end->k_tree.prev_sibling) {
          if (end->config.alignment.y != ALIGN_Y_BOTTOM) break;
            if (end->k_tree.next_sibling)
              end->box.y = end->k_tree.next_sibling->box.y - 
                         end->box.height - config.child_gap;
            else end->box.y = box.y + box.height - config.padding.bottom - end->box.height;

        }
      }
      if (start == nullptr) return;
      if (end == nullptr) return;
      // 1st pass: Center components
      {
        std::list<pos_along_center_component> center_components;
        for (auto i = start;i != end->k_tree.next_sibling;i = i->k_tree.next_sibling) {
          if (i->config.alignment.y == ALIGN_Y_CENTER) {
            bool is_connected = false;
            if (i->k_tree.prev_sibling)
              if (i->k_tree.prev_sibling->config.alignment.y == ALIGN_Y_CENTER) {
                center_components.back().children.push_back(i);
                center_components.back().length += i->box.height + config.child_gap;
                is_connected = true;
              }
            if (!is_connected)
              center_components.push_back({i->box.height,{i}});
          }
        }
        uint16_t count = 0;
        for (auto& i:center_components) {
          count++;
          uint16_t start_pos = box.height / (center_components.size()+1) * count;
          start_pos = start_pos - i.length / 2;
          for (auto j:i.children) {
            j->box.y = box.y + std::max(start_pos, top_offset);
            start_pos += j->box.height + config.child_gap;
            top_offset += j->box.height + config.child_gap;
          }
        }
      }
      // 2nd pass: Left components
      for (auto i=start;i != end->k_tree.next_sibling;i = i->k_tree.next_sibling) {
        if (i->config.alignment.y == ALIGN_Y_TOP) {
          i->box.y = i->k_tree.prev_sibling->box.y + 
                     i->k_tree.prev_sibling->box.height + 
                     config.child_gap;
        }
      }
      // 3rd pass: Right components
      for (auto i=end;i != start->k_tree.prev_sibling;i = i->k_tree.prev_sibling) {
        if (i->config.alignment.y == ALIGN_Y_BOTTOM) {
          i->box.y = i->k_tree.next_sibling->box.y - 
                     i->box.height - config.child_gap;
        }
      }
    }

    inline void on_pos_y_across() {
      for (auto i=k_tree.child_begin;i;i = i->k_tree.next_sibling) {
        switch (i->config.alignment.y) {
          case ALIGN_Y_TOP: 
          i->box.y = box.y + config.padding.top;
          break;
          case ALIGN_Y_BOTTOM:
          i->box.y = box.y + box.height - config.padding.bottom - i->box.height;
          break;
          case ALIGN_Y_CENTER:
          i->box.y = box.y + box.height / 2 - i->box.height / 2;
          break;
        }
      }
    }
    inline void on_grow_x_along() {
      int32_t remaining_width = box.width - config.padding.left - config.padding.right;
      remaining_width -= config.child_gap * (k_tree.child_count-1);
      std::list<component*> grow_components;
      for(component* i=k_tree.child_begin;i;i = i->k_tree.next_sibling) {
        if (i->config.sizing.x.mode == _GROW) {
          grow_components.push_back(i);
        }
        remaining_width -= i->box.width;
      }
      if (grow_components.size() == 0) return;
      while (remaining_width > 0) {
        uint16_t smallest = UINT16_MAX;
        uint16_t second_smallest = UINT16_MAX;
        float_t count = 0;
        uint16_t width_to_add = remaining_width;
        uint16_t width_add_cap = UINT16_MAX;
        for (component* i:grow_components) {
          count += i->config.sizing.x.percent;
          uint16_t real_width = i->box.width / i->config.sizing.x.percent;
          uint16_t real_width_cap = i->config.sizing.x.max / i->config.sizing.x.percent - real_width;
          width_add_cap = std::min(width_add_cap,real_width_cap);
          if (real_width < smallest) {
            second_smallest = smallest;
            smallest = real_width;
          }
          if (real_width > smallest) {
            second_smallest = std::min(second_smallest,real_width);
            width_to_add = second_smallest - smallest;
          }
        }
        width_to_add = std::min(std::min(width_to_add,(uint16_t)(remaining_width / count)),width_add_cap);
        if (width_to_add == 0) break;
        for (auto i = grow_components.begin();i!=grow_components.end();) {
          (*i)->box.width += width_to_add * (*i)->config.sizing.x.percent;
          remaining_width -= width_to_add;
          if ((*i)->box.width == (*i)->config.sizing.x.max) {
            grow_components.erase(i++);
          } else i++;
        }
      }
    }
    inline void on_grow_x_across() {
      for (auto i=k_tree.child_begin;i;i = i->k_tree.next_sibling) if (i->config.sizing.x.mode == _GROW) {
        i->box.width = std::min((uint16_t)((box.width - config.padding.left - config.padding.right) *
                                            i->config.sizing.x.percent),
                                i->config.sizing.x.max);
      }
    }
    inline void on_grow_y_along() {
      int32_t remaining_height = box.height - config.padding.top - config.padding.bottom;
      remaining_height -= config.child_gap * (k_tree.child_count-1);
      std::list<component*> grow_components;
      for(component* i=k_tree.child_begin;i;i = i->k_tree.next_sibling) {
        if (i->config.sizing.y.mode == _GROW) {
          grow_components.push_back(i);
          i->box.height = i->config.sizing.y.min;
        }
        remaining_height -= i->box.height;
      }
      if (grow_components.size() == 0) return;
      while (remaining_height > 0) {
        uint16_t smallest = UINT16_MAX;
        uint16_t second_smallest = UINT16_MAX;
        float_t count = 0;
        uint16_t height_to_add = remaining_height;
        uint16_t height_add_cap = UINT16_MAX;
        for (component* i:grow_components) {
          count += i->config.sizing.y.percent;
          uint16_t real_height = i->box.height / i->config.sizing.y.percent;
          uint16_t real_height_cap = i->config.sizing.y.max / i->config.sizing.y.percent - real_height;
          height_add_cap = std::min(height_add_cap,real_height_cap);
          if (real_height < smallest) {
            second_smallest = smallest;
            smallest = real_height;
          }
          if (real_height > smallest) {
            second_smallest = std::min(second_smallest,real_height);
            height_to_add = second_smallest - smallest;
          }
        }
        height_to_add = std::min(std::min(height_to_add,(uint16_t)(remaining_height / count)),height_add_cap);
        for (auto i = grow_components.begin();i!=grow_components.end();) {
          (*i)->box.height += height_to_add * (*i)->config.sizing.y.percent;
          remaining_height -= height_to_add;
          if ((*i)->box.height == (*i)->config.sizing.y.max) {
            grow_components.erase(i++);
          } else i++;
        }
      }
    }
    inline void on_grow_y_across() {
      for (auto i=k_tree.child_begin;i;i = i->k_tree.next_sibling) if (i->config.sizing.y.mode == _GROW) {
        i->box.height = std::min((uint16_t)((box.height - config.padding.top - config.padding.bottom) * 
                                            i->config.sizing.y.percent),
                                 i->config.sizing.y.max);
      }
    }

  };
};

#endif
