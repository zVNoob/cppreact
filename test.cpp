#include "component/component.hpp"
#include "cppreact.hpp"
#include "renderer/X11_renderer.hpp"
#include <functional>
#include "component/function.hpp"

using namespace std;
using namespace cppreact;


int main() {
  std::function<component*()> b;
  { 
    int c = 0;
    b = [c](){return new component({.sizing = {.x = FIXED(c)}});};
  }
  auto d = cppreact::function(b);
  component* a = new wrap(5,{.sizing = {.x = GROW(),.y = GROW()},.padding = {10,10,10,10},.child_gap = 10},{
              new rect({255,50,50},{.sizing = {.x = FIXED(50),.y = FIXED(50)},.alignment = {.x = XLEFT}}),
              //new rect({50,50,255},{.sizing = {.x = FIXED(100),.y = FIXED(50)},.alignment = {.x = XCENTER}}),
              new rect({50,255,50},{.sizing = {.x = GROW(0.2),.y = FIXED(200)},.padding = {10,10,10,10},.child_gap = 10},{
                new rect({255,50,50},{.sizing = {.x = FIXED(100),.y = GROW()},.alignment = {.x = XRIGHT}}),
              }),
              new rect({50,255,255},{.sizing = {.x = FIXED(200),.y = GROW(0.2)}}),
              new rect({50,0,255},{.sizing = {.x = FIXED(100),.y = GROW(0.1)},.alignment = {.x = XCENTER}}),
              new rect({255,50,255},{.padding = {10,10,10,10},.alignment = {.x = XRIGHT}}, {
                new rect({50,50,255},{.sizing = {.x = FIXED(50),.y = FIXED(100)},.alignment = {.x = XLEFT}}),
              }),
              new rect({255,255,50},{.sizing = {.x = FIXED(50),.y = FIXED(50)},.alignment = {.x = XRIGHT}}),
              });
  X11_renderer renderer(500,900,{a});
  renderer.run();
  return 0;
}
