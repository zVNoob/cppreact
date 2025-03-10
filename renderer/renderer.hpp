#ifndef _CPPREACT_RENDERER_HPP
#define _CPPREACT_RENDERER_HPP
#include "../component/component.hpp"
#include "../component/rect.hpp"
#include <cstdint>
#include <initializer_list>

namespace cppreact {
  class renderer : protected component {
    public:
    renderer(std::initializer_list<component*> children) : component({},children) {
      
    }
    void run() {
      while (on_loop());
    }
    protected:
    void set_size(uint16_t width,uint16_t height) {
      config.sizing = {FIXED(width),FIXED(height)}; 
    }
    void render() {
      auto a = layout();
      on_rect({0,0,0,255}, a.clearing_box);
      for (auto& i:a.commands) {
        switch (i.id) {
        case RECT_RENDER_ID:
        on_rect(*reinterpret_cast<color*>(i.data), i.box);
        break;
        }
      }  
    }
    
    virtual bool on_loop() = 0;
    virtual void on_rect(color c,bounding_box box) = 0;
    
  };
}
#endif
