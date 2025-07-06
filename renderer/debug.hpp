#pragma once

#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include "../cppreact.hpp"

namespace cppreact {
  class debug_printer {
    void _print_box(bounding_box box) {
      std::cout << " " << box.x << "," << box.y << ":" << box.width << "*" << box.height;
    }
    void _component(component* current,std::string name = "component",std::string arg = "") {
      if (current->get_config().id) {
        std::cout << current->get_config().id << " (" << name << ") ";
      }
      else {
        std::cout << name << " ";
      }
      if (arg != "") {
        std::cout << arg << " ";
      }
      _print_box(current->box);
    }
    void _rect(rect* current) {
      std::cout << "\x1b[38;2;" << (int)current->col.r << ";" << (int)current->col.g << ";" << (int)current->col.b << "m";
      if (current->get_config().id) {
        std::cout << current->get_config().id << " (" << "rect" << ")";
      }
      else {
        std::cout << "rect" << "";
      }
      std::cout << " (" << (int)current->col.r << "," << 
        (int)current->col.g << "," << (int)current->col.b << "," << (int)current->col.a;
      switch (current->blend) {
        case BLEND_ADD:
          std::cout << " ADD";
          break;
        case BLEND_MULTIPLY:
          std::cout << " MULTIPLY";
          break;
        case BLEND_NONE:
          break;
      }
      std::cout << ") ";
      std::cout << "\x1b[0m";
      _print_box(current->box);
    }
    void _wrap(wrap* current) {
      _component(current,"wrap");
    }
    void _func(func* current) {
      _component(current,"func");
    }
    void _text(text* current) {
      std::cout << "\x1b[38;2;" << (int)current->col.r << ";" << (int)current->col.g << ";" << (int)current->col.b << "m";
      if (current->get_config().id) {
        std::cout << current->get_config().id << " (" << "text" << ")";
      }
      else {
        std::cout << "text" << "";
      }
      std::cout << " (" << current->t;

      std::cout << ") ";
      std::cout << "\x1b[0m";
      _print_box(current->box);
    }
    void _image(image* current) {
      if (current->get_config().id) {
        std::cout << current->get_config().id << " (" << "image" << ")";
      }
      else {
        std::cout << "image" << "";
      }
      std::cout << " (" << current->tex << ':' << current->tex->size().first << "*" << current->tex->size().second;

      std::cout << ") ";
      std::cout << "\x1b[0m";
      _print_box(current->box);

    }
    void _dispatch(component* current) {
      if (dynamic_cast<image*>(current)) {
        _image(static_cast<image*>(current));
      } else
      if (dynamic_cast<text*>(current)) {
        _text(static_cast<text*>(current));
      } else
      if (dynamic_cast<func*>(current)) {
        _func(static_cast<func*>(current));
      } else
      if (dynamic_cast<wrap*>(current)) {
        _wrap(static_cast<wrap*>(current));
      } else
      if (dynamic_cast<rect*>(current)) {
        _rect(static_cast<rect*>(current));
      } else 
      if (dynamic_cast<component*>(current)) {
        _component(current);
      } 
    }
    void _render(component* current,int depth = 0) {
      for (int i = 0;i < depth;i++) std::cout << "  ";
      _dispatch(current);
      std::cout << std::endl;
      for (component* i = current->first_child();i;i = i->next_sibling()) {
        _render(i,depth + 1);
      }
    }
    public:
    void print(component* root) {
      _render(root);
    }
  
  };
}
