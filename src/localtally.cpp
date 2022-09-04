#include "main.h"

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(256, PixelPin);


unsigned long tallyLast = 0;
static bool tallyCleared = false;
static unsigned long statusLast = 0;

static uint8_t tallyState[TALLY_MAX_NUM];
static uint8_t tallyBrightness[TALLY_MAX_NUM];
static bool tally_changed = false;
static char tallyText[100];

void localtally_setup() {
    memset(tallyState, 0, sizeof(tallyState));
    strip.Begin();
}

void sendStatus(void) {
    DynamicJsonDocument json(128);
    String output;
    json["cmd"] = "STATUS";
    json["version"] = VERSION_STR "-" PLATFORM_STR "-" BUILD_STR;
    json["uptime"] = millis()/1000;
    json["battVolt"] = BATTVOLT();
    json["LoRaMsgCnt"] = LoRaGetMsgCnt();
    json["LoRaRSSI"] = LoRaGetRSSI();

    serializeJson(json, output);
    ws2All(output.c_str());
}

void sendTally(void) {
    DynamicJsonDocument json(128);
    String output;
    uint8_t stateRH, brightnessRH;
    uint8_t stateLH, brightnessLH;
    if (getTallyState(cfg.tally_id|TALLY_RH, stateRH, brightnessRH) && getTallyState(cfg.tally_id|TALLY_LH, stateLH, brightnessLH)) {
        json["cmd"] = "TALLY";
        json["stateLH"] = stateLH;
        json["brightnessLH"] = brightnessLH;
        json["stateRH"] = stateRH;
        json["brightnessRH"] = brightnessRH;
        json["text"] = tallyText;
        serializeJson(json, output);
        ws2All(output.c_str());
    }
}

void rgbFromTSL(int &r, int &g, int &b, int state, int brightness, bool atem) {
    brightness++;
    r = g = b = 0;
    if (atem && state == 3) {
        state = 1;
    }
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

bool setTallyState(int index, uint8_t state, uint8_t brightness, char *text) {
    if (index < TALLY_MAX_NUM) {
        if (index == cfg.tally_id) {
            if (text) {
                strncpy(tallyText, text, sizeof(tallyText) - 1);
            } else {
                tallyText[0] = 0;
            }
        }
        if (tallyState[index - 1] != state ||
            tallyBrightness[index - 1] != brightness) {
            tallyState[index - 1] = state;
            tallyBrightness[index - 1] = brightness;
            tally_changed = true;           
        }
    }
    return tally_changed;
}

bool getTallyState(int index, uint8_t &state, uint8_t &brightness) {
    if (index >=0 && index < TALLY_MAX_NUM) {
        state = tallyState[index - 1];
        brightness = tallyBrightness[index - 1];
        return true;
    }
    return false;
}

bool getTallyChanged(bool clear) {
    bool val = tally_changed;
    if (clear) {
        tally_changed = false;
    }
    return val;
}

void setTallyLight(int r, int g, int b, dispMode_t disp, int pixel,
                   char *text) {
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
            strip.SetPixelColor(pixel - 1, RgbColor(r, g, b));
    }
    strip.Show();
    if (disp > DISP_OFF) {
        if (text != NULL && strlen(text)) {
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.setFont(ArialMT_Plain_24);
            display.drawString(64, 20, text);
        } else if (r | g | b) {
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.setFont(ArialMT_Plain_24);
            display.drawString(64, 4, "Tally " + String(cfg.tally_id));
        }
        /*
                    display.drawString(64, 14, "Red  : " + String(r));
                    display.drawString(64, 24, "Green: " + String(g));
                    display.drawString(64, 34, "Blue : " + String(b));
        */
        if (disp == DISP_RSSI) {
            display.setFont(ArialMT_Plain_10);
            display.drawString(64, 44, "RSSI : " + String(RSSIlast));
        }
        d();
    }
#endif
}

void setTallyLight(int tally_id, dispMode_t disp, int pixel, char *text) {
    uint8_t state, brightness;
    if (pixel < 2) {
        tally_id |= TALLY_RH;
    } else {
        tally_id |= TALLY_LH;
    }
    if (getTallyState(tally_id, state, brightness)) {
        int r, g, b;
        rgbFromTSL(r, g, b, state, brightness);
        setTallyLight(r, g, b, disp, pixel, text);
    }
}

void tally_loop() {
    if (getTallyChanged()) {
        if(cfg.tally_id > 0 && cfg.tally_id < (TALLY_MAX_NUM/2)) {
            setTallyLight(cfg.tally_id, DISP_ON, 1, tallyText);
            setTallyLight(cfg.tally_id, DISP_OFF, 2);
            sendTally();
        }
    }

    if (cfg.tally_timeout > 1000 && (millis() - tallyLast > cfg.tally_timeout) && !tallyCleared) {
        setTallyLight(0, 0, 0, DISP_OFF);
        tallyCleared = true;
    }
    if ((millis() - statusLast) > 1000) {
        statusLast = millis();
        sendStatus();
    }
}
