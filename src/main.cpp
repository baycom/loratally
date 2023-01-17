#include "main.h"

bool eth_connected = false;
bool disable_poweroff = false;

static EOTAUpdate *updater;

const int lipocurve[21][2]={ {0, 3270}, {5, 3610}, {10, 3690}, {15, 3710}, {20, 3730}, {25, 3750}, {30, 3770}, {35, 3790}, {40, 3800}, {45, 3820},
                            {50, 3840}, {55, 3850}, {60, 3870}, {65, 3910}, {70, 3950}, {75, 3980}, {80, 4020}, {85, 4080}, {90, 4110}, {95, 4150}, {100, 4200}};

Ewma *voltageFilter;

float battVolt(bool live) {
  float mvolts = 300.0+analogReadMilliVolts(GPIO_BATTERY)*(100.0+220.0)/100.0;
  if(live) {
    return mvolts;
  }
  return voltageFilter->filter(mvolts);
}

uint8_t battVoltToPercent(float mvolts) {
    if(mvolts<3270)
        return 0;
    int i;
    for(i=0; i<21; i++) {
        float v = lipocurve[i][1];
        if((mvolts-v)<0) {
            i--;
            break;
        }
    }
    if(i<0) {
        return 0;
    }
    if(i<20) {
        float vdiff = lipocurve[i+1][1] - lipocurve[i][1];
        float mdiff = mvolts - lipocurve[i][1];
        float perc = mdiff / vdiff;
        float pdiff = lipocurve[i+1][0] - lipocurve[i][0];
        float ret = pdiff * perc + lipocurve[i][0];
        dbg("pos: %d mvolts: %.3f vdiff: %.3f mdiff %.3f perc: %.3f pdiff: %.3f ret: %.3f\n", i, mvolts, vdiff, mdiff, perc, pdiff, ret);
        return ret;
    }
    return 100;
}

static void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            dbg("Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            dbg("Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            dbg("Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            dbg("Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            dbg("Wakeup caused by ULP program");
            break;
        default:
            dbg("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
    }
    if (wakeup_reason) {
        uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
        dbg("GPIO that triggered the wake up: GPIO %d\n",
            log(GPIO_reason) / log(2), 0);
    }
}

void WiFiEvent(WiFiEvent_t event) {
    dbg("WiFiEvent: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            dbg("ETH Started\n");
            // set eth hostname here
            ETH.setHostname(cfg.wifi_hostname);
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            dbg("ETH Connected\n");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            info("ETH MAC: %s, IPv4: %s (%s, %dMbps)\n",
                 ETH.macAddress().c_str(), ETH.localIP().toString().c_str(),
                 ETH.fullDuplex() ? "FULL_DUPLEX" : "HALF_DUPLEX",
                 ETH.linkSpeed());
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            if (!eth_connected) {
                if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                    info("WiFi MAC: %s, IPv4: %s\n", WiFi.macAddress().c_str(),
                         WiFi.localIP().toString().c_str());
                }
#ifdef HAS_DISPLAY
                display.setFont(ArialMT_Plain_10);
                display.drawString(64, 54, "IP: " + WiFi.localIP().toString());
                d();
#endif
                if (!MDNS.begin(cfg.wifi_hostname)) {
                    err("MDNS responder failed to start\n");
                }
                e131_setup();
                mqtt_setup();
                atem_setup();
                tsl_setup(cfg.tally_screen,cfg.tally_id,cfg.tsl_host, cfg.tsl_port);
                eth_connected = true;
                setTallyLight(0, 0, 0, DISP_OFF);
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            dbg("ETH Disconnected\n");
            //            eth_connected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            dbg("ETH Stopped\n");
            //            eth_connected = false;
            break;
        default:
            break;
    }
}

void setup() {
    randomSeed(analogRead(GPIO_BATTERY));
    Serial.begin(115200);
    info("Version: %s-%s-%s, Version Number: %d, CFG Number: %d\n", VERSION_STR,
         PLATFORM_STR, BUILD_STR, VERSION_NUMBER, cfg_ver_num);
    info("Initializing ... ");
    voltageFilter = new Ewma(0.1, battVolt(true));
    config_setup();
    read_config();

    WiFi.onEvent(WiFiEvent);
    buttons_setup();
#ifdef HELTEC
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);
#endif
    localtally_setup();
    display_setup();
    lora_setup();

    setTallyLight(32, 0, 0, DISP_OFF);

    //  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ALL_LOW);
    print_wakeup_reason();
#ifdef HAS_DISPLAY
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.clear();
    display.drawString(64, 4, "LoRa-Tally-Receiver");
    display.drawString(64, 14, "Version: " + String(VERSION_STR));
    String modeStr = "";
    switch (cfg.wifi_opmode) {
        case 0:
            modeStr = "AP";
            break;
        case 1:
            modeStr = "STA";
            break;
        case 2:
            modeStr = "ETH";
            break;
    }
    display.drawString(64, 24, "Mode: " + modeStr);
    if (cfg.wifi_opmode < 2) {
        display.drawString(64, 34, "SSID: " + String(cfg.wifi_ssid));
    }
    display.drawString(64, 44, "NAME: " + String(cfg.wifi_hostname));
    d();
#endif
    setTallyLight(0, 32, 0, DISP_OFF);

    updater = new EOTAUpdate(cfg.ota_path, VERSION_NUMBER);

    if (cfg.wifi_opmode == OPMODE_ETH_CLIENT) {
    #ifdef LILYGO_POE    
        pinMode(NRST, OUTPUT);
        for(int i=0;i<4;i++) {
            digitalWrite(NRST, i&1);
            delay(200);
        }
    #endif

    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN,
              ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
    } else if (cfg.wifi_opmode == OPMODE_WIFI_STATION) {
        WiFi.disconnect();
        WiFi.setAutoReconnect(true);
        WiFi.setHostname(cfg.wifi_hostname);
        WiFi.setSleep(cfg.wifi_powersave);
        WiFi.mode(WIFI_STA);

        IPAddress myIP;
        IPAddress myGW;
        IPAddress myNM;
        IPAddress myDNS;

        myIP.fromString(cfg.ip_addr);
        myGW.fromString(cfg.ip_gw);
        myNM.fromString(cfg.ip_netmask);
        myDNS.fromString(cfg.ip_dns);

        WiFi.config(myIP, myGW, myNM, myDNS);
        WiFi.begin(cfg.wifi_ssid, cfg.wifi_secret);

        info("\n");
    } else if (cfg.wifi_opmode == OPMODE_WIFI_ACCESSPOINT) {
        WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
        IPAddress IP = WiFi.softAPIP();
        info("AP IP address: %s\n", IP.toString().c_str());
#ifdef HAS_DISPLAY        
        display.setFont(ArialMT_Plain_10);
        for (int x = 0; x < 128; x++) {
            for (int y = 0; y < 20; y++) {
                display.clearPixel(x, y + 24);
            }
        }
        display.drawString(
            64, 24,
            "WIFI: " + String((cfg.wifi_opmode == OPMODE_WIFI_STATION) ? "STA"
                                                                       : "AP"));
        display.drawString(64, 34, "SSID: " + String(cfg.wifi_ssid));
        display.drawString(64, 54, "IP: " + IP.toString());
        d();
#endif        
    }
    webserver_setup();

    setTallyLight(0, 0, 0, DISP_OFF);
}

