#include "main.h"

SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
static unsigned long displayTime = millis();
static unsigned long displayCleared = millis();

void display_setup() {
    display.init();
    display.flipScreenVertically();
}

void d() {
#ifdef HELTEC
    displayTime = millis();
    displayCleared = 0;
    display.display();
#endif
}

void display_loop() {
#ifdef HELTEC
    if (cfg.display_timeout > 1000 && (millis() - displayTime > cfg.display_timeout) && !displayCleared) {
        displayCleared = millis();
        display.clear();
        display.display();
    }
#endif
}
