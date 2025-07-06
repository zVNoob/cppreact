#ifndef _CPP_DEBUG_CALL_HPP
#define _CPP_DEBUG_CALL_HPP

#include <iostream>
#include "renderer.hpp"

namespace  cppreact {
  class debug_call_renderer : public renderer {
    public:
    using renderer::set_size;
    protected:
    void on_rect(bounding_box box, color col, _BlendMode blend) override {
      std::cout << "\x1b[38;2;" << (int)col.r << ";" << (int)col.g << ";" << (int)col.b << "m" << std::flush;
      std::cout << "rect (" << (int)col.r << "," << (int)col.g << "," << (int)col.b << "," << (int)col.a << ") " << "\x1b[0m" <<
      (int)box.x << "," << (int)box.y << ":" << (int)box.width << "*" << (int)box.height << std::endl;
    }
  };
}

#endif
