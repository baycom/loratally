#include <main.h>

settings_t cfg;

void write_config(void) {
    EEPROM.writeBytes(0, &cfg, sizeof(cfg));
    EEPROM.commit();
}

void read_config(void) {
    EEPROM.readBytes(0, &cfg, sizeof(cfg));
    if (cfg.version != cfg_ver_num) {
        if (cfg.version == 0xff) {
            uint8_t mac[10];
            WiFi.macAddress(mac);
            sprintf(cfg.wifi_ssid, "Tally-%02X%02X%02X", mac[3], mac[4], mac[5]);
            strcpy(cfg.wifi_secret, "");
            strcpy(cfg.wifi_hostname, cfg.wifi_ssid);
            cfg.wifi_opmode = OPMODE_WIFI_ACCESSPOINT;
            cfg.wifi_powersave = true;
            cfg.tx_frequency = BAND;
            cfg.wifi_ap_fallback = 0;
            cfg.tx_power = 20;
            cfg.sf = 7;
            cfg.bandwidth = 125E3;
            cfg.syncword = 0x34;
            cfg.tally_id = -1;
            cfg.num_pixels = 1;
            cfg.tally_timeout = 60000;
            cfg.display_timeout = 5000;
            cfg.led_max_brightness = 192;
            cfg.status_interval = 30000;
            cfg.command_interval = 0;
            cfg.inactivity_timeout = 86400 * 1000;
            cfg.mqtt_host[0] = 0;
            cfg.atem_host[0] = 0;
            cfg.tsl_port = 0;
        }
        if (cfg.ota_path[0] == 0xff) {
            cfg.ota_path[0] = 0;
        }
        cfg.version = cfg_ver_num;
        write_config();
    }
    info("Settings:\n");
    info("cfg version     : %d\n", cfg.version);
    info("ssid            : %s\n", cfg.wifi_ssid);
    info("wifi_secret     : %s\n", cfg.wifi_secret);
    info("wifi_hostname   : %s\n", cfg.wifi_hostname);
    info("wifi_opmode     : %d\n", cfg.wifi_opmode);
    info("wifi_powersave  : %d\n", cfg.wifi_powersave);
    info("wifi_ap_fallback: %d\n", cfg.wifi_ap_fallback);
    info("ota_path        : %s\n", cfg.ota_path);
    info("tx_frequency    : %.6fMhz\n", cfg.tx_frequency / 1E6);
    info("bandwidth       : %.1fkHz\n", cfg.bandwidth / 1E3);
    info("tx_power        : %ddBm\n", cfg.tx_power);
    info("spreading factor: %d\n", cfg.sf);
    info("syncword        : %d\n", cfg.syncword);
    info("tally_id        : %d\n", cfg.tally_id);
    info("num_pixel       : %d\n", cfg.num_pixels);
    info("tally_timeout   : %ldms\n", cfg.tally_timeout);
    info("display_timeout : %ldms\n", cfg.display_timeout);
    info("led_max_brightness: %d\n", cfg.led_max_brightness);
    info("inactivity_timeout: %ldms\n", cfg.inactivity_timeout);
    info("status_interval : %ldms\n", cfg.status_interval);
    info("command_interval: %ldms\n", cfg.command_interval);
    info("MQTT Host       : %s\n", cfg.mqtt_host);
    info("ATEM Host       : %s\n", cfg.atem_host);
    info("TSL Port        : %d\n", cfg.tsl_port);
}

