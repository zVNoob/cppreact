#ifndef _CPPREACT_X11_RENDERER_HPP
#define _CPPREACT_X11_RENDERER_HPP
#include "renderer.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/render.h>
#include <X11/extensions/renderproto.h>
#include <X11/extensions/Xrender.h>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace cppreact {
  class X11_renderer : public renderer {
  Display* _x11_Display;
  Window _x11_Window;
  int _x11_Screen;

  GC _x11_GC_global;
  int _x11_Width;
  int _x11_Height;

  Pixmap back_buffer;
  Picture back_buffer_pict;
  XRenderPictFormat* back_buffer_format;

  Picture window_buffer_pict;
  
  Atom close_msg;
  std::string title;

  GC rgba_gc;
  int gc_depth(int depth, Display *dpy, Window scr, Window root, GC *gc) {
        Window win;
        Visual *visual;
        XVisualInfo vis_info;
        XSetWindowAttributes win_attr;
        unsigned long win_mask;

        XMatchVisualInfo(dpy, scr, depth, TrueColor, &vis_info);
    
        visual = vis_info.visual;

        win_attr.colormap = XCreateColormap(dpy, root, visual, AllocNone);
        win_attr.background_pixel = 0;
        win_attr.border_pixel = 0;

        win_mask = CWBackPixel | CWColormap | CWBorderPixel;

        win = XCreateWindow(
                        dpy, root,
                        0, 0,
                        100, 100,        /* dummy size */
                        0, depth,
                        InputOutput, visual,
                        win_mask, &win_attr);
        /* To flush out any errors */
        *gc = XCreateGC(dpy, win, 0, 0);

        XDestroyWindow(dpy, win);

        return 0;
  }
  
public:
    X11_renderer(uint16_t width,uint16_t height,std::string title):
    _x11_Width(width),_x11_Height(height), title(title) {
      _x11_Display = XOpenDisplay(0);
      if (_x11_Display == NULL) throw std::runtime_error("X11 not supported");
      {
        int dumb1,dumb2;
        if (!XRenderQueryVersion(_x11_Display, &dumb1, &dumb2)) throw std::runtime_error("XRender not supported");
      }
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
      XGCValues gc_values;
      _x11_GC_global = XCreateGC(_x11_Display, _x11_Window, 0, &gc_values);

      gc_depth(32, _x11_Display, _x11_Screen, DefaultRootWindow(_x11_Display), &rgba_gc);

      close_msg = XInternAtom(_x11_Display, "WM_DELETE_WINDOW", False);
      XSetWMProtocols(_x11_Display, _x11_Window, &close_msg, 1);

      set_size(_x11_Width, _x11_Height);
      //Commit changes to X11
      XFlush(_x11_Display);

      back_buffer = XCreatePixmap(_x11_Display, _x11_Window, 4096, 4096, DefaultDepth(_x11_Display, _x11_Screen));
      back_buffer_format = XRenderFindStandardFormat(_x11_Display, PictStandardRGB24);

      XRenderPictureAttributes pict_attr;
      back_buffer_pict = XRenderCreatePicture(_x11_Display, back_buffer, back_buffer_format,0,&pict_attr);
      window_buffer_pict = XRenderCreatePicture(_x11_Display, _x11_Window, back_buffer_format,0,&pict_attr);
      
    };
    ~X11_renderer() {
      XRenderFreePicture(_x11_Display, back_buffer_pict);
      XRenderFreePicture(_x11_Display, window_buffer_pict);
      XFreePixmap(_x11_Display, back_buffer);
      XUnmapWindow(_x11_Display, _x11_Window);
      XDestroyWindow(_x11_Display, _x11_Window);
      XCloseDisplay(_x11_Display);
    }
protected:
    void clear_buffer() {
      XRenderColor color = {0,0,0,0};
      XRenderFillRectangle(_x11_Display, PictOpSrc, back_buffer_pict, &color, 0, 0, _x11_Width, _x11_Height);
    }
    void on_loop(component* root) override {
      XEvent e;
      XNextEvent(_x11_Display, &e);
      switch (e.type) {
        case MotionNotify:
        clear_buffer();
        render(root);
        XRenderComposite(_x11_Display, PictOpSrc, back_buffer_pict, None, window_buffer_pict, 0, 0, 0, 0,0,0, _x11_Width, _x11_Height);
        break;
        case ConfigureNotify:
        if (_x11_Width == e.xconfigure.width && _x11_Height == e.xconfigure.height) break;
        _x11_Width = e.xconfigure.width;
        _x11_Height = e.xconfigure.height;
        set_size(_x11_Width, _x11_Height); 
        clear_buffer();
        render(root);
        XRenderComposite(_x11_Display, PictOpSrc, back_buffer_pict, None, window_buffer_pict, 0, 0, 0, 0,0,0, _x11_Width, _x11_Height);
        break;
        case Expose:
        XRenderComposite(_x11_Display, PictOpSrc, back_buffer_pict, None, window_buffer_pict, 0, 0, 0, 0,0,0, _x11_Width, _x11_Height);
        break;
        case ClientMessage:
        if (e.xclient.data.l[0] == close_msg) exit();
        break;  
      }
    }
    void on_begin_loop(component* root) override {
      render(root);
      XRenderComposite(_x11_Display, PictOpSrc, back_buffer_pict, None, window_buffer_pict, 0, 0, 0, 0,0,0, _x11_Width, _x11_Height);
    }
    void on_rect(bounding_box box, color c, _BlendMode blend) override {
      XRenderColor color = {static_cast<unsigned short>(c.r * 255),
                            static_cast<unsigned short>(c.g * 255), 
                            static_cast<unsigned short>(c.b * 255), 
                            static_cast<unsigned short>(c.a * 255)};
      int op = PictOpOver;
      if (blend == BLEND_ADD) op = PictOpAdd;
      if (blend == BLEND_MULTIPLY) op = PictOpMultiply;
      XRenderFillRectangle(_x11_Display, op, back_buffer_pict, &color, box.x, box.y, box.width, box.height);
    }
  };
}

#endif

