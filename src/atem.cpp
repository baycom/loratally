#include "main.h"

ATEMext AtemSwitcher;

void atem_setup() {
    if (cfg.atem_host[0]) {
        IPAddress ATEMIp;
        if (!ATEMIp.fromString(cfg.atem_host)) {
            ATEMIp = MDNS.queryHost(cfg.atem_host);
            if (ATEMIp.toString() == "0.0.0.0") {
                printf("cannot resolve atem host: %s\n", cfg.atem_host);
                return;
            }
        }
        AtemSwitcher.begin(ATEMIp);
        AtemSwitcher.serialOutput(0x80);
        AtemSwitcher.connect();
    }
}

void atem_loop() {
    uint8_t property_vals[TALLY_MAX_NUM * 3];
    bool tally_changed = false;

    if (cfg.atem_host[0] && eth_connected) {
        AtemSwitcher.runLoop();
        if (AtemSwitcher.hasInitialized()) {
            uint16_t indexSources = AtemSwitcher.getTallyByIndexSources();
            if (indexSources > TALLY_MAX_NUM) {
                indexSources = TALLY_MAX_NUM;
            }
            for (uint16_t n = 0; n < indexSources; n++) {
                uint8_t tally = AtemSwitcher.getTallyByIndexTallyFlags(n);
                if (tally != tallyState[n]) {
                    printf("Tally: %d State: %d\n", n, tally);
                    tallyState[n] = tally;
                    tally_changed = true;
                }
                uint8_t ts = tallyState[n];
                property_vals[n * 3] = (ts & 1) ? 192 : 0;
                property_vals[(n * 3) + 1] =
                    (!property_vals[n * 3] && ts & 2) ? 192 : 0;
                property_vals[(n * 3) + 2] = 0;
            }
            if (tally_changed) {
                uint16_t pos = cfg.tally_id - 1;
                if (indexSources >= pos) {
                    setTallyLight(property_vals[pos * 3],
                                  property_vals[pos * 3 + 1], 0);
                }
                LoRaBC(property_vals, indexSources * 3, false);
            }
        }
    }
}
