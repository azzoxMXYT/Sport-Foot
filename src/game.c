#include "game.h"
/* Pitch coordinates are in logical pixels; gameplay is independent of the
 * display. */
typedef unsigned int Col;
Col pixels[W * H];
float axisX, axisY;
int mode, mobile, sprint;
static float min(float a, float b) { return a < b ? a : b; }
static float max(float a, float b) { return a > b ? a : b; }
static float clamp(float a, float lo, float hi) { return max(lo, min(hi, a)); }
static float ab(float a) { return max(a, -a); }
static float root(float x) {
  if (x < .00001f)
    return 0;
  float r = x > 1 ? x : 1;
  for (int i = 0; i < 13; i++)
    r = (r + x / r) * .5f;
  return r;
}
static void rect(int x, int y, int w, int h, Col c) {
  int x2 = min(W, x + w), y2 = min(H, y + h);
  x = max(0, x);
  y = max(0, y);
  for (int j = y; j < y2; j++)
    for (int i = x; i < x2; i++)
      pixels[j * W + i] = c;
}
static void line(int x, int y, int X, int Y, Col c) {
  int dx = ab(X - x), dy = -ab(Y - y), sx = x < X ? 1 : -1, sy = y < Y ? 1 : -1,
      e = dx + dy;
  for (;;) {
    rect(x, y, 1, 1, c);
    if (x == X && y == Y)
      break;
    int t = 2 * e;
    if (t >= dy) {
      e += dy;
      x += sx;
    }
    if (t <= dx) {
      e += dx;
      y += sy;
    }
  }
}
static void ellipse(int x, int y, int a, int b, Col c) {
  for (int j = -b; j <= b; j++) {
    int s = a * root(1.f - (float)(j * j) / (b * b));
    rect(x - s, y + j, 2 * s + 1, 1, c);
  }
}
static void box(int x, int y, int w, int h, Col c) {
  rect(x, y, w, 1, c);
  rect(x, y + h - 1, w, 1, c);
  rect(x, y, 1, h, c);
  rect(x + w - 1, y, 1, h, c);
}
/* Five-column pixel alphabet, top pixel in the low bit. */
static const unsigned char font[][5] = {
    {126, 17, 17, 17, 126}, {127, 73, 73, 73, 54},  {62, 65, 65, 65, 34},
    {127, 65, 65, 34, 28},  {127, 73, 73, 73, 65},  {127, 9, 9, 9, 1},
    {62, 65, 73, 73, 122},  {127, 8, 8, 8, 127},    {65, 65, 127, 65, 65},
    {32, 64, 65, 63, 1},    {127, 8, 20, 34, 65},   {127, 64, 64, 64, 64},
    {127, 2, 12, 2, 127},   {127, 4, 8, 16, 127},   {62, 65, 65, 65, 62},
    {127, 9, 9, 9, 6},      {62, 65, 81, 33, 94},   {127, 9, 25, 41, 70},
    {70, 73, 73, 73, 49},   {1, 1, 127, 1, 1},      {63, 64, 64, 64, 63},
    {31, 32, 64, 32, 31},   {127, 32, 24, 32, 127}, {99, 20, 8, 20, 99},
    {3, 4, 120, 4, 3},      {97, 81, 73, 69, 67},   {62, 81, 73, 69, 62},
    {0, 66, 127, 64, 0},    {98, 81, 73, 73, 70},   {34, 65, 73, 73, 54},
    {24, 20, 18, 127, 16},  {39, 69, 69, 69, 57},   {60, 74, 73, 73, 48},
    {1, 113, 9, 5, 3},      {54, 73, 73, 73, 54},   {6, 73, 73, 41, 30},
    {0, 0, 36, 0, 0},       {8, 8, 8, 8, 8},        {64, 32, 16, 8, 4},
    {0, 0, 64, 0, 0},       {0, 20, 20, 20, 0},     {0, 65, 34, 20, 8}};
