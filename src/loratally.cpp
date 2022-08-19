#include "main.h"
#include <CRC32.h>

static unsigned long statusLast = 0;
static unsigned long commandLast = 0;
int RSSIlast = 0;
static uint16_t msg_cnt = 0;

void lora_setup() {
    SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_CS);
    LoRa.setPins(LoRa_CS, LoRa_RST, LoRa_DIO0);

    if (!LoRa.begin(BAND)) {
        printf("SX1276 fail\n");
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_24);
        display.drawString(64, 12, "SX127X");
        display.drawString(64, 42, "FAIL");
        d();
//        while (true) {
//            yield();
//        }
    }

    LoRa.setFrequency(cfg.tx_frequency);
    LoRa.setSignalBandwidth(cfg.bandwidth);
    LoRa.setSpreadingFactor(cfg.sf);
    LoRa.setSyncWord(cfg.syncword);
    LoRa.enableCrc();

    setTallyLight(0, 0, 32, false);

#ifdef DEBUG
    LoRa.dumpRegisters(Serial);
#endif
}

 int LoRaSend(uint8_t addr, uint8_t r, uint8_t g, uint8_t b,
                    bool disp) {
    tallyCMD_t t;
    memset(&t, 0, sizeof(tallyCMD_t));
    t.t.version = CMD_VERSION;
    t.t.cmd = cmd_TALLY;
    t.t.addr = addr;
    t.t.r = r;
    t.t.g = g;
    t.t.b = b;
    t.dw.crc32 = CRC32::calculate(t.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));
    if (disp && heltec) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 4, "Addr : " + String(t.t.addr));
        display.drawString(64, 14, "Red  : " + String(t.t.r));
        display.drawString(64, 24, "Green: " + String(t.t.g));
        display.drawString(64, 34, "Blue : " + String(t.t.b));
        d();
    }
#ifdef DEBUG
    printf("crc32: %08x\n", t.dw.crc32);
    printf("E1.31 -> Lora: ");
    for (int i = 0; i < sizeof(t); i++) {
        printf("%02X ", t.b8[i]);
    }
    printf("\n");
#endif
    LoRa.beginPacket();
    LoRa.write(t.b8, sizeof(t));
    LoRa.endPacket();

    return 1;
}

int LoRaBC(uint8_t *property_values, int numChannels, bool disp) {
    tallyCMD_t t;
    memset(&t, 0, sizeof(tallyCMD_t));
    t.b.version = CMD_VERSION;
    t.b.cmd = cmd_BROADCAST;

    if (numChannels < 1) {
        return 0;
    }

    numChannels /= 3;

    if (numChannels > TALLY_MAX_NUM) {
        numChannels = TALLY_MAX_NUM;
    }
    for (int i = 0; i < numChannels; i++) {
        uint8_t r = property_values[i * 3];
        uint8_t g = property_values[(i * 3) + 1];
        uint8_t b = property_values[(i * 3) + 2];
        uint8_t y = (0.3333 * r + 0.3333 * g + 0.3333 * b);
#ifdef DEBUG
        printf("LoRaBC1: r: %d g: %d b: %d y: %d\n", r, g, b, y);
#endif
        r >>= 6;
        b >>= 6;
        g >>= 6;
        y >>= 6;
#ifdef DEBUG
        printf("LoRaBC2: y: %d r: %d g: %d b: %d\n", y, r, g, b);
#endif
        t.b.rgb[i] = y << 6 | g << 4 | b << 2 | r;
    }
    t.dw.crc32 = CRC32::calculate(t.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));

    if (disp && heltec) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 4, "Addr : " + String(t.t.addr));
        d();
    }

#ifdef DEBUG
    printf("crc32: %08x\n", t.dw.crc32);
    printf("E1.31 -> Lora: ");
    for (int i = 0; i < sizeof(t); i++) {
        printf("%02X ", t.b8[i]);
    }
    printf("\n");
#endif
    LoRa.beginPacket();
    LoRa.write(t.b8, sizeof(t));
    LoRa.endPacket();

    return 1;
}

void commandBC(void) {
    uint8_t property_vals[TALLY_MAX_NUM * 3];
    for (uint16_t n = 0; n < TALLY_MAX_NUM; n++) {
        int r, g, b;
        rgbFromTSL(r, g, b, 3, tallyState[n]);
        property_vals[n * 3] = r;
        property_vals[(n * 3) + 1] = g;
        property_vals[(n * 3) + 2] = b;
    }
    LoRaBC(property_vals, TALLY_MAX_NUM, false);
}

static void tallyFromEncoded(uint8_t *encodedrgb, uint8_t tally_id) {
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
#ifdef DEBUG
    printf("tallyFromEncoded: %d %d\n", v, tally_id);
    printf("gx: %d rx: %d bx %d y: %d\n", gx, rx, bx, y);
    printf("r: %d g: %d b %d \n", r, g, b);
#endif
    setTallyLight(r, g, b);
}

void lora_loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        int rssi = LoRa.packetRssi();
#ifdef DEBUG
        printf("incoming packet: %d RSSI %d\n", packetSize, rssi);
#endif
        if (packetSize == sizeof(tallyCMD_t)) {
            tallyCMD_t t;
            LoRa.readBytes(t.b8, sizeof(t));
#ifdef DEBUG
            for (int i = 0; i < packetSize; i++) {
                printf("%02X ", t.b8[i]);
            }
            printf("\n");
#endif
            uint32_t crc32 =
                CRC32::calculate(t.b8, sizeof(tallyCMD_t) - sizeof(uint32_t));
#ifdef DEBUG
            printf("crc32: %08x %08x\n", t.dw.crc32, crc32);
#endif
            if (t.dw.crc32 == crc32) {
                if (t.t.version <= CMD_VERSION) {
                    RSSIlast = rssi;
                    if (t.t.cmd == cmd_TALLY && t.t.addr == cfg.tally_id) {
                        msg_cnt++;
                        setTallyLight(t.t.r, t.t.g, t.t.b);
                    } else if (t.b.cmd == cmd_BROADCAST) {
                        msg_cnt++;
#ifdef DEBUG
                        printf("Tally Broadcast:\n");
#endif
                        tallyFromEncoded(t.b.rgb, cfg.tally_id);
                    } else if (t.t.cmd == cmd_STATUS) {
                        DynamicJsonDocument json(256);
#ifdef DEBUG
                        printf("Tally Status:\n");
                        printf("Addr     : %d\n", t.s.addr);
                        printf("Voltage  : %d\n", t.s.voltage);
                        printf("MsgCount : %d\n", t.s.msgcnt);
                        printf("RSSI     : %d\n\n", t.s.rssi);
#endif
                        json["version"] = CMD_VERSION;
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
#ifdef DEBUG
                        printf("ret: %d json: %s\n", ret, output.c_str());
#endif
                    }
                } else {
                    printf("protocol version mismatch: %d %d\n", t.t.version,
                           CMD_VERSION);
                }
            } else {
                printf("crc32 failed\n");
            }
        }
    }
    if (cfg.status_interval && (millis() - statusLast) > cfg.status_interval) {
#ifdef DEBUG
        printf("Transmit status:\n");
#endif
        tallyCMD_t s;
        s.s.version = CMD_VERSION;
        s.s.cmd = cmd_STATUS;
        s.s.addr = cfg.tally_id;
        s.s.voltage = BATTVOLT() / 100.0;
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
        commandBC();
    }
}
