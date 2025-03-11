#include "cppreact.hpp"
#include "renderer/X11_renderer.hpp"
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace cppreact;


int main() {
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
              new func([](state_system& s) {
                auto& r = s.get<uint8_t>(200);
                auto& g = s.get<uint8_t>(200);
                auto& b = s.get<uint8_t>(200);
                return new hover([&](hover_position p){
                                    r = rand();
                                    g = rand();
                                    b = rand();
                                    cout << p.x <<  ' ' << p.y << ' ' << p.width << ' ' << p.height << endl;
                                    return true;
                                  },{},{
                  new rect({r,g,b},{.sizing = {FIXED(50),FIXED(50)}})
                });
                }),
              });
  X11_renderer renderer(900,900,{a});
  renderer.run();

  return 0;
}
