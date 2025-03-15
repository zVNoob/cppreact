#ifndef _CPPREACT_RENDERER_HPP
#define _CPPREACT_RENDERER_HPP
#include "../component/component.hpp"
#include "../component/rect.hpp"
#include "../component/button.hpp"
#include "../component/func.hpp"
#include <cstdint>
#include <list>
#include <map>
#include <set>

namespace cppreact {
  class renderer : protected component {
  // Hover handling
    typedef std::list<button_create_data*> pointer_ev_list;
    typedef std::map<uint16_t,pointer_ev_list> pointer_ev_y1;
    typedef std::map<uint16_t,pointer_ev_y1> pointer_ev_x1;
    typedef std::map<uint16_t,pointer_ev_x1> pointer_ev_y2;
    typedef std::map<uint16_t,pointer_ev_y2> pointer_ev_x2;
    pointer_ev_x2 button_map;
    void button_add(bounding_box box,button_create_data* data) {
      auto& temp = button_map[box.x + box.width][box.y + box.height][box.x][box.y];
      temp.push_back(data);
      data->ref = &*(--temp.end());
    }
    void pointer_ev_run(uint16_t x,uint16_t y,button_type type,int16_t state) {
      for (auto ix2 = button_map.begin();ix2 != button_map.end();) {
        auto& x2 = ix2->second;
        for (auto iy2 = x2.begin();iy2 != x2.end();) {
          auto& y2 = iy2->second;
          for (auto ix1 = y2.begin();ix1 != y2.end();) {
            auto& x1 = ix1->second;
            for (auto iy1 = x1.begin();iy1 != x1.end();) {
              auto& y1 = iy1->second;
              for (auto i = y1.begin();i != y1.end();) {
                if ((*i) == 0) y1.erase(i++);
                else i++;
              }
              if (y1.empty()) x1.erase(iy1++);
              else iy1++;
            }
            if (x1.empty()) y2.erase(ix1++);
            else ix1++;
          }
          if (y2.empty()) x2.erase(iy2++);
          else iy2++;
        }
        if (x2.empty()) button_map.erase(ix2++);
        else ix2++;
      }
      auto x2_iter = button_map.lower_bound(x);
      if (x2_iter == button_map.end()) return;
        
      auto y2_iter = x2_iter->second.lower_bound(y);
      if (y2_iter == x2_iter->second.end()) return;
          
      auto x1_iter = --y2_iter->second.lower_bound(x);
            
      auto y1_iter = --x1_iter->second.lower_bound(y);
              
      for (auto i=y1_iter->second.begin();i!=y1_iter->second.end();) {
        component* c = (*i)->object;
        if (x >= c->box.x && y >= c->box.y) {
          button_ev temp = {static_cast<uint16_t>(x - c->box.x),
                            static_cast<uint16_t>(y - c->box.y),
                            box.width,
                            box.height,
                            state,
                            type,
          };
          (*i)->callback(temp);
        }
        i++;
      }
    }
    // Render optimization & component change handling
    std::set<component*> change_element;
    color clear_color;
    std::list<dynamic_register_data*> update_data;
    void dynamic_handle() {
      for (auto i = update_data.begin();i != update_data.end();) {
        auto t = *i;
        if (t) {
          if (t->changed) change_element.insert(t->obj);
          t->changed = false;
          i++;
        } else {
          update_data.erase(i++);
        }
      }
    }
    void dynamic_add(dynamic_register_data* data) {
      update_data.push_back(data);
      auto i = --update_data.end();
      (*i)->ref = &*i;
    }
    render_commands reduced_layout() {
      for (auto i = change_element.begin();i != change_element.end();) {
        component* temp = *i;
        while (temp->k_tree.parent) {
          if (temp->k_tree.parent->config.sizing.x.mode == _FIT) temp = temp->k_tree.parent;
          else if (temp->k_tree.parent->config.sizing.y.mode == _FIT) temp = temp->k_tree.parent;
          else break;
          auto t = change_element.find(temp);
          if (t != change_element.end())
            if (t != i) {
              change_element.erase(i++);
              break;
            }
        }
        i++;
      }
      render_commands result;
      render_commands a;
      for (auto& i:change_element) {
        a = i->layout();
        on_clear(a.clearing_box);
        result.commands.insert(result.commands.end(),a.commands.begin(),a.commands.end());
      }
      return result;
    }
  // Close handling
    bool running = true;
    void (*on_close)(bool&) = 0;
    public:
  // Public API
    renderer(std::initializer_list<component*> children,color clear_color = {0,0,0,255}) :
      component({},children),clear_color(clear_color) {
    }
    virtual ~renderer() { 
      for (auto& x2:button_map)
        for (auto& y2:x2.second)
          for (auto& x1:y2.second)
            for (auto& y1:x1.second)
              for (auto& i:y1.second)
                if (i) i->ref = 0;
      for (auto& i:update_data) if (i) if (i->ref) i->ref = 0;
    } 
    void run() {
      while (running) on_loop();
    }
    void run_once() { on_loop(); }
    protected:
  // Protected API
    void set_size(uint16_t width,uint16_t height) {
      config.sizing = {SIZING_FIXED(width),SIZING_FIXED(height)};
      change_element.insert(this);
    }
    void close() {
      if (on_close) on_close(running);
      else running = false;
    }
    inline void set_pointer_move(uint16_t x,uint16_t y) {
      pointer_ev_run(x,y,EV_BUTTON_NONE,EV_BUTTON_MOVE);
    }
    inline void set_pointer_press(uint16_t x,uint16_t y,button_type type,bool is_down) {
      pointer_ev_run(x, y, type, (is_down)?EV_BUTTON_DOWN:EV_BUTTON_UP);
    }
    inline void set_pointer_scroll(uint16_t x,uint16_t y,bool is_up,uint16_t delta) {
      pointer_ev_run(x,y,(is_up)?EV_BUTTON_WHEEL_UP:EV_BUTTON_WHEEL_DOWN,delta);
    }
    void render() {
      dynamic_handle();
      render_commands a = reduced_layout();
      change_element.clear();
      for (auto& i:a.commands) {
        switch (i.id) {
        case RECT_RENDER_ID:
        on_rect(*reinterpret_cast<color*>(i.data), i.box);
        break;
        case BUTTON_CREATE_ID:
        button_add(i.box, reinterpret_cast<button_create_data*>(i.data));
        break;
        case DYNAMIC_REGISTER_ID:
        dynamic_add(reinterpret_cast<dynamic_register_data*>(i.data));
        break;
        }
      }  
    }
    
    virtual void on_loop() = 0;
    virtual void on_rect(color c,bounding_box box) = 0;
    virtual void on_clear(bounding_box box) {
      on_rect(clear_color,box);
    }
  };
}
#endif
