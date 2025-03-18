#ifndef _CPPREACT_BUTTON_HPP
#define _CPPREACT_BUTTON_HPP
#include "component.hpp"
#include <cstdint>
#include <functional>

namespace cppreact {
  enum {BUTTON_CREATE_ID=1000};
  enum button_state {
    EV_BUTTON_DOWN = -2,
    EV_BUTTON_UP = -1,
    EV_BUTTON_MOVE = 0,
  };
  enum button_type {
    EV_BUTTON_NONE,
    EV_BUTTON_LEFT,
    EV_BUTTON_RIGHT,
    EV_BUTTON_MIDDLE,
    EV_BUTTON_WHEEL_UP,
    EV_BUTTON_WHEEL_DOWN,
  };
  struct button_ev {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    int16_t state;
    button_type type;
  };
  struct button_create_data {
  std::function<void(button_ev)> callback;
    component* object;
    button_create_data** ref;
    bool operator==(button_create_data& other) { 
      return object == other.object;
    }
  };
  // Pointer-reactive component with callback
  class button : public component { 
    bounding_box old_box = {0,0,0,0};
    button_create_data data;
    public:
    button(std::function<void(button_ev)> callback,
      layout_config config,std::initializer_list<component*> children = {}) :
      component(config,children),data({callback,this}) {}
    ~button() {
      if (data.ref)
        *(data.ref) = nullptr;
    } 
    protected:
    
    std::list<render_command> on_layout() override {
      std::list<render_command> result;
      if (old_box.width && old_box.height) {
        if (old_box.width != box.width || old_box.height != box.height) {
          result.push_back({.box = box,.id = BUTTON_CREATE_ID,.data = &data});
        }
      } else {
        result.push_back({.box = box,.id = BUTTON_CREATE_ID,.data = &data});
      }
      old_box = box;
      result.splice(result.end(),component::on_layout());
      return result;
    }
  };
}

#endif

