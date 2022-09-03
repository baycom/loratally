#include "main.h"

bool eth_connected = false;

#ifdef HELTEC
bool heltec = true;
#else
bool heltec = false;
#endif

static unsigned long lastReconnect = 0;

static EOTAUpdate *updater;

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
            dbg("Wakeup was not caused by deep sleep: %d\n",
                          wakeup_reason);
            break;
    }
    if (wakeup_reason) {
        uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
        Serial.print("GPIO that triggered the wake up: GPIO ");
        Serial.println((log(GPIO_reason)) / log(2), 0);
    }
}

void WiFiEvent(WiFiEvent_t event) {
    dbg("WiFiEvent: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH Started");
            // set eth hostname here
            ETH.setHostname(cfg.wifi_hostname);
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("ETH Connected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("ETH MAC: ");
            Serial.print(ETH.macAddress());
            Serial.print(", IPv4: ");
            Serial.print(ETH.localIP());
            if (ETH.fullDuplex()) {
                Serial.print(", FULL_DUPLEX");
            }
            Serial.print(", ");
            Serial.print(ETH.linkSpeed());
            Serial.println("Mbps");
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            if (!eth_connected) {
                if (!MDNS.begin(cfg.wifi_hostname)) {
                    err("MDNS responder failed to start\n");
                }
                e131_setup();
                mqtt_setup();
                atem_setup();
                tsl_setup(cfg.tsl_port);
                eth_connected = true;
            }
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH Disconnected");
            eth_connected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("ETH Stopped");
            eth_connected = false;
            break;
        default:
            break;
    }
}

void setup() {
    randomSeed(analogRead(5));
    Serial.begin(115200);
    info("Version: %s-%s-%s, Version Number: %d, CFG Number: %d\n",
           VERSION_STR, PLATFORM_STR, BUILD_STR, VERSION_NUMBER, cfg_ver_num);
    info("Initializing ... ");

    EEPROM.begin(EEPROM_SIZE);
    read_config();

    WiFi.onEvent(WiFiEvent);
    buttons_setup();

#ifdef HELTEC
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);

    localtally_setup();

    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);  // low to reset OLED
    delay(50);
    digitalWrite(OLED_RST, HIGH);  // must be high to turn on OLED
#endif
    display_setup();

    setTallyLight(32, 0, 0, DISP_OFF);

    //  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ALL_LOW);
    print_wakeup_reason();
#ifdef HELTEC
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
    display.drawString(64, 34, "SSID: " + String(cfg.wifi_ssid));
    display.drawString(64, 44, "NAME: " + String(cfg.wifi_hostname));
    d();
    setTallyLight(0, 32, 0, DISP_OFF);
#endif

    updater = new EOTAUpdate(cfg.ota_path, VERSION_NUMBER);

    lora_setup();

    if (cfg.wifi_opmode == OPMODE_ETH_CLIENT) {
        ETH.begin();
    } else {
        if (cfg.wifi_opmode == OPMODE_WIFI_STATION) {
            WiFi.disconnect();
            WiFi.setHostname(cfg.wifi_hostname);
            WiFi.setSleep(cfg.wifi_powersave);
            WiFi.mode(WIFI_STA);
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
            WiFi.begin(cfg.wifi_ssid, cfg.wifi_secret);

            info("\n");

            unsigned long lastConnect = millis();
            int count = 0;

            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                setTallyLight(32 * (count & 1), 0, 32 * !(count & 1), DISP_OFF);
                count++;
                info("%d\n", count);
                if (((millis() - lastConnect) > 10000) &&
                    cfg.wifi_ap_fallback) {
                    if (cfg.wifi_ap_fallback < 2) {
                        cfg.wifi_opmode = OPMODE_WIFI_ACCESSPOINT;
                    }
                    break;
                }
            }
            if (cfg.wifi_opmode == OPMODE_WIFI_STATION && count < 21) {
                info("\n");
                info("Connected to %s\n", cfg.wifi_ssid);
                info("STA IP address: %s\n",
                       WiFi.localIP().toString().c_str());

                display.setFont(ArialMT_Plain_10);
                display.drawString(64, 54, "IP: " + WiFi.localIP().toString());
                d();
            } else {
                WiFi.disconnect();
                warn(
                    "\nFailed to connect to SSID %s falling back to AP mode\n",
                    cfg.wifi_ssid);
                strcpy(cfg.wifi_ssid, "RPS");
                cfg.wifi_secret[0] = 0;
            }
        }
        if (cfg.wifi_opmode == OPMODE_WIFI_ACCESSPOINT) {
            WiFi.softAP(cfg.wifi_ssid, cfg.wifi_secret);
            IPAddress IP = WiFi.softAPIP();
            info("AP IP address: %s\n", IP.toString().c_str());
            display.setFont(ArialMT_Plain_10);
            for (int x = 0; x < 128; x++) {
                for (int y = 0; y < 20; y++) {
                    display.clearPixel(x, y + 24);
                }
            }
            display.drawString(
                64, 24,
                "WIFI: " + String((cfg.wifi_opmode == OPMODE_WIFI_STATION)
                                      ? "STA"
                                      : "AP"));
            display.drawString(64, 34, "SSID: " + String(cfg.wifi_ssid));
            display.drawString(64, 54, "IP: " + IP.toString());
            d();
        }
    }
    webserver_setup();

    setTallyLight(0, 0, 0, DISP_OFF);
}

static void wifi_loop() {
    if (WiFi.getMode() == WIFI_MODE_STA && WiFi.status() == WL_DISCONNECTED) {
        if (millis() - lastReconnect > 5000) {
            warn("lost WIFI connecion - trying to reconnect\n");
            WiFi.reconnect();
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
            WiFi.setHostname(cfg.wifi_hostname);
            lastReconnect = millis();
        }
    }
}

void power_off(int state) {
    if (state & 1) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 32, "Power off.");
        d();
    }
    if (state & 2) {
        setTallyLight(0, 0, 32, DISP_OFF);
        sleep(2);
        display.clear();
        digitalWrite(OLED_RST, LOW);  // low to reset OLED
        digitalWrite(Vext, HIGH);
        LoRa.sleep();
        esp_deep_sleep_start();
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
    if (cfg.inactivity_timeout && tallyLast &&
        (millis() - tallyLast) > cfg.inactivity_timeout) {
        power_off(3);
    }
}
