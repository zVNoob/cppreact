#ifndef _CPPREACT_X11_RENDERER_HPP
#define _CPPREACT_X11_RENDERER_HPP
#include "renderer.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/dbe.h>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace cppreact {
  class X11_renderer : public renderer {
  Display* _x11_Display;
  Window _x11_Window;
  int _x11_Screen;

  GC _x11_GC_global;
  int _x11_Width;
  int _x11_Height;
  GC _x11_GC_local;

  Pixmap back_buffer;
  
  Atom close_msg;
  std::string title;
public:
    X11_renderer(uint16_t width,uint16_t height,std::string title,std::initializer_list<component*> children):
    renderer(children),_x11_Width(width),_x11_Height(height), title(title) {
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

      close_msg = XInternAtom(_x11_Display, "WM_DELETE_WINDOW", False);
      XSetWMProtocols(_x11_Display, _x11_Window, &close_msg, 1);

      set_size(_x11_Width, _x11_Height);
      //Commit changes to X11
      XFlush(_x11_Display);

      back_buffer = XCreatePixmap(_x11_Display, _x11_Window, 4096, 4096, DefaultDepth(_x11_Display, _x11_Screen));
      XFillRectangle(_x11_Display, back_buffer, _x11_GC_global, box.x, box.y, box.width, box.height);
      render();

      XCopyArea(_x11_Display, back_buffer, _x11_Window, _x11_GC_global,0, 0, _x11_Width, _x11_Height, 0, 0);
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
        XFlush(_x11_Display);

    }
    void on_loop() override {
      XEvent e;
      XNextEvent(_x11_Display, &e);
      switch (e.type) {
        case MotionNotify:
        set_pointer_move(e.xmotion.x, e.xmotion.y);
        render();
        XCopyArea(_x11_Display, back_buffer, _x11_Window, _x11_GC_global,0, 0, _x11_Width, _x11_Height, 0, 0);
        break;
        case ConfigureNotify:
        if (_x11_Width == e.xconfigure.width && _x11_Height == e.xconfigure.height) break;
        _x11_Width = e.xconfigure.width;
        _x11_Height = e.xconfigure.height;
        set_size(_x11_Width, _x11_Height); 
        render();
        XCopyArea(_x11_Display, back_buffer, _x11_Window, _x11_GC_global,0, 0, _x11_Width, _x11_Height, 0, 0);
        break;
        case Expose:
        XCopyArea(_x11_Display, back_buffer, _x11_Window, _x11_GC_global,0, 0, _x11_Width, _x11_Height, 0, 0);
        break;
        case ClientMessage:
        if (e.xclient.data.l[0] == close_msg) close();
        break;  
      }
    }
    void on_rect(color c, bounding_box box) override {
      XSetForeground(_x11_Display, _x11_GC_local, _encode_RGB(c.r, c.g, c.b));
      XSetBackground(_x11_Display, _x11_GC_local, _encode_RGB(c.r, c.g, c.b));
      XFillRectangle(_x11_Display, back_buffer, _x11_GC_local, box.x, box.y, box.width, box.height);
    }
  };
}

#endif