String get_settings(void) {
    DynamicJsonDocument json(1024);
    json["version"] = VERSION_STR;
    json["wifi_hostname"] = cfg.wifi_hostname;
    json["wifi_ssid"] = cfg.wifi_ssid;
    json["wifi_opmode"] = cfg.wifi_opmode;
    json["wifi_ap_fallback"] = cfg.wifi_ap_fallback;
    json["wifi_powersave"] = cfg.wifi_powersave;
    json["wifi_secret"] = cfg.wifi_secret;
    json["ota_path"] = cfg.ota_path;
    json["tx_frequency"] = cfg.tx_frequency;
    json["sf"] = cfg.sf;
    json["bandwidth"] = cfg.bandwidth;
    json["tx_power"] = cfg.tx_power;
    json["syncword"] = cfg.syncword;
    json["tally_id"] = cfg.tally_id;
    json["num_pixels"] = cfg.num_pixels;
    json["tally_timeout"] = cfg.tally_timeout;
    json["display_timeout"] = cfg.display_timeout;
    json["led_max_brightness"] = cfg.led_max_brightness;
    json["status_interval"] = cfg.status_interval;
    json["command_interval"] = cfg.command_interval;
    json["inactivity_timeout"] = cfg.inactivity_timeout;
    json["mqtt_host"] = cfg.mqtt_host;
    json["atem_host"] = cfg.atem_host;
    json["tsl_port"] = cfg.tsl_port;

    String output;
    serializeJson(json, output);
    return output;
}

boolean parse_settings(DynamicJsonDocument json) {
    if (json.containsKey("wifi_hostname"))
        strncpy(cfg.wifi_hostname, json["wifi_hostname"],
                sizeof(cfg.wifi_hostname));
    if (json.containsKey("wifi_ssid"))
        strncpy(cfg.wifi_ssid, json["wifi_ssid"], sizeof(cfg.wifi_ssid));
    if (json.containsKey("wifi_opmode")) cfg.wifi_opmode = json["wifi_opmode"];
    if (json.containsKey("wifi_powersave"))
        cfg.wifi_powersave = json["wifi_powersave"];
    if (json.containsKey("wifi_ap_fallback"))
        cfg.wifi_ap_fallback = json["wifi_ap_fallback"];
    if (json.containsKey("wifi_secret"))
        strcpy(cfg.wifi_secret, json["wifi_secret"]);
    if (json.containsKey("tx_frequency"))
        cfg.tx_frequency = json["tx_frequency"];
    if (json.containsKey("bandwidth")) cfg.bandwidth = json["bandwidth"];
    if (json.containsKey("tx_power")) cfg.tx_power = json["tx_power"];
    if (json.containsKey("sf")) cfg.sf = json["sf"];
    if (json.containsKey("syncword")) cfg.syncword = json["syncword"];
    if (json.containsKey("tally_id")) cfg.tally_id = json["tally_id"];
    if (json.containsKey("num_pixels")) cfg.num_pixels = json["num_pixels"];
    if (json.containsKey("tally_timeout"))
        cfg.tally_timeout = json["tally_timeout"];
    if (json.containsKey("display_timeout"))
        cfg.display_timeout = json["display_timeout"];
    if (json.containsKey("led_max_brightness"))
        cfg.led_max_brightness = json["led_max_brightness"];
    if (json.containsKey("status_interval"))
        cfg.status_interval = json["status_interval"];
    if (json.containsKey("command_interval"))
        cfg.command_interval = json["command_interval"];
    if (json.containsKey("inactivity_timeout"))
        cfg.inactivity_timeout = json["inactivity_timeout"];
    if (json.containsKey("mqtt_host"))
        strncpy(cfg.mqtt_host, json["mqtt_host"], sizeof(cfg.mqtt_host));
    if (json.containsKey("atem_host"))
        strncpy(cfg.atem_host, json["atem_host"], sizeof(cfg.atem_host));
    if (json.containsKey("ota_path"))
        strncpy(cfg.ota_path, json["ota_path"], sizeof(cfg.ota_path));
    if (json.containsKey("tsl_port")) cfg.tsl_port = json["tsl_port"];

    write_config();
    return true;
}
void factory_reset(int state) {
    if (state & 1) {
        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 32, "FACTORY RESET");
        d();
    }
    if (state & 2) {
        info("RESET Config\n");
        cfg.version = 0xff;
        write_config();
        sleep(1);
        ESP.restart();
    }
}
