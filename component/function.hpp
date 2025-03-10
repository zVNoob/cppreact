#pragma once

#include "component.hpp"
#include <functional>


namespace cppreact {
  class function : public component {
    std::function<component*()> func;
    public:
    function(std::function<component*()> f) : 
      func(f),component({}) {
    }
    protected:
    
  };
}
