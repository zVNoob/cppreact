#include "component/absolute.hpp"
#include "component/component.hpp"
#include "component/floating.hpp"
#include "cppreact.hpp"
#include <cstdint>
#include <cstdlib>
#ifdef WIN32
#include "renderer/Win32_renderer.hpp"
#else
#include "renderer/X11_renderer.hpp"
#endif

using namespace std;
using namespace cppreact;


int main() {
  component* a = new wrap(5,{.sizing = {.x = SIZING_GROW(),.y = SIZING_GROW()},.padding = {10,10,10,10},.child_gap = 10},{
              new rect({255,50,50},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(50)}}),
              new rect({50,255,50},{.sizing = {.x = SIZING_GROW(0.2),.y = SIZING_FIXED(200)},.padding = {10,10,10,10},.child_gap = 10},{
                new rect({255,50,50},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_GROW()}}),
              }),
              new rect({50,255,255},{.sizing = {.x = SIZING_FIXED(200),.y = SIZING_GROW(0.2)}}),
              new rect({50,0,255},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_GROW(0.1)},.alignment = {.x = ALIGN_X_CENTER}}),
              new rect({255,50,255},{.padding = {10,10,10,10},.alignment = {.x = ALIGN_X_LEFT}}, {
                new rect({50,50,255},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_FIXED(100)},.alignment = {.x = ALIGN_X_LEFT}}),
                new floating({.alignment = {.x = ALIGN_X_CENTER,.y = ALIGN_Y_BOTTOM}}, {
                  new rect({50,255,255},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(200)}})
                })
              }),
              new rect({255,255,50},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(50)},.alignment = {.x = ALIGN_X_LEFT}}),
              new func([](state_system& s) {
                auto& r = s.get<uint8_t>(200);
                auto& g = s.get<uint8_t>(200);
                auto& b = s.get<uint8_t>(200);
                auto& x = s.get<uint16_t>(100);
                auto& y = s.get<uint16_t>(100);
                return new button([&](button_ev p){
                                    r = rand();
                                    g = rand();
                                    b = rand();
                                    x = rand() % 100 + 100;
                                    y = rand() % 100 + 100;
                                  },{},{
                  new rect({r,g,b},{.sizing = {SIZING_FIXED(x),SIZING_FIXED(y)}})
                });
                }),
                new absolute(200,200,{.direction = LEFT_TO_RIGHT},{
                  new rect({50,50,255},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(100)},.alignment = {.x = ALIGN_X_LEFT}}),
                }),
              });
  // component* a = new func([](state_system& s) {
  //   auto x = s.get<uint16_t>(100);
  //   auto y = s.get<uint16_t>(100);
  //   return new absolute(x,y,{}, {
  //     new button([=](button_ev p){
  //       x = rand() % 100 + 100;
  //       y = rand() % 100 + 100;
  //     },{},{
  //       new rect({200,100,50},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_FIXED(100)}})
  //     })
  //   });
  // });
  #ifdef WIN32
  Win32_renderer renderer(900,900,"a",{a});
  #else
  X11_renderer renderer(1200,900,"a",{a});
  #endif
  renderer.run();

  return 0;
}
