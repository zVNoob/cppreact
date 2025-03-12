#include "cppreact.hpp"
//#include "renderer/X11_renderer.hpp"
#include "renderer/Win32_renderer.hpp"
#include <iostream>

using namespace std;
using namespace cppreact;


int main() {
  component* a = new wrap(5,{.sizing = {.x = GROW(),.y = GROW()},.padding = {10,10,10,10},.child_gap = 10},{
              new rect({255,50,50},{.sizing = {.x = cppreact::FIXED(50),.y = cppreact::FIXED(50)},.alignment = {.x = XLEFT}}),
              //new rect({50,50,255},{.sizing = {.x = FIXED(100),.y = FIXED(50)},.alignment = {.x = XCENTER}}),
              new rect({50,255,50},{.sizing = {.x = GROW(0.2),.y = cppreact::FIXED(200)},.padding = {10,10,10,10},.child_gap = 10},{
                new rect({255,50,50},{.sizing = {.x = cppreact::FIXED(100),.y = GROW()},.alignment = {.x = XRIGHT}}),
              }),
              new rect({50,255,255},{.sizing = {.x = cppreact::FIXED(200),.y = GROW(0.2)}}),
              new rect({50,0,255},{.sizing = {.x = cppreact::FIXED(100),.y = GROW(0.1)},.alignment = {.x = XCENTER}}),
              new rect({255,50,255},{.padding = {10,10,10,10},.alignment = {.x = XRIGHT}}, {
                new rect({50,50,255},{.sizing = {.x = cppreact::FIXED(50),.y = cppreact::FIXED(100)},.alignment = {.x = XLEFT}}),
              }),
              new rect({255,255,50},{.sizing = {.x = cppreact::FIXED(50),.y = cppreact::FIXED(50)},.alignment = {.x = XRIGHT}}),
              new func([](state_system& s) {
                auto& r = s.get<uint8_t>(200);
                auto& g = s.get<uint8_t>(200);
                auto& b = s.get<uint8_t>(200);
                auto& a = s.get<uint8_t>(255);
                return new hover([&](hover_position p){
                                    r = rand();
                                    g = rand();
                                    b = rand();
                                    cout << p.x << ' ' << p.y << endl;
                                    return true;
                                  },{},{
                  new rect({&r,&g,&b,&a},{.sizing = {cppreact::FIXED(50),cppreact::FIXED(50)}})
                });
                }),
              });
  //X11_renderer renderer(900,900,{a});
  Win32_renderer renderer(900,900,"a",{a});
  renderer.run();

  return 0;
}
