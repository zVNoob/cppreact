#include "cppreact.hpp"
#include "renderer/X11_renderer.hpp"
//#include "renderer/SFML_renderer.hpp"
#include "renderer/debug.hpp"

#include <iostream>
#include <fstream>

using namespace std;
using namespace cppreact;


int main() {
    srand(time(NULL));
    // TEST 1
    // ifstream f("noto.ttf",ios::binary);
    // font font1(f,50);
    // component* a = new wrap(5,{.sizing = {.x = SIZING_GROW(),.y = SIZING_GROW()},.padding = {10,10,10,10},.child_gap = 10},{
    //           new rect({255,50,255},{},{
    //             new text(font1,"hế lô",{255,255,0}),
    //           }),
    //           new rect({50,255,50},{.sizing = {.x = SIZING_GROW(0.2),.y = SIZING_FIXED(200)},.padding = {10,10,10,10},.child_gap = 10},{
    //             new text(font1,"lô con cặc",{127,0,255}),
    //           }),
    //           new rect({50,255,255},{.sizing = {.x = SIZING_FIXED(200),.y = SIZING_GROW(0.2)}}),
    //           new rect({50,0,255},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_GROW(0.1)},.alignment = {.x = ALIGN_X_CENTER}}),
    //           new rect({255,50,255},{.padding = {10,10,10,10},.alignment = {.x = ALIGN_X_LEFT}}, {
    //             new rect({50,50,255},{.sizing = {.x = SIZING_FIXED(100),.y = SIZING_FIXED(100)},.alignment = {.x = ALIGN_X_LEFT}}),
    //           }),
    //           new func([](state_system& state) {
    //             auto r = state.get<uint8_t>(rand() % 256);
    //             auto g = state.get<uint8_t>(rand() % 256);
    //             auto b = state.get<uint8_t>(rand() % 256);
    //             return new rect({r,g,b},{.sizing = {.x = SIZING_FIXED(50),.y = SIZING_FIXED(50)},.alignment = {.x = ALIGN_X_RIGHT}});
    //           }),
    //           });
  //
  // TEST 2
  ifstream f("p.png",ios::binary);
  texture tex1 = texture_from_stream(f);
  texture tex;
  tex.set_size(200,200);
  for (int i = 0; i < 200; i++) {
    for (int j = 0; j < 200; j++) {
      (tex)[i][j] = {255,0,0,127};
    }
  }
  component* a = new image(&tex1,true,{.sizing = {.x = SIZING_GROW(),.y = SIZING_GROW()},.alignment = {.y = ALIGN_Y_CENTER}},{
    new image(&tex,true,{.sizing = {.x = SIZING_GROW(),.y = SIZING_GROW()},.alignment = {.y = ALIGN_Y_CENTER}}),
  });


  //debug_call_renderer r1;
  debug_printer debug;
  X11_renderer r2(1366,768,"a");
  r2.on_render_loop = [&](component*){
    cout << "\x1b[2J\x1b[H" << flush;
    //r1.set_size(r2.size());
    //r1.render(a);
    debug.print(a);
  };
  //r1.set_size(1366,768);
  r2.render_loop(a);


  return 0;
}
