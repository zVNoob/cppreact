#include <initializer_list>
#include <minwindef.h>
#include <string>
#include <windows.h>
#include <wingdi.h>
#include "renderer.hpp"

namespace cppreact {
  class Win32_renderer : public renderer {
  private:
  int width,height;
  std::string title;
  HWND hwnd;
  HDC hdc;
  HDC draw_hdc;
  PAINTSTRUCT ps;
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Win32_renderer* obj = reinterpret_cast<Win32_renderer*>(lParam);
    switch (msg) {
    case WM_PAINT:
      // Handle WM_PAINT message
      obj->draw_hdc = BeginPaint(obj->hwnd, &obj->ps);
      obj->render();
      EndPaint(obj->hwnd,&obj->ps);
      break;
    case WM_DESTROY:
      // Handle WM_DESTROY message
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
  }
  public:
  Win32_renderer(int width,int height,std::string title = "",std::initializer_list<component*> children = {}) :
    renderer(children),width(width),height(height),title(title) {
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

      hdc = GetDC(hwnd);
      ShowWindow(hwnd, SW_SHOW);
    }
    ~Win32_renderer() {
      ReleaseDC(hwnd, hdc);
      DestroyWindow(hwnd);
      UnregisterClass("Win32_renderer", GetModuleHandle(NULL));
    }
    protected:
    bool on_loop() override {
      MSG msg;
      BOOL bRet;
      bRet = GetMessage(&msg, NULL, 0, 0);
      if (bRet == 0 || bRet == -1) return false;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      return 1;
    } 
    void on_rect(color c, bounding_box box) override {
      SetDCPenColor(draw_hdc, RGB(c.r,c.g,c.b));
      SetDCBrushColor(draw_hdc, RGB(c.r,c.g,c.b));
      Rectangle(draw_hdc,box.x,box.y,box.x+box.width,box.y+box.height);
    };
  };
}
