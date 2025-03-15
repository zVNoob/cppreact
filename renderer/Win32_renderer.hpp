#ifndef _CPPREACT_WIN32_RENDERER_HPP
#define _CPPREACT_WIN32_RENDERER_HPP

#include <cstdint>
#include <initializer_list>
#include <minwindef.h>
#include <string>
#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
#include <winuser.h>
#include "renderer.hpp"

namespace cppreact {
  class Win32_renderer : public renderer {
  private:
  int width,height;
  std::string title;
  HWND hwnd;
  HDC draw_hdc;
  struct {
    HDC hdc;
    HBITMAP hbm;
    uint16_t width;
    uint16_t height;
    bool inited = false;
  } backbuffer;
  PAINTSTRUCT ps;
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Win32_renderer* obj = 0;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = (CREATESTRUCT*) lParam;
        obj = reinterpret_cast<Win32_renderer*>(cs->lpCreateParams);
        HDC hdc = GetDC(hwnd);
        obj->backbuffer.hbm = CreateCompatibleBitmap(hdc, 4096,4096);

        obj->backbuffer.hdc = CreateCompatibleDC(hdc);
        ReleaseDC(hwnd, hdc);
        SelectObject(obj->backbuffer.hdc, obj->backbuffer.hbm);
        DeleteObject(obj->backbuffer.hbm);
        SetLastError(0);
        if (SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) obj) == 0) {
            if (GetLastError() != 0)
                return FALSE;
        }
    } else {
        obj = reinterpret_cast<Win32_renderer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    switch (msg) {
    case WM_PAINT:
      // Handle WM_PAINT message
      obj->on_paint();
      break;
    case WM_SIZE:
      obj->on_size();
      break;
    case WM_MOUSEMOVE:
      obj->on_mousemove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
      break;
    case WM_ERASEBKGND:
      break;
    case WM_CLOSE:
      close();
      break;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return TRUE;
  }
  void on_mousemove(uint16_t x,uint16_t y) {
    set_pointer(x, y);
    RECT RcClient;
    GetClientRect(hwnd,&RcClient);
    InvalidateRect(hwnd,&RcClient,FALSE);

  }
  void on_size() {
    RECT RcClient;
    GetClientRect(hwnd,&RcClient);
    set_size(RcClient.right - RcClient.left,RcClient.bottom - RcClient.top);
    InvalidateRect(hwnd,&RcClient,FALSE);
  }
  void on_paint() {
    render();
    draw_hdc = BeginPaint(hwnd, &ps);
    BitBlt(draw_hdc, 0, 0, width, height, backbuffer.hdc, 0, 0, SRCCOPY);
    EndPaint(hwnd,&ps);
  }
  public:
  Win32_renderer(int width,int height,std::string title = "",std::initializer_list<component*> children = {}) :
    renderer(children,{255,255,255,255}),width(width),height(height),title(title) {
      set_size(width,height);
      WNDCLASSEX wc = {0};
      wc.cbSize = sizeof(WNDCLASSEX);
      wc.style = 0;
      wc.lpfnWndProc = WndProc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.hInstance = GetModuleHandle(NULL);
      wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
      wc.hCursor = LoadCursor(NULL, IDC_ARROW);
      wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
      wc.lpszMenuName = NULL;
      wc.lpszClassName = "Win32_renderer";
      wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
      RegisterClassEx(&wc);
      hwnd = CreateWindowEx(
      0, "Win32_renderer", title.c_str(),
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
      width, height, NULL, NULL, GetModuleHandle(NULL), this);

      ShowWindow(hwnd, SW_SHOW);
    }
    ~Win32_renderer() {
      DestroyWindow(hwnd);
      UnregisterClass("Win32_renderer", GetModuleHandle(NULL));
    }
    protected:
    bool on_loop() override {
      MSG msg;
      BOOL bRet;
      bRet = GetMessage(&msg, NULL, 0, 0);
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    void on_rect(color c, bounding_box box) override {
      HBRUSH br =  CreateSolidBrush(RGB(c.r,c.g,c.b));
      RECT rect = {box.x, box.y, box.x +box.width, box.y + box.height};
      FillRect(backbuffer.hdc,&rect,br);
      DeleteObject(br);
    };
  };
}

#endif
