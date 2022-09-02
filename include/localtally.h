#ifndef LOCALTALLY_H
#define LOCALTALLY_H

#include <NeoPixelAnimator.h>
#include <NeoPixelBus.h>
#include <Wire.h>

#define TALLY_MAX_NUM 64
#define LED_MAX_BRIGHNESS 192
#define TALLY_LH 0
#define TALLY_RH 32
typedef enum {
    DISP_OFF = 0,
    DISP_ON = 1,
    DISP_RSSI = 2
} dispMode_t;

// extern uint8_t tallyState[TALLY_MAX_NUM];
// extern uint8_t tallyBrightness[TALLY_MAX_NUM];
extern unsigned long tallyLast;

void localtally_setup();
void rgbFromTSL(int &r, int &g, int &b, int state, int brightness = 3,
                bool atem = false);
bool setTallyState(int index, uint8_t state, uint8_t brightness = 3,
                   char *text = NULL);
bool getTallyState(int index, uint8_t &state, uint8_t &brightness);
bool getTallyChanged(bool clear = true);
void setTallyLight(int r, int g, int b, dispMode_t disp = DISP_ON, int pixel = 0,
                   char *text = NULL);
void setTallyLight(int tally_id, dispMode_t disp = DISP_ON, int pixel = 0,
                   char *text = NULL);

void tally_loop();

#endif