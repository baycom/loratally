#ifndef LOCALTALLY_H
#define LOCALTALLY_H

#include <Wire.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#define TALLY_MAX_NUM 32
#define LED_MAX_BRIGHNESS 192

extern uint8_t tallyState[TALLY_MAX_NUM];
extern unsigned long tallyLast;

void localtally_setup();
void rgbFromTSL(int &r, int &g, int &b, int brightness, int state);
void setTallyState(int index, int state);
void setTallyLight(int r, int g, int b, bool disp = true, int pixel = 0, char *text = NULL);
void tally_loop();

#endif