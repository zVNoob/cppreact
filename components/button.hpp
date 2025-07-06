#ifndef _CPPREACT_BUTTON_HPP
#define _CPPREACT_BUTTON_HPP

#include "component.hpp"

namespace cppreact {

  class button : public component {
    public:
    button(layout_config config,std::initializer_list<component*> children = {}) : component(config,children) {}
  
  };
}

#endif
