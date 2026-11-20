/* White-box tests for deterministic gameplay, without a window or device. */
#include "../src/game.c"
#include <assert.h>
#include <stdio.h>
int main(void) {
  init();
  assert(mode == MENU);
  tap(42, 380);
  assert(mode == HELP);
  tap(5, 5);
  assert(mode == MENU);
  tap(60, 340);
  assert(mode == PLAY && owner == selected);
  float x = p[selected].x;
  axisX = 1;
  tick(.016f);
  assert(p[selected].x > x);
  axisX = 0;
  action(PASS);
  assert(owner == -1 && selected != 4);
  action(BACK);
  float t = remaining;
  tick(.03f);
  assert(mode == PAUSE && remaining == t);
  action(START);
  setup(0);
  action(SHOT);
  assert(owner == -1 && vx > 0);
  bx = 728;
  by = 226;
  vx = 300;
  vy = 0;
  tick(.02f);
  assert(mode == GOAL && score[0] == 1);
  for (int i = 0; i < 100; i++)
    tick(.02f);
  assert(mode == PLAY);
  remaining = 60.01f;
  tick(.02f);
  assert(half == 2 && owner == 9);
  remaining = .01f;
  tick(.02f);
  assert(mode == END);
  action(START);
  assert(mode == PLAY && remaining == 120 && !score[0]);
  mobile = 1;
  tap(719, 21);
  assert(mode == PAUSE);
  tap(380, 255);
  assert(mode == MENU);
  tap(33, 340);
  tap(690, 400);
  assert(owner == -1);
  init();
  action(START);
  for (int i = 0; i < 13000 && mode != END; i++) {
    axisX = (i / 90) % 3 - 1;
    axisY = (i / 120) % 3 - 1;
    if (i % 127 == 0)
      action(PASS);
    if (i % 310 == 0)
      action(SHOT);
    tick(.016f);
    if (i % 120 == 0)
      draw();
    for (int j = 0; j < 10; j++)
      assert(p[j].x >= 42 && p[j].x <= 726 && p[j].y >= 91 && p[j].y <= 367);
  }
  assert(mode == END);
  puts("OK : menus, mouvement, passe, tir, but, engagement, pause, tactile, "
       "mi-temps et match complet.");
  return 0;
}
