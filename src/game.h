#ifndef SPORT_FOOT_H
#define SPORT_FOOT_H
#define W 768
#define H 432
extern unsigned int pixels[W * H];
extern float axisX, axisY;
extern int mode, mobile, sprint;
enum { MENU, PLAY, PAUSE, GOAL, END, HELP };
enum { START, PASS, SHOT, SWITCH, BACK };
void init(void);
void tick(float dt);
void draw(void);
void action(int a);
void tap(int x, int y);
#endif
