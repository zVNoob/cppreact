#include "cppreact.hpp"
#ifdef WIN32
#include "renderer/Win32_renderer.hpp"
#else
#include "renderer/X11_renderer.hpp"
#endif
#include <iostream>

using namespace std;
using namespace cppreact;


int main() {
  component* a = new wrap(5,{.sizing = {.x = SIZING_GROW(),.y = SIZING_GROW()},.padding = {10,10,10,10},.child_gap = 10},{
              new rect({255,50,50},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(50)},.alignment = {.x = XLEFT}}),
              new rect({50,255,50},{.sizing = {.x = SIZING_GROW(0.2),.y = SIZING_FIXED(200)},.padding = {10,10,10,10},.child_gap = 10},{
                new rect({255,50,50},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_GROW()},.alignment = {.x = XRIGHT}}),
              }),
              new rect({50,255,255},{.sizing = {.x = SIZING_FIXED(200),.y = SIZING_GROW(0.2)}}),
              new rect({50,0,255},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_GROW(0.1)},.alignment = {.x = XCENTER}}),
              new rect({255,50,255},{.padding = {10,10,10,10},.alignment = {.x = XRIGHT}}, {
                new rect({50,50,255},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(100)},.alignment = {.x = XLEFT}}),
              }),
              new rect({255,255,50},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(50)},.alignment = {.x = XRIGHT}}),
              new func([](state_system& s) {
                auto r = s.get<uint8_t>(200);
                auto g = s.get<uint8_t>(200);
                auto b = s.get<uint8_t>(200);
                return new hover([=](hover_position p){
                                    r = rand();
                                    g = rand();
                                    b = rand();
                                    cout << p.x << ' ' << p.y << endl;
                                  },{},{
                  new rect({r,g,b},{.sizing = {SIZING_FIXED(50),SIZING_FIXED(50)}})
                });
                }),
              nullptr,
              });
  #ifdef WIN32
  Win32_renderer renderer(900,900,"a",{a});
  #else
  X11_renderer renderer(900,900,"a",{a});
  #endif
  renderer.run();

  return 0;
}