static void text(int x, int y, const char *s, int z, Col c, int italic) {
  for (; *s; s++, x += 6 * z) {
    int k = *s >= 'A' && *s <= 'Z'   ? *s - 'A'
            : *s >= '0' && *s <= '9' ? 26 + *s - '0'
            : *s == ':'              ? 36
            : *s == '-'              ? 37
            : *s == '/'              ? 38
            : *s == '.'              ? 39
            : *s == '='              ? 40
            : *s == '>'              ? 41
                                     : -1;
    if (*s == 39) {
      rect(x + z, y, z, 2 * z, c);
      continue;
    }
    if (k < 0)
      continue;
    for (int a = 0; a < 5; a++)
      for (int b = 0; b < 7; b++)
        if ((font[k][a] >> b) & 1)
          rect(x + a * z + (italic ? (6 - b) * z / 3 : 0), y + b * z, z, z, c);
  }
}
static int len(const char *s) {
  int n = 0;
  while (*s++)
    n++;
  return n;
}
static void centered(int x, int y, const char *s, int z, Col c) {
  text(x - (len(s) * 6 - 1) * z / 2, y, s, z, c, 0);
}
#define INK 0x101b30
#define NAVY 0x172944
#define WHITE 0xf7f4df
#define LIME 0xdafa53
#define BLUE 0x54bdfb
#define RED 0xff604e
#define MUTED 0xa0b3bd
static unsigned hash(unsigned i) {
  i ^= i >> 16;
  i *= 0x45d9f3bu;
  i ^= i >> 16;
  return i;
}
static void badge(int x, int y, Col c) {
  rect(x + 3, y, 16, 16, INK);
  rect(x + 5, y + 16, 12, 3, INK);
  rect(x + 8, y + 19, 6, 2, INK);
  rect(x + 5, y + 2, 12, 12, c);
  rect(x + 7, y + 14, 8, 3, c);
  rect(x + 10, y + 17, 2, 2, c);
  rect(x + 8, y + 4, 3, 8, WHITE);
  rect(x + 12, y + 4, 2, 8, WHITE);
}
/* Layers of a hand-drawn arcade footballer. */
static const char *hero[] = {
    "         11111      ", "       11111111     ", "       12222211     ",
    "       1222322      ", "        222222      ", "         2233       ",
    "      44424444      ", "     4555445544     ", "    55555555554     ",
    "    2255656555222   ", "   222 566655  222  ", "  222  565655   222 ",
    "  22   555554    22 ", "       444444       ", "      77777777      ",
    "      7777 7777     ", "     7777   777     ", "     222    222     ",
    "    222      222    ", "    666      666    ", "   666       666    ",
    "  1111       11111  "};
static void athlete(int x, int y, int scale, int side, int id, int gait) {
  Col pal[] = {0,
               INK,
               id % 3 == 0 ? 0xb97549 : 0xe3a674,
               0x986044,
               scale == 1 && id % 5 == 0 ? 0xb79523
               : side                    ? 0xa63443
                                         : 0x236fa7,
               scale == 1 && id % 5 == 0 ? 0xffda58
               : side                    ? RED
                                         : BLUE,
               WHITE,
               side ? 0x632c48 : 0x263959};
  for (int j = 0; j < 22; j++)
    for (int i = 0; hero[j][i]; i++) {
      int t = hero[j][i] - '0';
      if (t > 0)
        rect(x + (i - 10) * scale + (j > 15 ? (i < 10 ? gait : -gait) : 0),
             y + (j - 22) * scale, scale, scale, pal[t]);
    }
}
static void football(int x, int y, int r) {
  ellipse(x, y + 1, r + 1, r, INK);
  ellipse(x, y, r, r, WHITE);
  rect(x - r / 3, y - r / 3, r / 2 + 1, r / 2 + 1, INK);
  rect(x + r / 2, y - 1, max(1, r / 3), max(1, r / 3), INK);
  rect(x - r / 2, y + r / 2, max(1, r / 3), max(1, r / 4), INK);
}
static void crowd(int top, int bottom) {
  rect(0, top, W, bottom - top, NAVY);
  Col p[] = {0x667487, 0x90a3a8, 0x384b68, 0xb0786e, 0xc4a47f, 0x56798d};
  for (int y = top + 1; y < bottom - 3; y += 6)
    for (int x = -2; x < W; x += 7) {
      unsigned r = hash(x + 7 + y * 768);
      int k = x + r % 3;
      rect(k, y, 3, 2, p[r % 6]);
      rect(k - 1, y + 2, 5, 2, p[(r >> 8) % 6]);
    }
  rect(0, bottom - 3, W, 3, INK);
}
#ifdef SF_RUNTIME_COVER
/* UIKit decodes the bundled PNG once at launch. */
unsigned int sfCover[411][408];
#define COVER_AT(y, x) sfCover[y][x]
#else
#include "cover.inc"
#define COVER_AT(y, x) inkset[artwork[y][x]]
#endif
static void cover(void) {
  for (int y = 0; y < 411; y++)
    for (int x = 0; x < 408; x++)
      pixels[y * W + 360 + x] = COVER_AT(y, x);
  for (int y = 0; y < H; y++) {
    int width = 452 - y / 5;
    rect(0, y, width, 1, INK);
    rect(width, y, 4, 1, LIME);
  }
  rect(26, 26, 26, 4, LIME);
  text(63, 25, "SPORT / FOOT", 1, WHITE, 0);
  text(28, 86, "EDITION 01   /   FOOTBALL ARCADE", 1, LIME, 0);
  text(24, 112, "SPORT", 9, WHITE, 1);
  text(25, 187, "FOOT", 10, LIME, 1);
  rect(28, 282, 26, 2, BLUE);
  text(28, 297, "MATCH RAPIDE   /   5 CONTRE 5", 1, WHITE, 0);
  rect(28, 327, 258, 36, LIME);
  text(45, 338, "COUP D'ENVOI", 2, INK, 0);
  text(251, 338, ">", 2, INK, 0);
  text(29, 379, "COMMANDES  >", 1, MUTED, 0);
  rect(0, 411, W, 21, INK);
  text(28, 418, "SF / 01", 1, LIME, 0);
  text(106, 418, "ARCADE FOOTBALL CLUB", 1, MUTED, 0);
  text(608, 418, "STADE DES DUNES", 1, WHITE, 0);
}
typedef struct {
  float x, y, hx, hy, walk;
} Player;
static Player p[10];
static float bx, by, vx, vy, lock, hold, remaining, stopTime, fx = 1, fy,
                                                              elapsed;
