#include "main.h"

#include <LoRa.h>
#include <CRC32.h>

static unsigned long statusLast = 0;
static unsigned long commandLast = 0;

int RSSIlast = 0;
static uint16_t msg_cnt = 0;

void lora_setup() {
#ifdef HAS_LORA
    SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_CS);
    LoRa.setPins(LoRa_CS, LoRa_RST, LoRa_DIO0);

    if (!LoRa.begin(BAND)) {
        err("SX1276 fail\n");
#ifdef HAS_DISPLAY
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_24);
        display.drawString(64, 12, "SX127X");
        display.drawString(64, 42, "FAIL");
        d();
#endif        
        //        while (true) {
        //            yield();
        //        }
    }

    LoRa.setFrequency(cfg.tx_frequency);
    LoRa.setSignalBandwidth(cfg.bandwidth);
    LoRa.setSpreadingFactor(cfg.sf);
    LoRa.setSyncWord(cfg.syncword);
    LoRa.enableCrc();

    setTallyLight(0, 0, 32, DISP_OFF);

#ifdef DEBUG
    LoRa.dumpRegisters(Serial);
#endif
#endif
}

void lora_shutdown() {
#ifdef HAS_LORA
    LoRa.sleep();
#endif
}

uint16_t LoRaGetMsgCnt(void) {
    return msg_cnt;
}

int LoRaGetRSSI(void) {
    return RSSIlast;
}

int LoRaSend(uint8_t addr, uint8_t r, uint8_t g, uint8_t b, bool disp) {
#ifdef HAS_LORA
    tallyCMD_t t;
    memset(&t, 0, sizeof(tallyCMD_t));
    t.t.version = CMD_VERSION;
    t.t.cmd = cmd_TALLY;
    t.t.addr = addr;
    t.t.r = r;
    t.t.g = g;
    t.t.b = b;
    t.dw.crc32 = CRC32::calculate(t.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));
#ifdef HAS_DISPLAY    
    if (disp) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 4, "Addr : " + String(t.t.addr));
        display.drawString(64, 14, "Red  : " + String(t.t.r));
        display.drawString(64, 24, "Green: " + String(t.t.g));
        display.drawString(64, 34, "Blue : " + String(t.t.b));
        d();
    }
#endif    
    dbg("crc32: %08x\n", t.dw.crc32);
    dbg("E1.31 -> Lora: ");
    #ifdef DEBUG
    for (int i = 0; i < sizeof(t); i++) {
        printf("%02X ", t.b8[i]);
    }
    #endif
    dbg("\n");

    LoRa.beginPacket();
    LoRa.write(t.b8, sizeof(t));
    LoRa.endPacket();
#endif
    return 1;
}

static uint8_t LoRaEncodeTallyState(int index) {
    uint8_t stateRH, brightnessRH, stateLH, brightnessLH;
    if (getTallyState(index | TALLY_RH, stateRH, brightnessRH) &&
        getTallyState(index | TALLY_LH, stateLH, brightnessLH)) {
        return (stateRH & 3) | (brightnessRH & 3) << 2 | (stateLH & 3) << 4 |
               (brightnessLH & 3) << 6;
    }
    return 0;
}

static void LoRaDecodeTallyState(int index, uint8_t tallyStateEncoded) {
    uint8_t stateRH = tallyStateEncoded & 3;
    uint8_t brightnessRH = (tallyStateEncoded >> 2) & 3;
    uint8_t stateLH = (tallyStateEncoded >> 4) & 3;
    uint8_t brightnessLH = (tallyStateEncoded >> 6) & 3;

    setTallyState(index | TALLY_RH, stateRH, brightnessRH);
    setTallyState(index | TALLY_LH, stateLH, brightnessLH);
}

int LoRaBCTS(void) {
#ifdef HAS_LORA
    tallyCMD_t t;
    memset(&t, 0, sizeof(tallyCMD_t));
    t.bts.version = CMD_VERSION;
    t.bts.cmd = cmd_BC_TS;

    for (int i = 1; i <= LORA_MAX_TS; i++) {
        t.bts.tally_state[i-1] = LoRaEncodeTallyState(i);
    }
    t.dw.crc32 = CRC32::calculate(t.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));

    LoRa.beginPacket();
    LoRa.write(t.b8, sizeof(t));
    LoRa.endPacket();
    commandLast = millis();
#endif
    return 1;
}

int LoRaBCRGB(uint8_t *property_values, int numChannels, bool disp) {
#ifdef HAS_LORA
    tallyCMD_t t;
    memset(&t, 0, sizeof(tallyCMD_t));
    t.b.version = CMD_VERSION;
    t.b.cmd = cmd_BROADCAST;

    if (numChannels < 1) {
        return 0;
    }

    numChannels /= 3;

    if (numChannels > LORA_MAX_RGB) {
        numChannels = LORA_MAX_RGB;
    }
    for (int i = 0; i < numChannels; i++) {
        uint8_t r = property_values[i * 3];
        uint8_t g = property_values[(i * 3) + 1];
        uint8_t b = property_values[(i * 3) + 2];
        uint8_t y = (0.3333 * r + 0.3333 * g + 0.3333 * b);
        dbg("LoRaBC1: r: %d g: %d b: %d y: %d\n", r, g, b, y);
        r >>= 6;
        b >>= 6;
        g >>= 6;
        y >>= 6;
        dbg("LoRaBC2: y: %d r: %d g: %d b: %d\n", y, r, g, b);
        t.b.rgb[i] = y << 6 | g << 4 | b << 2 | r;
    }
    t.dw.crc32 = CRC32::calculate(t.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));
#ifdef HAS_DISPLAY
    if (disp) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 4, "Addr : " + String(t.t.addr));
        d();
    }
