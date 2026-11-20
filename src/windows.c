#define WIN32_LEAN_AND_MEAN
#include "game.h"
#include <windows.h>
static int w, h, ox, oy;
static BITMAPINFO bi;
static unsigned char keys[256];
static void viewport(HWND win) {
  RECT r;
  GetClientRect(win, &r);
  w = r.right;
  h = w * H / W;
  if (h > r.bottom) {
    h = r.bottom;
    w = h * W / H;
  }
  ox = (r.right - w) / 2;
  oy = (r.bottom - h) / 2;
}
static LRESULT CALLBACK events(HWND win, UINT m, WPARAM k, LPARAM v) {
  switch (m) {
  case WM_SIZE:
    viewport(win);
    return 0;
  case WM_KILLFOCUS:
    for (int i = 0; i < 256; i++)
      keys[i] = 0;
    if (mode == PLAY)
      action(BACK);
    return 0;
  case WM_LBUTTONDOWN:
    if (w && h)
      tap(((short)LOWORD(v) - ox) * W / w, ((short)HIWORD(v) - oy) * H / h);
    return 0;
  case WM_KEYDOWN:
    if (k < 256)
      keys[k] = 1;
    if (v & (1L << 30))
      return 0;
    if (k == VK_ESCAPE || k == 'P')
      action(BACK);
    else if (k == VK_RETURN)
      action(START);
    else if (k == 'C')
      action(PASS);
    else if (k == VK_SPACE || k == 'X')
      action(SHOT);
    else if (k == VK_TAB)
      action(SWITCH);
    else if (k == 'H' && mode == MENU)
      mode = HELP;
    else if (k == 'M' && mode == PAUSE)
      mode = MENU;
    return 0;
  case WM_KEYUP:
    if (k < 256)
      keys[k] = 0;
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(win, &ps);
    RECT r;
    GetClientRect(win, &r);
    if (ox || oy)
      FillRect(dc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, ox, oy, w, h, 0, 0, W, H, pixels, &bi, DIB_RGB_COLORS,
                  SRCCOPY);
    EndPaint(win, &ps);
    return 0;
  }
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(win, m, k, v);
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev, LPSTR cmd, int show) {
  (void)prev;
  (void)cmd;
  SetProcessDPIAware();
  WNDCLASSW wc = {0};
  wc.lpfnWndProc = events;
  wc.hInstance = instance;
  wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
  wc.lpszClassName = L"SportFootArcade";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClassW(&wc);
  RECT r = {0, 0, 1152, 648};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
  HWND win = CreateWindowW(
      wc.lpszClassName, L"Sport Foot | Arcade Football Club",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
      r.bottom - r.top, NULL, NULL, instance, NULL);
  if (!win)
    return 1;
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = W;
  bi.bmiHeader.biHeight = -H;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  init();
  draw();
  ShowWindow(win, show);
  LARGE_INTEGER freq, t, now;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&t);
  for (;;) {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT)
        return 0;
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    QueryPerformanceCounter(&now);
    float dt = (float)(now.QuadPart - t.QuadPart) / freq.QuadPart;
    if (dt < 1.f / 60) {
      Sleep(1);
      continue;
    }
    t = now;
    axisX = (keys[VK_RIGHT] || keys['D']) -
            (keys[VK_LEFT] || keys['A'] || keys['Q']);
    axisY =
        (keys[VK_DOWN] || keys['S']) - (keys[VK_UP] || keys['W'] || keys['Z']);
    sprint = keys[VK_SHIFT];
    tick(dt);
    draw();
    InvalidateRect(win, NULL, FALSE);
  }
}
