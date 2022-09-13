#ifndef _MAIN_H
#define _MAIN_H
#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include <EEPROM.h>
#include <ESPmDNS.h>
#include <EOTAUpdate.h>

#include "ATEMbase.h"
#include "ATEMext.h"

#include "version.h"
#include "atem.h"
#include "buttons.h"
#include "cfg.h"
#include "display.h"
#include "e131.h"
#include "localtally.h"
#include "loratally.h"
#include "mqtt.h"
#include "TSLReceiver.h"
#include "webserver.h"

#ifdef DEBUG
  #define dbg(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define err(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define info(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define warn(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
#else
  #define dbg(format, arg...) do {} while (0)
  #define err(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define info(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
  #define warn(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg);}
#endif 

#define BAND 868E6
#define BATTVOLT() (analogReadMilliVolts(GPIO_BATTERY)*(100.0+220.0)/100.0)

#ifdef HELTEC
    #define HAS_DISPLAY
    #define HAS_PIXEL
    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 4  
    #define OLED_SCL 15 
    #define OLED_RST 16 

    #define GPIO_BUTTON GPIO_NUM_0
    #define GPIO_BATTERY GPIO_NUM_37

    #define LoRa_SCK  5 //SCK   // GPIO 5
    #define LoRa_MISO 19 //MISO  // GPIO 19
    #define LoRa_MOSI 27 //MOSI  // GPIO 27
    #define LoRa_RST  14 //RST_LoRa  // GPIO 14
    #define LoRa_CS   18 //SS   // GPIO 18
    #define LoRa_DIO0 26 //DIO0 // GPIO 26
    #define Vext      21
    #define PixelPin  23 // make sure to set this to the correct pin, ignored for Esp8266
#endif
#ifdef OLIMEX_POE_ISO 
    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 36
    #define OLED_SCL 36 
    #define OLED_RST 36 

    #define GPIO_BUTTON GPIO_NUM_34
    #define GPIO_BATTERY GPIO_NUM_35

    #define LoRa_SCK  13
    #define LoRa_MISO  4
    #define LoRa_MOSI 16
    #define LoRa_RST  14  
    #define LoRa_CS    2  
    #define LoRa_DIO0  5 
    #define Vext      12
    #define PixelPin  23  
#endif
#ifdef OLIMEX_POE
    #define HAS_PIXEL
    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 36
    #define OLED_SCL 36
    #define OLED_RST 36

    #define GPIO_BUTTON GPIO_NUM_34
    #define GPIO_BATTERY GPIO_NUM_35

    #define LoRa_SCK  13
    #define LoRa_MISO  4
    #define LoRa_MOSI 16
    #define LoRa_RST  14
    #define LoRa_CS    2
    #define LoRa_DIO0 36
    #define Vext      12
    #define PixelPin  5
#endif

#define BUTTON_PIN_BITMASK (1LL<<GPIO_BUTTON)

extern const uint8_t data_index_html_start[] asm("_binary_data_index_html_start");
extern const uint8_t data_index_html_end[] asm("_binary_data_index_html_end");
extern const uint8_t data_script_js_start[] asm("_binary_data_script_js_start");
extern const uint8_t data_script_js_end[] asm("_binary_data_script_js_end");

extern bool eth_connected;
extern bool heltec;
void power_off(int state);
#endif