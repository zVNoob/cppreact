#ifndef _CPPREACT_HOVER_HPP
#define _CPPREACT_HOVER_HPP
#include "component.hpp"
#include <cstdint>
#include <functional>

namespace cppreact {
  enum {HOVER_DESTROY_ID = 10000,HOVER_CREATE_ID};
  struct hover_position {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
  };
  struct hover_create_data {
  std::function<void(hover_position)> callback;
    component* object;
    hover_create_data** ref;
    bool operator==(hover_create_data& other) { 
      return object == other.object;
    }
  };
  
  class hover : public component { 
    bounding_box old_box = {0,0,0,0};
    hover_create_data data;
    public:
    hover(std::function<void(hover_position)> callback,
      layout_config config,std::initializer_list<component*> children = {}) :
      component(config,children),data({callback,this}) {}
    ~hover() {
      if (data.ref)
        *(data.ref) = nullptr;
    } 
    protected:
    
    std::list<render_command> on_layout() override {
      std::list<render_command> result;
      if (old_box.width && old_box.height) {
        if (old_box.width != box.width || old_box.height != box.height) {
          result.push_back({.box = old_box,.id = HOVER_DESTROY_ID});
          result.push_back({.box = box,.id = HOVER_CREATE_ID,.data = &data});
        }
      } else {
        result.push_back({.box = box,.id = HOVER_CREATE_ID,.data = &data});
      }
      old_box = box;
      return result;
    }
  };
}

#endif