void power_off(int state) {
#ifdef HAS_DISPLAY    
    if (state & 1) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 32, "Power off.");
        d();
    }
#endif
    if (state & 2) {
        setTallyLight(0, 0, 32, DISP_OFF);
        sleep(2);
        setTallyLight(0, 0, 0, DISP_OFF);
#ifdef HAS_DISPLAY
        display.clear();
        digitalWrite(OLED_RST, LOW);  // low to reset OLED
#endif
#ifdef HELTEC
        digitalWrite(Vext, HIGH);
#endif
        lora_shutdown();
        esp_deep_sleep_start();
    }
}

void wifi_loop() {
    static unsigned long last_blink = 0;
    static int count = 0;

    if (!eth_connected && millis() < 30000) {
        if (cfg.wifi_opmode == OPMODE_WIFI_STATION &&
            cfg.wifi_ap_fallback == 1 && WiFi.status() != WL_CONNECTED) {
            uint8_t mac[10];
            WiFi.macAddress(mac);
            sprintf(cfg.wifi_ssid, "SIP-%02X%02X%02X", mac[3], mac[4], mac[5]);
            warn("\nFailed to connect to SSID %s falling back to AP mode\n",
                 cfg.wifi_ssid);
            cfg.wifi_secret[0] = 0;
            cfg.wifi_opmode = OPMODE_WIFI_ACCESSPOINT;
            WiFi.disconnect();
            WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
            IPAddress IP = WiFi.softAPIP();
            info("AP IP address: %s\n", IP.toString().c_str());
            disable_poweroff = true;
        }
        if ((millis() - last_blink) > 500) {
            last_blink = millis();
            setTallyLight(0, 0, 32 * !(count & 1), DISP_OFF);
            count++;
            info("%d\n", count);
        }
    }
}

void loop() {
    wifi_loop();
    display_loop();
    buttons_loop();
    lora_loop();
    e131dmx_loop();
    mqtt_loop();
    atem_loop();
    tsl_loop();
    tally_loop();

    if (cfg.ota_path[0] && WiFi.getMode() == WIFI_MODE_STA &&
        WiFi.status() == WL_CONNECTED) {
        updater->CheckAndUpdate();
    }
    if (!disable_poweroff && cfg.inactivity_timeout > 30000 && tallyLast &&
        (millis() - tallyLast) > cfg.inactivity_timeout) {
        power_off(3);
    }
}
