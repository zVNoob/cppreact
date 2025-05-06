#include "cppreact.hpp"
#include "renderer/debug_call.hpp"
#include "renderer/X11_renderer.hpp"

using namespace std;
using namespace cppreact;


int main() {
    component* a = new component({.sizing = {.x = SIZING_GROW(),.y = SIZING_GROW()},.padding = {10,10,10,10},.child_gap = 10},{
              new rect({255,50,50},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(50)}}),
              new rect({50,255,50},{.sizing = {.x = SIZING_GROW(0.2),.y = SIZING_FIXED(200)},.padding = {10,10,10,10},.child_gap = 10},{
                new rect({255,50,50},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_GROW()}}),
              }),
              //new rect({50,255,255},{.sizing = {.x = SIZING_FIXED(200),.y = SIZING_GROW(0.2)}}),
              new rect({50,0,255},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_GROW(0.1)},.alignment = {.x = ALIGN_X_RIGHT}}),
              new rect({255,50,255},{.padding = {10,10,10,10},.alignment = {.x = ALIGN_X_LEFT}}, {
                new rect({50,50,255},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_FIXED(100)},.alignment = {.x = ALIGN_X_LEFT}}),
              }),
              new rect({255,255,50},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(50)},.alignment = {.x = ALIGN_X_LEFT}}),
              });
  debug_call_renderer r1;
  X11_renderer r2(1366,768,"a");
  r2.on_render_loop = [&](component*){
    cout << "\x1b[2J\x1b[H" << flush;
    r1.set_size(r2.size());
    r1.render(a);
  };
  r1.set_size(1366,768);
  r2.render_loop(a);

  //r2.~X11_renderer();


  return 0;
}
