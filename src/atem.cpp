#include "main.h"

#include "ATEMbase.h"
#include "ATEMext.h"

ATEMext AtemSwitcher;

void atem_setup() {
    if (cfg.atem_host[0]) {
        IPAddress ATEMIp;
        if (!ATEMIp.fromString(cfg.atem_host)) {
            ATEMIp = MDNS.queryHost(cfg.atem_host);
            if (ATEMIp.toString() == "0.0.0.0") {
                err("cannot resolve atem host: %s\n", cfg.atem_host);
                return;
            }
        }
        AtemSwitcher.begin(ATEMIp);
        AtemSwitcher.serialOutput(0x80);
        info("AtemSwitcher\n");
    }
}

void atem_loop() {
    if (cfg.atem_host[0] && eth_connected) {
        AtemSwitcher.runLoop();
        if (AtemSwitcher.isConnected()) {
            bool tallyChanged = false;
            uint16_t indexSources = AtemSwitcher.getTallyByIndexSources();
            if (indexSources > TALLY_MAX_NUM) {
                indexSources = TALLY_MAX_NUM;
            }
//            indexSources=16;
            for (uint16_t n = cfg.atem_channel_offset; n < indexSources; n++) {
                uint8_t tallyState = AtemSwitcher.getTallyByIndexTallyFlags(n);
                tallyChanged |= setTallyState(n+1|TALLY_RH, tallyState, 3);
                tallyChanged |= setTallyState(n+1|TALLY_LH, tallyState, 3);
                if(tallyState && tallyChanged) {
                    dbg("tally: %d %d\n", n+1, tallyState);
                }
            }
            if(tallyChanged) {
                LoRaBCTS();
            }
        }
    }
}
