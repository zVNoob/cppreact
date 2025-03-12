#ifndef _CPPREACT_RENDERER_HPP
#define _CPPREACT_RENDERER_HPP
#include "../component/component.hpp"
#include "../component/rect.hpp"
#include "../component/hover.hpp"
#include "../component/func.hpp"
#include <cstdint>
#include <list>
#include <map>
#include <set>

namespace cppreact {
  class renderer : protected component {
  // Hover handling
    struct hover_data {
      hover_create_data* data;
      uint16_t x1,x2,y1,y2;
    };
    struct {
      std::list<hover_data> data;
      std::map<uint16_t,std::list<hover_data>::iterator> x;
      std::map<uint16_t,std::list<hover_data>::iterator> y;
    } hover_event_data;
    std::set<component*> change_element;
    color clear_color;
    void hover_add(bounding_box box,hover_create_data* data) {
      hover_event_data.data.push_back({
        data,
        box.x,
        uint16_t(box.x + box.width),
        box.y,
        uint16_t(box.y + box.height)});
      hover_event_data.x[box.x+box.width] = --hover_event_data.data.end();
      hover_event_data.y[box.y+box.height] = --hover_event_data.data.end();
      data->ref = &(--hover_event_data.data.end())->data;
    }
    hover_create_data hover_get(uint16_t x,uint16_t y) {
      auto xtmp = hover_event_data.x.lower_bound(x);
      if (xtmp == hover_event_data.x.end()) return {nullptr,nullptr,nullptr};
      auto xit = xtmp->second;
      auto ytmp = hover_event_data.y.lower_bound(y);
      if (ytmp == hover_event_data.y.end()) return {nullptr,nullptr,nullptr};
      auto yit = ytmp->second;
      if (xit->x1 <= x && yit->y1 <= y && xit->x2 >= x && yit->y2 >= y) {
        if (xit->data == yit->data) {
          if (xit->data) return *xit->data;
          else {
            hover_event_data.x.erase(xit->x2);
            hover_event_data.y.erase(xit->y2);
            hover_event_data.data.erase(xit);
          }
        }  
      }
      return {nullptr,nullptr,nullptr};
    }
    // Render optimization & component change handling
    std::list<update_register_data*> update_data;
    void update_handle() {
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
    void update_add(update_register_data* data) {
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

    public:
  // Public API
    renderer(std::initializer_list<component*> children,color clear_color = {0,0,0,255}) :
      component({},children),clear_color(clear_color) {
    }
    virtual ~renderer() { 
      for (auto& i:hover_event_data.data) if (i.data)
        if (i.data->ref) i.data->ref = 0;
      for (auto& i:update_data) if (i) if (i->ref) i->ref = 0;
    } 
    void run() {
      while (on_loop());
    }
    void (*on_pointer_change)(uint16_t x,uint16_t y) = 0;

    protected:
  // Protected API
    void set_size(uint16_t width,uint16_t height) {
      config.sizing = {SIZING_FIXED(width),SIZING_FIXED(height)};
      change_element.insert(this);
    }
    void set_pointer(uint16_t x,uint16_t y) {
      if (on_pointer_change) on_pointer_change(x,y);
      auto hover = hover_get(x,y);
      if (hover.object) {
        hover_position temp = {
          static_cast<uint16_t>(x - hover.object->box.x),
          static_cast<uint16_t>(y - hover.object->box.y),
          hover.object->box.width,
          hover.object->box.height,
        };
        hover.callback(temp);
      }
    }
    void render() {
      update_handle();
      render_commands a = reduced_layout();
      change_element.clear();
      for (auto& i:a.commands) {
        switch (i.id) {
        case RECT_RENDER_ID:
        on_rect(*reinterpret_cast<color*>(i.data), i.box);
        break;
        case HOVER_CREATE_ID:
        hover_add(i.box, reinterpret_cast<hover_create_data*>(i.data));
        break;
        case UPDATE_REGISTER_ID:
        update_add(reinterpret_cast<update_register_data*>(i.data));
        break;
        }
      }  
    }
    
    virtual bool on_loop() = 0;
    virtual void on_rect(color c,bounding_box box) = 0;
    virtual void on_clear(bounding_box box) {
      on_rect(clear_color,box);
    }
  };
}
#endif