static int owner, selected, last, score[2], lastGoal, half;
static const int px[5] = {56, 207, 305, 344, 373},
                 py[5] = {226, 225, 147, 307, 226};
static float distance(float x, float y, float X, float Y) {
  return root((X - x) * (X - x) + (Y - y) * (Y - y));
}
static void setup(int side) {
  for (int i = 0; i < 10; i++) {
    int n = i % 5;
    p[i] = (Player){i < 5 ? px[n] : W - px[n], py[n], i < 5 ? px[n] : W - px[n],
                    py[n], 0};
  }
  owner = side ? 9 : 4;
  selected = 4;
  last = owner;
  bx = p[owner].x;
  by = p[owner].y;
  vx = vy = 0;
  hold = 0;
  lock = .7f;
  fx = 1;
  fy = 0;
}
void init(void) {
  mode = MENU;
  score[0] = score[1] = 0;
  remaining = 120;
  half = 1;
  axisX = axisY = 0;
  sprint = 0;
  elapsed = 0;
  setup(0);
}
static void move(int i, float x, float y, float speed, float dt) {
  float dx = x - p[i].x, dy = y - p[i].y, d = root(dx * dx + dy * dy);
  float s = min(d, speed * dt);
  if (d > .1f) {
    p[i].x += dx / d * s;
    p[i].y += dy / d * s;
    p[i].walk += s;
  }
  p[i].x = clamp(p[i].x, 42, 726);
  p[i].y = clamp(p[i].y, 91, 367);
}
static void kick(float x, float y, float speed) {
  if (owner < 0)
    return;
  float dx = x - p[owner].x, dy = y - p[owner].y,
        d = max(1, root(dx * dx + dy * dy));
  bx = p[owner].x + 6 * dx / d;
  by = p[owner].y + 6 * dy / d;
  vx = dx / d * speed;
  vy = dy / d * speed;
  last = owner;
  owner = -1;
  lock = .2f;
  hold = 0;
}
static int nearest(int team, float x, float y, int skip) {
  float m = 10000;
  int k = team + 1;
  for (int i = team + 1; i < team + 5; i++)
    if (i != skip) {
      float d = distance(x, y, p[i].x, p[i].y);
      if (d < m)
        m = d, k = i;
    }
  return k;
}
void action(int a) {
  if (a == BACK) {
    if (mode == HELP)
      mode = MENU;
    else if (mode == PAUSE)
      mode = PLAY;
    else if (mode == PLAY)
      mode = PAUSE;
    else if (mode == END)
      mode = MENU;
    return;
  }
  if (a == START) {
    if (mode == MENU || mode == END) {
      init();
      mode = PLAY;
    } else if (mode == PAUSE)
      mode = PLAY;
    else if (mode == HELP)
      mode = MENU;
    return;
  }
  if (mode != PLAY)
    return;
  if (owner != selected) {
    if (a == SWITCH || a == PASS)
      selected = nearest(0, bx, by, selected);
    return;
  }
  if (a == SHOT)
    kick(739, clamp(226 + axisY * 29, 197, 255), 368);
  else if (a == PASS) {
    float best = -100000;
    int k = -1;
    for (int i = 1; i < 5; i++)
      if (i != selected) {
        float dx = p[i].x - bx, dy = p[i].y - by,
              d = max(1, root(dx * dx + dy * dy)),
              rating = (dx * fx + dy * fy) / d * 140 - d * .25f;
        if (rating > best) {
          best = rating;
          k = i;
        }
      }
    if (k >= 0) {
      kick(p[k].x, p[k].y, 265);
      selected = k;
    }
  }
}
void tap(int x, int y) {
  if (mode == MENU) {
    if (x < 323 && y >= 323 && y <= 368)
      action(START);
    else if (x < 255 && y >= 370 && y < 404)
      mode = HELP;
  } else if (mode == HELP)
    mode = MENU;
  else if (mode == END)
    action(START);
  else if (mode == PAUSE) {
    if (y > 235 && y < 279)
      mode = MENU;
    else
      action(START);
  } else if (mode == PLAY) {
    if (x > 698 && y < 53)
      action(BACK);
    else if (mobile && y > 375 && x > 650)
      action(SHOT);
    else if (mobile && y > 375 && x > 532)
      action(PASS);
  }
}
void tick(float dt) {
  dt = clamp(dt, 0, .04f);
  if (mode == GOAL) {
    stopTime -= dt;
    if (stopTime <= 0) {
      setup(1 - lastGoal);
      mode = PLAY;
    }
    return;
  }
  if (mode != PLAY)
    return;
  remaining -= dt;
  elapsed += dt;
  if (remaining <= 0) {
    remaining = 0;
    mode = END;
    return;
  }
  if (half == 1 && remaining <= 60) {
    half = 2;
    setup(1);
  }
  hold += dt;
  lock -= dt;
  float d = root(axisX * axisX + axisY * axisY);
  if (d > .08f) {
    fx = axisX / d;
    fy = axisY / d;
    move(selected, p[selected].x + fx * 50, p[selected].y + fy * 50,
         sprint ? 119 : 94, dt);
  }
  int h = nearest(0, bx, by, -1), a = nearest(5, bx, by, -1);
  for (int i = 0; i < 10; i++) {
    if (i == selected)
      continue;
    int side = i / 5, dir = side ? -1 : 1, keeper = (i % 5 == 0);
    float x = p[i].hx, y = p[i].hy, speed = 57;
    if (owner == i) {
      if (keeper) {
        int k = nearest(side * 5, p[i].x + dir * 160, p[i].y, -1);
        if (hold > .36f)
          kick(p[k].x, p[k].y, 240);
      } else {
        x = side ? 44 : 724;
        y = 226 + (p[i].hy - 226) * .14f;
        speed = 66;
        if (ab(p[i].x - x) < 145 && hold > .4f)
          kick(x + dir * 15, 204 + (int)(elapsed * 17) % 44, 282);
        else if (hold > 1 &&
                 distance(p[i].x, p[i].y, p[selected].x, p[selected].y) < 36) {
          int k = nearest(side * 5, p[i].x + dir * 85, 226, i);
          kick(p[k].x, p[k].y, 225);
        }
      }
    } else if (keeper) {
      x = side ? 718 : 50;
      y = clamp(by, 192, 260);
      speed = 76;
    } else if (i == (side ? a : h) && (owner < 0 || owner / 5 != side)) {
      x = bx;
      y = by;
      speed = side ? 81 : 79;
    } else {
      x = clamp(x + (bx - 384) * .42f +
                    dir * (owner >= 0 && owner / 5 == side ? 60 : -27),
                77, 691);
      y = clamp(y + (by - 226) * .21f, 109, 349);
      if (owner == -1 && selected == i)
        x = bx, y = by;
    }
    move(i, x, y, speed, dt);
  }
  if (owner >= 0) {
    bx = p[owner].x + (owner < 5 ? 6 : -6);
    by = p[owner].y + 2;
    if (lock <= 0)
      for (int i = 0; i < 10; i++)
        if (i / 5 != owner / 5 && distance(p[i].x, p[i].y, bx, by) < 10) {
          owner = i;
          hold = 0;
          lock = .85f;
          if (i > 0 && i < 5)
            selected = i;
          break;
        }
  } else {
    bx += vx * dt;
    by += vy * dt;
    vx *= 1 - .65f * dt;
    vy *= 1 - .65f * dt;
    if (bx < 39 || bx > 729) {
      if (by > 192 && by < 260) {
        lastGoal = bx > 729 ? 0 : 1;
        score[lastGoal]++;
        mode = GOAL;
        stopTime = 1.45f;
        return;
      }
      bx = clamp(bx, 39, 729);
      vx *= -.67f;
    }
    if (by < 90 || by > 368) {
      by = clamp(by, 90, 368);
      vy *= -.72f;
    }
    for (int i = 0; i < 10; i++)
      if ((i != last || lock <= 0) &&
          distance(p[i].x, p[i].y, bx, by) < (i % 5 ? 9 : 12)) {
        owner = i;
        hold = 0;
        lock = .55f;
        if (i > 0 && i < 5)
          selected = i;
        break;
      }
  }
}
static void clockText(char *s, int n) {
  s[0] = '0' + n / 60 / 10;
  s[1] = '0' + n / 60 % 10;
  s[2] = ':';
  s[3] = '0' + n % 60 / 10;
  s[4] = '0' + n % 10;
  s[5] = 0;
}
static void pitch(void) {
  Col chalk = 0xc0e4ad;
  rect(0, 0, W, H, INK);
  crowd(47, 73);
  rect(0, 73, W, 9, 0x30495c);
  for (int x = 14; x < W; x += 108)
    text(x, 74, "SF / ARCADE", 1, WHITE, 0);
  rect(0, 82, W, 296, 0x1d7454);
  for (int i = 0; i < 12; i++)
    rect(39 + 58 * i, 90, 58, 279, i % 2 ? 0x359763 : 0x2f905d);
  for (int i = 0; i < 2250; i++) {
    unsigned n = hash(i + 19);
    int x = 40 + n % 686, y = 91 + (n >> 16) % 276;
    rect(x, y, 2, 1, (x - 39) / 58 % 2 ? 0x379a66 : 0x329361);
  }
  box(39, 90, 691, 279, chalk);
  rect(384, 90, 1, 279, chalk);
  for (int j = -39; j <= 39; j++) {
    int s = root(39 * 39 - j * j);
    rect(384 - s, 226 + j, 2, 1, chalk);
    rect(383 + s, 226 + j, 2, 1, chalk);
  }
  ellipse(384, 226, 2, 2, chalk);
  for (int i = 0; i < 2; i++) {
    box(i ? 646 : 39, 156, 84, 141, chalk);
    box(i ? 701 : 39, 189, 29, 74, chalk);
    ellipse(i ? 673 : 95, 226, 2, 2, chalk);
    int x = i ? 730 : 24;
    rect(x, 192, 15, 69, 0x529888);
    for (int j = 1; j < 15; j += 4)
      rect(x + j, 192, 1, 69, 0xa0c7ae);
    for (int j = 0; j < 69; j += 5)
      rect(x, 192 + j, 15, 1, 0xa0c7ae);
    box(x, 191, 16, 71, WHITE);
  }
  for (int j = 0; j < 10; j++)
    ellipse(p[j].x + 2, p[j].y + 1, 8, 3, 0x23754e);
  int order[10];
  for (int i = 0; i < 10; i++) {
    int k = i;
    while (k > 0 && p[order[k - 1]].y > p[i].y) {
      order[k] = order[k - 1];
      k--;
    }
    order[k] = i;
  }
  for (int k = 0; k < 10; k++) {
    int i = order[k];
    if (i == selected) {
      ellipse(p[i].x, p[i].y + 2, 10, 3, LIME);
      rect(p[i].x - 4, p[i].y - 30, 9, 2, LIME);
      rect(p[i].x - 2, p[i].y - 28, 5, 2, LIME);
      rect(p[i].x, p[i].y - 26, 1, 1, LIME);
    }
    athlete(p[i].x, p[i].y, 1, i >= 5, i, (int)(p[i].walk / 5) % 2);
  }
  football(bx, by, 3);
  rect(0, 0, W, 44, NAVY);
  rect(0, 42, W, 2, INK);
  rect(12, 9, 3, 24, LIME);
  text(22, 12, "SPORT FOOT", 2, WHITE, 1);
  text(23, 31, "ARCADE CLUB  /  01", 1, MUTED, 0);
  badge(211, 11, BLUE);
  text(242, 16, "DUN", 2, BLUE, 0);
  rect(292, 6, 67, 31, INK);
  char s[8];
  s[0] = '0' + score[0] % 10;
  s[1] = ':';
  s[2] = '0' + score[1] % 10;
  s[3] = 0;
  centered(325, 14, s, 2, WHITE);
  text(370, 16, "VOL", 2, RED, 0);
  badge(411, 11, RED);
  text(525, 12, half == 1 ? "MI-TEMPS 1/2" : "MI-TEMPS 2/2", 1, MUTED, 0);
  clockText(s, (int)(120 - remaining));
  text(525, 24, s, 2, WHITE, 0);
  rect(705, 6, 48, 31, INK);
  rect(722, 15, 4, 13, LIME);
  rect(730, 15, 4, 13, LIME);
  if (!mobile) {
    text(21, 394, "FLECHES / ZQSD", 1, LIME, 0);
    text(21, 407, "DEPLACEMENT", 1, MUTED, 0);
    text(192, 394, "X / ESPACE", 1, LIME, 0);
    text(192, 407, "FRAPPE", 1, MUTED, 0);
    text(346, 394, "C", 1, LIME, 0);
    text(346, 407, "PASSE / CHANGER", 1, MUTED, 0);
    text(505, 394, "MAJ", 1, LIME, 0);
    text(505, 407, "SPRINT", 1, MUTED, 0);
    text(655, 394, "ECHAP", 1, LIME, 0);
    text(655, 407, "PAUSE", 1, MUTED, 0);
  } else {
    rect(14, 383, 245, 42, NAVY);
    centered(136, 398, "GLISSER POUR COURIR", 1, WHITE);
    rect(539, 383, 106, 42, BLUE);
    centered(592, 397, "PASSE", 2, INK);
    rect(652, 383, 102, 42, LIME);
    centered(703, 397, "TIR", 2, INK);
  }
}
static void panel(const char *title, const char *sub, const char *bottom) {
  for (int i = 0; i < W * H; i++)
    pixels[i] = ((pixels[i] & 0xfefefe) >> 1) + 0x080d17;
  rect(202, 144, 364, 132, INK);
  rect(202, 144, 364, 3, LIME);
  centered(384, 161, sub, 1, MUTED);
  centered(384, 187, title, 3, WHITE);
  centered(384, 232, bottom, 1, LIME);
  if (mode == PAUSE)
    centered(384, 254, "RETOUR AU MENU", 1, MUTED);
}
void draw(void) {
  if (mode == MENU) {
    cover();
    return;
  }
  if (mode == HELP) {
    cover();
    rect(30, 52, 708, 330, INK);
    rect(30, 52, 708, 3, LIME);
    text(61, 85, "COMMANDES", 4, WHITE, 1);
    text(62, 140, "OBJECTIF / MARQUER PLUS DE BUTS EN DEUX MINUTES.", 2, LIME,
         0);
    text(62, 185,
         mobile ? "GLISSER A GAUCHE : DIRIGER LE JOUEUR"
                : "FLECHES / ZQSD / WASD : DEPLACER",
         2, WHITE, 0);
    text(62, 216,
         mobile ? "PASSE : PASSER OU CHANGER DE JOUEUR"
                : "C : PASSE OU CHANGEMENT DE JOUEUR",
         2, WHITE, 0);
    text(62, 247,
         mobile ? "TIR : FRAPPER VERS LE BUT"
                : "ESPACE / X : FRAPPER     MAJ : SPRINT",
         2, WHITE, 0);
    text(62, 287, "LES DUNES EN BLEU ATTAQUENT VERS LA DROITE.", 1, MUTED, 0);
    text(62, 335, mobile ? "TOUCHER POUR REVENIR" : "ENTREE / ECHAP : RETOUR",
         2, LIME, 0);
    return;
  }
  pitch();
  if (mode == GOAL)
    panel("BUUUT !", lastGoal ? "VOLCANS FC" : "LES DUNES FC", "ENGAGEMENT...");
  if (mode == PAUSE)
    panel("PAUSE", "SPORT FOOT  /  MATCH RAPIDE", "REPRENDRE  >");
  if (mode == END)
    panel(score[0] > score[1]   ? "VICTOIRE"
          : score[0] < score[1] ? "DEFAITE"
                                : "MATCH NUL",
          "COUP DE SIFFLET FINAL", "REJOUER  >");
}
