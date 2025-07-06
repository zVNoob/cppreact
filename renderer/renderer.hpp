#ifndef _CPPREACT_RENDERER_HPP
#define _CPPREACT_RENDERER_HPP

#include "../cppreact.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <tuple>
#include <utility>

namespace cppreact {
  
class renderer {
  component component_holder;
  bool running = true;
  public:
  renderer() : component_holder({.sizing = {SIZING_FIXED(0),SIZING_FIXED(0)}}) {}
  virtual ~renderer() {
    component_holder.detach(component_holder.first_child());
    for(auto& i: texture_cache) {
      *std::get<1>(i.second) = 0;
    }
  }
  void render(component* root) {
    component_holder.push_back(root);
    component_holder.layout();
    component* temp = component_holder.first_child();
    component_holder.detach(component_holder.first_child());
    while (temp) {
      bounding_box box = temp->box;
      if (dynamic_cast<rect*>(temp)) {
        on_rect(box,static_cast<rect*>(temp)->col,static_cast<rect*>(temp)->blend);
      }
      if (dynamic_cast<image*>(temp)) {
        on_image(box,static_cast<image*>(temp)->tex);
      }
      if (dynamic_cast<text*>(temp)) {
        on_text(box,static_cast<text*>(temp)->render_output,static_cast<text*>(temp)->col);
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
  // Internal rendering hook
  // Image caching
  std::map<texture*,std::tuple<bool,bool**,std::any>> texture_cache;
  void on_image(bounding_box box,texture* tex) {
    if (tex->size().first == 0 || tex->size().second == 0) return;
    if (texture_cache.find(tex) == texture_cache.end()) {
      texture_cache[tex] = {true,0,on_new_image(tex)};
      std::get<1>(texture_cache[tex]) = tex->hold(&std::get<0>(texture_cache[tex]));
    } else if (!std::get<0>(texture_cache[tex])) {
      texture_cache[tex] = {true,0,on_new_image(tex)};
      std::get<1>(texture_cache[tex]) = tex->hold(&std::get<0>(texture_cache[tex]));
    }
    on_image(box,std::get<2>(texture_cache[tex]));
  }
  // Text rendering
  void on_text(bounding_box box,std::vector<text_render_data>& data,color col) {
    for (auto& i: data) if (i.tex->size().first != 0 && i.tex->size().second != 0) {
      if (texture_cache.find(i.tex) == texture_cache.end()) {
        texture_cache[i.tex] = {true,0,on_new_image(i.tex)};
        std::get<1>(texture_cache[i.tex]) = i.tex->hold(&std::get<0>(texture_cache[i.tex]));
      } else if (!std::get<0>(texture_cache[i.tex])) {
        texture_cache[i.tex] = {true,0,on_new_image(i.tex)};
        std::get<1>(texture_cache[i.tex]) = i.tex->hold(&std::get<0>(texture_cache[i.tex]));
      }
      on_each_text({static_cast<uint16_t>(box.x + i.x),static_cast<uint16_t>(box.y + i.y),i.tex->size().first,i.tex->size().second},std::get<2>(texture_cache[i.tex]),col);
    }
  }
  // External rendering hook
  protected:
  virtual std::any on_new_image(texture* tex) = 0;
  virtual void on_image(bounding_box box,std::any& image) = 0;
  
  virtual void on_rect(bounding_box box,color col,_BlendMode blend) = 0;
  
  virtual void on_begin_loop(component* root) {};
  virtual void on_loop(component* root) {render(root);};
  
  virtual void on_each_text(bounding_box box,std::any& img,color col) = 0;
  // User hook
  public:
  std::function<bool()> on_exit;
  std::function<void(component*)> on_render_loop;
  };
}

#endif
