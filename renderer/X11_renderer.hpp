#pragma once

#include "renderer.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/dbe.h>
#include <cstdint>
#include <initializer_list>

namespace cppreact {
  class X11_renderer : public renderer {
  Display* _x11_Display;
  Window _x11_Window;
  int _x11_Screen;
  GC _x11_GC_global;
  int _x11_Width;
  int _x11_Height;
  GC _x11_GC_local;
  XdbeBackBuffer back_buffer;
public:
    X11_renderer(uint16_t width,uint16_t height,std::initializer_list<component*> children):
    renderer(children),_x11_Width(width),_x11_Height(height) {
      _x11_Display = XOpenDisplay(0);
      _x11_Screen = DefaultScreen(_x11_Display);
      _x11_Window = XCreateSimpleWindow(_x11_Display,
        RootWindow(_x11_Display, _x11_Screen),
        0, 0, _x11_Width, _x11_Height, 
        0, 
        WhitePixel(_x11_Display, _x11_Screen), 
        BlackPixel(_x11_Display, _x11_Screen));
      XSelectInput(_x11_Display, _x11_Window, ExposureMask | StructureNotifyMask | PointerMotionMask);
      //Show window
      XMapWindow(_x11_Display, _x11_Window);
      XGCValues XGCValues_temp;
      XGCValues_temp.foreground = _encode_RGB(0,0,0);
      XGCValues_temp.background = _encode_RGB(0,0,0);
    	_x11_GC_global = XCreateGC(_x11_Display, _x11_Window, GCForeground|GCBackground,&XGCValues_temp);
      _x11_GC_local = XCreateGC(_x11_Display, _x11_Window, GCForeground|GCBackground,&XGCValues_temp);
      set_size(_x11_Width, _x11_Height);
      back_buffer = XdbeAllocateBackBufferName(_x11_Display, _x11_Window, XdbeCopied);
      //Commit changes to X11
      XFlush(_x11_Display);
    };
    ~X11_renderer() {
      XFreeGC(_x11_Display, _x11_GC_global);
      XFreeGC(_x11_Display, _x11_GC_local);
      XUnmapWindow(_x11_Display, _x11_Window);
      XDestroyWindow(_x11_Display, _x11_Window);
      XCloseDisplay(_x11_Display);
    }
protected:
    unsigned long _encode_RGB(int r, int g, int b) {
	    return b + (g<<8) + (r<<16);
    }
    void render() {
        renderer::render();
        XdbeSwapInfo swap_info;
        swap_info.swap_window = _x11_Window;
        swap_info.swap_action = XdbeCopied;
        XdbeSwapBuffers(_x11_Display, &swap_info, 1);
        XFlush(_x11_Display);

    }
    bool on_loop() override {
      XEvent e;
      XNextEvent(_x11_Display, &e);
      switch (e.type) {
        case MotionNotify:
        set_pointer(e.xmotion.x, e.xmotion.y);
        render();
        break;
        case ConfigureNotify:
        if (_x11_Width == e.xconfigure.width && _x11_Height == e.xconfigure.height) break;
        _x11_Width = e.xconfigure.width;
        _x11_Height = e.xconfigure.height;
        set_size(_x11_Width, _x11_Height); 
        render();
        break;
        case Expose:
        render();
        break;
      }
      return true;
    }
    void on_rect(color c, bounding_box box) override {
      XSetForeground(_x11_Display, _x11_GC_local, _encode_RGB(c.r, c.g, c.b));
      XSetBackground(_x11_Display, _x11_GC_local, _encode_RGB(c.r, c.g, c.b));
      XFillRectangle(_x11_Display, back_buffer, _x11_GC_local, box.x, box.y, box.width, box.height);
    }
  };
}
