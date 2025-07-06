#ifndef _CPPREACT_IMAGE_HPP
#define _CPPREACT_IMAGE_HPP

#include "component.hpp"
#include "rect.hpp"

#include <cstdint>
#include <list>
#include <utility>
#include <vector>



#ifndef _CPPREACT_NO_STBI
#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"
#endif

namespace cppreact {
  class texture {
    std::vector<std::vector<color>> data;
    std::list<bool*> holder;
    public:
    std::vector<color>& operator[](size_t index) {
      return data[index];
    }
    std::pair<uint16_t, uint16_t> size() {
      int h = data.size();
      if (h == 0) return {0,0};
      int w = data[0].size();
      return {w,h};
    }
    void set_size(uint16_t w, uint16_t h) {
      data.resize(h);
      for (size_t i = 0; i < h; i++) {
        data[i].resize(w);
      }
    }
    void set_size(std::pair<uint16_t, uint16_t> size) {
      set_size(size.first, size.second);
    }
    bool** hold(bool* inp) {
      holder.push_back(inp);
      return &*(--holder.end());
    }
    ~texture() {
      for (bool* i : holder) {
        if (i)
          *i = false;
      }
    }
  };
#ifndef _CPPREACT_NO_STBI
  inline texture texture_from_stream(std::istream& stream) {
    std::string data;
    stream.seekg(0, std::ios::end);
    data.resize(stream.tellg());
    stream.seekg(0, std::ios::beg);
    stream.read(&data[0], data.size());
    int w, h, channels;
    unsigned char* img = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), &w, &h, &channels, 0);
    texture result;
    result.set_size(w, h);
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) {
        if (channels == 4)
          result[i][j] = {img[(i * w + j) * channels+0], img[(i * w + j) * channels + 1], img[(i * w + j) * channels + 2], img[(i * w + j) * channels + 3]};
        else
          result[i][j] = {img[(i * w + j) * channels+0], img[(i * w + j) * channels + 1], img[(i * w + j) * channels + 2], 255};
      }
    }
    stbi_image_free(img);
    return result;
  }
#endif
  class image : public component {
    bool keep_aspect;
    layout_config orig_config;
    public:
    texture* tex;
    image(texture* tex,bool keep_aspect,layout_config config,std::initializer_list<component*> children = {}) : 
      tex(tex),keep_aspect(keep_aspect),component(config,children) {
      orig_config = config;
    }
    protected:
    void on_init_layout() override {
      component::on_init_layout();
      config = orig_config;
      if (config.sizing.x.mode == _FIT) config.sizing.x.min = tex->size().first;
      if (config.sizing.y.mode == _FIT) config.sizing.y.min = tex->size().second;
    }
    void on_fit_along() override {
      if (keep_aspect) {
        if (tree.parent) {
          if (tree.parent->get_config().direction == LEFT_TO_RIGHT)
            on_fit_kasp_x_along();
          else on_fit_kasp_y_along();
        } else on_fit_kasp_x_along();
      }
      component::on_fit_along();
    }
    void on_child_grow_along() override {
      if (keep_aspect) {
        if (tree.parent) {
          if (tree.parent->get_config().direction == LEFT_TO_RIGHT)
            on_grow_kasp_x_along();
          else
            on_grow_kasp_y_along();
        } else on_grow_kasp_x_along();
      }
      component::on_child_grow_along();
    }
    private:

    inline void on_fit_kasp_x_along() {
      if (config.sizing.x.mode != _FIXED && config.sizing.y.mode == _FIXED) {
        config.sizing.x.min = config.sizing.y.min * tex->size().first / tex->size().second;
        config.sizing.x.mode = _FIXED;
      }
    }
    inline void on_grow_kasp_y_along() {
      config.sizing.x.mode = _FIXED;
      config.sizing.x.min = box.height * tex->size().first / tex->size().second;
    }
    inline void on_grow_kasp_x_along() {
      config.sizing.y.mode = _FIXED;
      config.sizing.y.min = box.width * tex->size().second / tex->size().first;
    }
    inline void on_fit_kasp_y_along() {
      if (config.sizing.y.mode != _FIXED && config.sizing.x.mode == _FIXED) {
        config.sizing.y.min = config.sizing.x.min * tex->size().second / tex->size().first;
        config.sizing.y.mode = _FIXED;
      }
    }       
  };
}

#endif
