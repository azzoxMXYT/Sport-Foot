#include "game.h"
#include <android/choreographer.h>
#include <android/looper.h>
#include <android/native_activity.h>
static ANativeWindow *win;
static AInputQueue *queue;
static int active, pending, sw, sh, bw, bh, ox, oy, stick = -1;
static float ax, ay;
static long prev;
static void frame(long ns, void *data);
static void schedule(void) {
  if (win && active && !pending) {
    pending = 1;
    AChoreographer_postFrameCallback(AChoreographer_getInstance(), frame, 0);
  }
}
static void frame(long ns, void *data) {
  (void)data;
  pending = 0;
  if (!active || !win)
    return;
  tick(prev ? (float)(ns - prev) * 1e-9f : .016f);
  prev = ns;
  draw();
  ANativeWindow_Buffer b;
  if (!ANativeWindow_lock(win, &b, 0)) {
    unsigned *out = b.bits;
    for (int y = 0; y < b.height; y++) {
      for (int x = 0; x < b.width; x++) {
        int xx = x - ox, yy = y - oy;
        unsigned c = xx >= 0 && xx < W && yy >= 0 && yy < H
                         ? pixels[yy * W + xx]
                         : 0x101b30;
        out[x] = 0xff000000 | ((c & 255) << 16) | (c & 0xff00) | (c >> 16);
      }
      out += b.stride;
    }
    ANativeWindow_unlockAndPost(win);
  }
  schedule();
}
static void surface(ANativeActivity *a, ANativeWindow *w) {
  (void)a;
  win = w;
  sw = ANativeWindow_getWidth(w);
  sh = ANativeWindow_getHeight(w);
  if (sw < 1 || sh < 1)
    return;
  bw = W;
  bh = sh * W / sw;
  if (bh < H) {
    bh = H;
    bw = sw * H / sh;
  }
  ox = (bw - W) / 2;
  oy = (bh - H) / 2;
  ANativeWindow_setBuffersGeometry(w, bw, bh, 1);
  schedule();
}
static void lost(ANativeActivity *a, ANativeWindow *w) {
  (void)a;
  (void)w;
  win = 0;
  prev = 0;
}
static void stopstick(void) {
  axisX = axisY = 0;
  stick = -1;
  sprint = 0;
}
static int input(int fd, int ev, void *d) {
  (void)fd;
  (void)ev;
  (void)d;
  AInputEvent *e;
  while (queue && AInputQueue_getEvent(queue, &e) >= 0) {
    if (AInputQueue_preDispatchEvent(queue, e))
      continue;
    int used = 0;
    if (AInputEvent_getType(e) == 2 && sw && sh) {
      int raw = AMotionEvent_getAction(e), a = raw & 255,
          index = (raw >> 8) & 255;
      for (unsigned i = 0; i < AMotionEvent_getPointerCount(e); i++) {
        int id = AMotionEvent_getPointerId(e, i);
        float x = AMotionEvent_getX(e, i) * bw / sw - ox,
              y = AMotionEvent_getY(e, i) * bh / sh - oy;
        if ((a == 0 || a == 5) && (int)i == index) {
          if (mode == PLAY && x < W / 2 && stick < 0) {
            stick = id;
            ax = x;
            ay = y;
          } else
            tap(x, y);
        }
        if (a == 2 && id == stick) {
          float dx = (x - ax) / 26, dy = (y - ay) / 26;
          axisX = dx;
          axisY = dy;
          sprint = dx * dx + dy * dy > 1.3f;
        }
        if ((a == 1 || a == 6) && (int)i == index && id == stick)
          stopstick();
      }
      if (a == 3)
        stopstick();
      used = 1;
    } else if (AInputEvent_getType(e) == 1 && AKeyEvent_getKeyCode(e) == 4 &&
               mode != MENU) {
      if (AKeyEvent_getAction(e) == 1)
        action(BACK);
      used = 1;
    }
    AInputQueue_finishEvent(queue, e, used);
  }
  return 1;
}
static void attached(ANativeActivity *a, AInputQueue *q) {
  (void)a;
  queue = q;
  AInputQueue_attachLooper(q, ALooper_forThread(), -1, input, 0);
}
static void detached(ANativeActivity *a, AInputQueue *q) {
  (void)a;
  AInputQueue_detachLooper(q);
  queue = 0;
  stopstick();
}
static void resume(ANativeActivity *a) {
  (void)a;
  active = 1;
  prev = 0;
  schedule();
}
static void pauseGame(ANativeActivity *a) {
  (void)a;
  active = 0;
  prev = 0;
  stopstick();
  if (mode == PLAY)
    action(BACK);
}
void ANativeActivity_onCreate(ANativeActivity *a, void *saved, size_t size) {
  (void)saved;
  (void)size;
  mobile = 1;
  init();
  a->callbacks->onResume = resume;
  a->callbacks->onPause = pauseGame;
  a->callbacks->onNativeWindowCreated = surface;
  a->callbacks->onNativeWindowDestroyed = lost;
  a->callbacks->onInputQueueCreated = attached;
  a->callbacks->onInputQueueDestroyed = detached;
}
