#include <ModbusMaster.h>

#include "main.h"

ModbusMaster node;
static uint8_t tallyState[TALLY_MAX_NUM];

void preTransmission() { usleep(2000); }

void modbus_setup() {
#ifdef HAS_MODBUS
    dbg("Init Modbus\n");
    Serial2.begin(19200, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
    node.preTransmission(preTransmission);
    node.begin(1, Serial2);
    node.writeSingleRegister(0, 0x0800);
#endif
}

void modbus_loop() {
#ifdef HAS_MODBUS
    for (int i = 1; i < 9; i++) {
        uint8_t state;
        uint8_t brightness;
        getTallyState(i, state, brightness);
        if (tallyState[i] != state) {
            tallyState[i] = state;
            if (state & 1) {
                node.writeSingleRegister(i, 0x0100);
                node.writeSingleRegister(i + 8, 0x0200);
            } else if (state & 2) {
                node.writeSingleRegister(i, 0x0200);
                node.writeSingleRegister(i + 8, 0x0100);
            } else {
                node.writeSingleRegister(i, 0x0200);
                node.writeSingleRegister(i + 8, 0x0200);
            }
        }
    }
#endif
}
