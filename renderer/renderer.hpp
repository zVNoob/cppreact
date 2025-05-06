#ifndef _CPPREACT_RENDERER_HPP
#define _CPPREACT_RENDERER_HPP

#include "../cppreact.hpp"
#include <cstdint>
#include <functional>
#include <utility>

namespace cppreact {
  
class renderer {
  component component_holder;
  bool running = true;
  public:
  renderer() : component_holder({.sizing = {SIZING_FIXED(0),SIZING_FIXED(0)}}) {}
  virtual ~renderer() {
    component_holder.detach(component_holder.first_child());
  }
  void render(component* root) {
    component_holder.push_back(root);
    component_holder.layout();
    component* temp = component_holder.first_child();
    component_holder.detach(component_holder.first_child());
    while (temp) {
      bounding_box box = temp->get_box();
      if (dynamic_cast<rect*>(temp)) {
        on_rect(box,static_cast<rect*>(temp)->col,static_cast<rect*>(temp)->blend);
      }
      temp = temp->next_preorder();
    }
  }
  void render_loop(component* root) {
    on_begin_loop(root);
    while (running) {
      on_loop(root);
      if (on_render_loop) on_render_loop(root);
    }
  }
  std::pair<uint16_t, uint16_t> size() {
    auto temp = component_holder.get_config().sizing;
    return {temp.x.min,temp.y.min};
  }
  // Inheritance API
  protected:
  void set_size(std::pair<uint16_t, uint16_t> size) {
    set_size(size.first,size.second);
  }
  void set_size(uint16_t width,uint16_t height) {
    component* temp = component_holder.detach(component_holder.first_child());
    component_holder = component({.sizing = {SIZING_FIXED(width),SIZING_FIXED(height)}});
    component_holder.push_back(temp);
  }
  void exit() {
    if (on_exit) running = on_exit();
    else running = false;
  }
  // Rendering hook
  protected:
  virtual void on_rect(bounding_box box,color col,_BlendMode blend) = 0;
  virtual void on_begin_loop(component* root) {};
  virtual void on_loop(component* root) {render(root);};

  
  // User hook
  public:
  std::function<bool()> on_exit;
  std::function<void(component*)> on_render_loop;
  };
}

#endif
