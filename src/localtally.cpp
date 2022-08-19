#include "main.h"

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(256, PixelPin);

uint8_t tallyState[TALLY_MAX_NUM];
unsigned long tallyLast = 0;
static bool tallyCleared = false;

void localtally_setup() {
        strip.Begin();
}

void rgbFromTSL(int &r, int &g, int &b, int brightness, int state) {
    brightness++;
    r = g = b = 0;
    switch (state) {
        case 1:
            r = 63 * brightness;
            break;
        case 2:
            g = 63 * brightness;
            break;
        case 3:
            r = 63 * brightness;
            g = 22 * brightness;
            break;
    }
}

void setTallyState(int index, int state) {
    if (index < TALLY_MAX_NUM) {
        tallyState[index - 1] = state;
    }
}

void setTallyLight(int r, int g, int b, bool disp, int pixel, char *text) {
    tallyLast = millis();

    if (r || g || b) {
        tallyCleared = false;
    }

#ifdef HELTEC
    r = r * cfg.led_max_brightness / 255;
    g = g * cfg.led_max_brightness / 255;
    b = b * cfg.led_max_brightness / 255;

    if (pixel == 0) {
        for (int i = 0; i < cfg.num_pixels; i++) {
            strip.SetPixelColor(i, RgbColor(r, g, b));
        }
    } else {
        if (pixel <= cfg.num_pixels)
            strip.SetPixelColor(pixel, RgbColor(r, g, b));
    }
    strip.Show();
    if (disp) {
        if (text != NULL && strlen(text)) {
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.setFont(ArialMT_Plain_24);
            display.drawString(64, 20, text);
            d();
        } else if (r | g | b) {
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.setFont(ArialMT_Plain_10);
            display.drawString(64, 4, "Addr : " + String(cfg.tally_id));
            display.drawString(64, 14, "Red  : " + String(r));
            display.drawString(64, 24, "Green: " + String(g));
            display.drawString(64, 34, "Blue : " + String(b));
            display.drawString(64, 44, "RSSI : " + String(RSSIlast));
            d();
        }
    }
#endif
}
void tally_loop() {
    if ((millis() - tallyLast > cfg.tally_timeout) && !tallyCleared) {
        setTallyLight(0, 0, 0, false);
        tallyCleared = true;
    }
}

