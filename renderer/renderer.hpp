#ifndef _CPPREACT_RENDERER_HPP
#define _CPPREACT_RENDERER_HPP
#include "../component/component.hpp"
#include "../component/rect.hpp"
#include "../component/hover.hpp"
#include <cstdint>
#include <initializer_list>
#include <map>

namespace cppreact {
  class renderer : protected component {
    struct {
      std::map<uint16_t, hover_create_data> x{{0,{nullptr,nullptr}},{UINT16_MAX,{nullptr,nullptr}}};
      std::map<uint16_t, hover_create_data> y{{0,{nullptr,nullptr}},{UINT16_MAX,{nullptr,nullptr}}};
    } hover_event_data;
    void hover_add(bounding_box box,hover_create_data data) {
      hover_event_data.x[box.x]  = data;
      hover_event_data.x[box.x+box.width] = data;
      hover_event_data.y[box.y]  = data;
      hover_event_data.y[box.y+box.height] = data;
    }
    void hover_delete(bounding_box box) {
      hover_event_data.x.erase(box.x);
      hover_event_data.x.erase(box.x+box.width);
      hover_event_data.y.erase(box.y);
      hover_event_data.y.erase(box.y+box.height);
    }
    hover_create_data hover_get(uint16_t x, uint16_t y) {
      auto x1 = hover_event_data.x.lower_bound(x);
      auto x2 = prev(x1);
      auto y1 = hover_event_data.y.lower_bound(y);
      auto y2 = prev(y1);
      if (x1->second == x2->second && y1->second == y2->second && x1->second == y1->second) {
        return x1->second;
      }
      return {nullptr,nullptr};
    }
    public:
    renderer(std::initializer_list<component*> children) : component({},children) {
      
    }
    void run() {
      while (on_loop());
    }
    void (*on_pointer_change)(uint16_t x,uint16_t y) = 0;
    protected:
    void set_size(uint16_t width,uint16_t height) {
      config.sizing = {FIXED(width),FIXED(height)}; 
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
      auto a = layout();
      on_rect({0,0,0,255}, a.clearing_box);
      for (auto& i:a.commands) {
        switch (i.id) {
        case RECT_RENDER_ID:
        on_rect(*reinterpret_cast<color*>(i.data), i.box);
        break;
        case HOVER_CREATE_ID:
        hover_add(i.box, *reinterpret_cast<hover_create_data*>(i.data));
        break;
        case HOVER_DESTROY_ID:
        hover_delete(i.box);
        break;
        }
      }  
    }
    
    virtual bool on_loop() = 0;
    virtual void on_rect(color c,bounding_box box) = 0;
  };
}
#endif
