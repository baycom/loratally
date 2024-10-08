#ifndef LORATALLY_H
#define LORATALLY_H

#define CMD_VERSION 3
#define LORA_MAX_RGB 32
#define LORA_MAX_TS 32

typedef enum {
    cmd_TALLY = 1,
    cmd_STATUS = 2,
    cmd_BROADCAST = 3,
    cmd_BC_TS = 4
} tally_cmd_t;

typedef union {
    struct {
        uint8_t version;
        uint8_t cmd;
        uint8_t addr;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } t;
    struct {
        uint8_t version;
        uint8_t cmd;
        uint8_t addr;
        uint16_t voltage;
        int16_t rssi;
        uint8_t msgcnt;
        uint8_t mac[6];
    } s;
    struct {
        uint8_t version;
        uint8_t cmd;
        uint8_t rgb[32];
    } b;
    struct {
        uint8_t version;
        uint8_t cmd;
        uint8_t tally_state[32];
    } bts;
    struct {
        uint32_t ui32_1;
        uint32_t ui32_2;
        uint32_t ui32_3;
        uint32_t ui32_4;
        uint32_t ui32_5;
        uint32_t ui32_6;
        uint32_t ui32_7;
        uint32_t ui32_8;
        uint32_t crc32;
    } dw;

    uint32_t b32[9];
    uint16_t b16[18];
    uint8_t  b8[36];
} tallyCMD_t;

void lora_setup();
void lora_shutdown();
uint16_t LoRaGetMsgCnt(void);
int LoRaGetRSSI(void);
int LoRaSend(uint8_t addr, uint8_t r, uint8_t g, uint8_t b, bool disp = false);
int LoRaBCRGB(uint8_t *property_values, int numChannels, bool disp = false);
int LoRaBCTS(void);
void commandBC(void);
void lora_loop();


#endif