#ifndef _MAIN_H
#define _MAIN_H
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <esp_adc_cal.h>


#include <EEPROM.h>
#include <ESPmDNS.h>
#include <EOTAUpdate.h>
#include <Ewma.h>

/*
 * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
 * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
 * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
 * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
 */
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

#ifdef LILYGO_POE
// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN 16
#else
#define ETH_POWER_PIN -1
#endif

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR 0

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN 23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN 18
#define NRST        5

#include <ETH.h>

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
#include "modbus.h"

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
#define OLED_ADDRESS 0x3c

/*
#define OLED_SDA 4  
#define OLED_SCL 15 
#define OLED_RST 16 
#define GPIO_BUTTON 0
#define PixelPin 23
#define GPIO_BATTERY GPIO_NUM_35
*/
#ifdef HELTEC
    #define HAS_BATTERY
    #define HAS_DISPLAY
    #define HAS_DISPLAY_UPSIDEDOWN
    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 4
    #define OLED_SCL 15 
    #define OLED_RST 16 

    #define GPIO_BUTTON GPIO_NUM_0
    #define GPIO_BATTERY GPIO_NUM_37

    #define HAS_LORA
    #define LoRa_SCK  5 //SCK   // GPIO 5
    #define LoRa_MISO 19 //MISO  // GPIO 19
    #define LoRa_MOSI 27 //MOSI  // GPIO 27
    #define LoRa_RST  14 //RST_LoRa  // GPIO 14
    #define LoRa_CS   18 //SS   // GPIO 18
    #define LoRa_DIO0 26 //DIO0 // GPIO 26
    #define LoRa DIO1 35
    #define LoRa DIO2 34

    #define ADCR1     100.0
    #define ADCR2     220.0
    #define BATTCELLS 1

    #define Vext      21

    #define HAS_PIXEL
    #define PixelPin  23 // make sure to set this to the correct pin, ignored for Esp8266
#endif
#ifdef HELTECV3
    #define HAS_BATTERY
    #define HAS_DISPLAY
    #define HAS_DISPLAY_UPSIDEDOWN
    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 17
    #define OLED_SCL 18
    #define OLED_RST 21

    #define GPIO_BUTTON GPIO_NUM_0
    #define GPIO_BATTERY GPIO_NUM_2

    #define HAS_LORA
    #define LoRa_SCK  9
    #define LoRa_MISO 11
    #define LoRa_MOSI 10
    #define LoRa_RST  12
    #define LoRa_CS   8 //SS
    #define LoRa_DIO1 14
    #define LoRa_BUSY 13

    #define Vext      36
    #define ADCctrl   37
//    #define ADCR1     100.0
//    #define ADCR2     390.0
    #define ADCR1     240.0
    #define ADCR2     634.0
    #define BATTCELLS 2

    #define HAS_PIXEL
    #define PixelPin  4
#endif
#ifdef OLIMEX_POE_ISO 
    #define HAS_ETHERNET

    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 36
    #define OLED_SCL 36 
    #define OLED_RST 36 

    #define GPIO_BUTTON GPIO_NUM_34
    #define GPIO_BATTERY GPIO_NUM_35

    #define ADCR1     470.0
    #define ADCR2     470.0
    #define BATTCELLS 1

    #define HAS_LORA
    #define LoRa_SCK  13
    #define LoRa_MISO  4
    #define LoRa_MOSI 16
    #define LoRa_RST  14  
    #define LoRa_CS    2  
    #define LoRa_DIO0  5 

    #define Vext      12

    #define PixelPin  23

    #define MODBUS_TX 32
    #define MODBUS_RX 33  
#endif
#ifdef OLIMEX_POE
    #define HAS_ETHERNET

    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 36
    #define OLED_SCL 36
    #define OLED_RST 36

    #define GPIO_BUTTON GPIO_NUM_34
    #define GPIO_BATTERY GPIO_NUM_35

    #define Vext      12
    #define ADCR1     470.0
    #define ADCR2     470.0
    #define BATTCELLS 1

    #define HAS_LORA
    #define LoRa_SCK  13
    #define LoRa_MISO  4
    #define LoRa_MOSI 16
    #define LoRa_RST  14
    #define LoRa_CS    2
    #define LoRa_DIO0 36


    #define HAS_PIXEL
    #define PixelPin  5

    #define MODBUS_TX 32
    #define MODBUS_RX 33  
#endif
#ifdef LILYGO_POE
    #define HAS_ETHERNET

    #define OLED_ADDRESS 0x3c
    #define OLED_SDA 32
    #define OLED_SCL 33
    #define OLED_RST 34

    #define GPIO_BUTTON GPIO_NUM_0
    #define GPIO_BATTERY -1

    #define HAS_LORA
    #define LoRa_SCK  14
    #define LoRa_MISO  2
    #define LoRa_MOSI 15
    #define LoRa_RST  12
    #define LoRa_CS    4
    #define LoRa_DIO0  16

    #define Vext      -1

    #define PixelPin  32
#endif

#define BUTTON_PIN_BITMASK (1LL<<GPIO_BUTTON)

extern const uint8_t data_index_html_start[] asm("_binary_data_index_html_start");
extern const uint8_t data_index_html_end[] asm("_binary_data_index_html_end");
extern const uint8_t data_script_js_start[] asm("_binary_data_script_js_start");
extern const uint8_t data_script_js_end[] asm("_binary_data_script_js_end");

extern bool eth_connected;
extern bool heltec;

uint8_t battVoltToPercent(float mvolts);
float get_batt_volt(void);
void power_off(int state);
#endif