#endif
    dbg("crc32: %08x\n", t.dw.crc32);
    dbg("E1.31 -> Lora: ");
    for (int i = 0; i < sizeof(t); i++) {
        dbg("%02X ", t.b8[i]);
    }
    dbg("\n");

    LoRa.beginPacket();
    LoRa.write(t.b8, sizeof(t));
    LoRa.endPacket();
#endif
    return 1;
}

void commandBC(void) {
#ifdef HAS_LORA
    uint8_t property_vals[TALLY_MAX_NUM * 3];
    for (uint16_t n = 0; n < TALLY_MAX_NUM; n++) {
        int r, g, b;
        uint8_t state, brightness;
        getTallyState(n, state, brightness);
        rgbFromTSL(r, g, b, state, brightness);
        property_vals[n * 3] = r;
        property_vals[(n * 3) + 1] = g;
        property_vals[(n * 3) + 2] = b;
    }
    LoRaBCRGB(property_vals, TALLY_MAX_NUM);
#endif
}

static void tallyFromEncoded(uint8_t *encodedrgb, uint8_t tally_id) {
#ifdef HAS_LORA
    if (tally_id < 1 || tally_id > 14) {
        return;
    }
    uint8_t v = encodedrgb[tally_id - 1];
    uint8_t gx = (v >> 4) & 3;
    uint8_t bx = (v >> 2) & 3;
    uint8_t rx = (v & 3);
    uint8_t y = (v >> 6) & 3;

    uint8_t g = gx << y << 5;
    uint8_t b = bx << y << 5;
    uint8_t r = rx << y << 5;
    dbg("tallyFromEncoded: %d %d\n", v, tally_id);
    dbg("gx: %d rx: %d bx %d y: %d\n", gx, rx, bx, y);
    dbg("r: %d g: %d b %d \n", r, g, b);
    setTallyLight(r, g, b, DISP_RSSI);
#endif
}

void lora_loop() {
#ifdef HAS_LORA
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        int rssi = LoRa.packetRssi();
        dbg("incoming packet: %d RSSI %d\n", packetSize, rssi);
        if (packetSize == sizeof(tallyCMD_t)) {
            tallyCMD_t t;
            LoRa.readBytes(t.b8, sizeof(t));
#ifdef DEBUG
            for (int i = 0; i < packetSize; i++) {
                printf("%02X ", t.b8[i]);
            }
#endif            
            dbg("\n");
            uint32_t crc32 =
                CRC32::calculate(t.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));
            dbg("crc32: %08x %08x\n", t.dw.crc32, crc32);
#
            if (t.dw.crc32 == crc32) {
                if (t.t.version <= CMD_VERSION) {
                    if (t.t.cmd == cmd_TALLY && t.t.addr == cfg.tally_id) {
                        msg_cnt++;
                        RSSIlast = rssi;
                        setTallyLight(t.t.r, t.t.g, t.t.b, DISP_RSSI);
                    } else if (t.b.cmd == cmd_BROADCAST) {
                        msg_cnt++;
                        RSSIlast = rssi;
                        dbg("Tally Broadcast:\n");
                        tallyFromEncoded(t.b.rgb, cfg.tally_id);
                    } else if (t.t.cmd == cmd_BC_TS) {
                        RSSIlast = rssi;
                        msg_cnt++;
                        for (int i = 0; i < LORA_MAX_TS; i++) {
                            LoRaDecodeTallyState(i + 1, t.bts.tally_state[i]);
                        }
                    } else if (t.t.cmd == cmd_STATUS) {
                        DynamicJsonDocument json(256);
                        dbg("Tally Status:\n");
                        dbg("Addr     : %d\n", t.s.addr);
                        dbg("Voltage  : %d\n", t.s.voltage);
                        dbg("MsgCount : %d\n", t.s.msgcnt);
                        dbg("RSSI     : %d\n\n", t.s.rssi);

                        json["version"] = CMD_VERSION;
                        json["uptime"] = millis()/1000;
                        json["address"] = t.s.addr;
                        json["voltage"] = t.s.voltage;
                        json["msgCount"] = t.s.msgcnt;
                        json["RSSI"] = t.s.rssi;
                        char macStr[18] = {0};
                        uint8_t *mac = t.s.mac;
                        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
                                mac[1], mac[2], mac[3], mac[4], mac[5]);
                        json["MAC"] = macStr;
                        String output;
                        serializeJson(json, output);
                        String topic = "tally/status/" + String(cfg.tally_id);
                        boolean ret =
                            mqtt_publish(topic.c_str(), output.c_str());
                        dbg("ret: %d json: %s\n", ret, output.c_str());
                    }
                } else {
                    warn("protocol version mismatch: %d %d\n", t.t.version,
                           CMD_VERSION);
                }
            } else {
                warn("crc32 failed\n");
            }
        }
    }
    if (cfg.status_interval && (millis() - statusLast) > cfg.status_interval) {
        dbg("Transmit status:\n");
        tallyCMD_t s;
        s.s.version = CMD_VERSION;
        s.s.cmd = cmd_STATUS;
        s.s.addr = cfg.tally_id;
        s.s.voltage = battVolt() / 100.0;
        s.s.msgcnt = msg_cnt;
        s.s.rssi = RSSIlast;
        WiFi.macAddress(s.s.mac);
        s.dw.crc32 =
            CRC32::calculate(s.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));

        LoRa.beginPacket();
        LoRa.write(s.b8, sizeof(s));
        LoRa.endPacket();
        statusLast = millis();
    }
    if (cfg.command_interval &&
        (millis() - commandLast) > cfg.command_interval) {
        dbg("Transmit commands:\n");
        LoRaBCTS();
    }
#endif
}
