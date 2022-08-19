#include "main.h"

static ESPAsyncE131 e131(UNIVERSE_COUNT);
static int lastPacket = -1;

void e131_setup() {
        e131.begin(E131_UNICAST);
}

void e131dmx_loop(void) {
    if (!e131.isEmpty()) {
        e131_packet_t packet;
        e131.pull(&packet);  // Pull packet from ring buffer
#ifdef DEBUG
        Serial.printf(
            "Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
            htons(packet.universe),  // The Universe for this packet
            htons(packet.property_value_count) -
                1,  // Start code is ignored, we're interested in dimmer data
            e131.stats.num_packets,      // Packet counter
            e131.stats.packet_errors,    // Packet error counter
            packet.property_values[1]);  // Dimmer data for Channel 1
#endif
        uint16_t numChannels = htons(packet.property_value_count) - 1;
        if (numChannels >= 3) {
            lastPacket = millis();
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t addr;

#ifdef DEBUG
            for (int ch = 0; ch < numChannels; ch++) {
                printf("%d %d\n", ch, packet.property_values[ch]);
            }
#endif

            addr = htons(packet.universe);
            r = packet.property_values[1];
            g = packet.property_values[2];
            b = packet.property_values[3];
            if (addr < 255) {
                LoRaSend(addr, r, g, b);
            } else {
                if ((numChannels / 3) >= cfg.tally_id) {
                    int pos = (cfg.tally_id - 1) * 3;
                    setTallyLight(packet.property_values[pos + 1],
                                  packet.property_values[pos + 2],
                                  packet.property_values[pos + 3]);
                }
                LoRaBC(packet.property_values + 1, numChannels - 1);
            }
            if (addr == cfg.tally_id) {
                setTallyLight(r, g, b);
            }
        }
    }
}
