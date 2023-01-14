#include "main.h"
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include "TSLReceiverCommon.h"



class ESPTslReceiver : public TslReceiver {
   public:
    ESPTslReceiver(ConnectionSet * cs) : TslReceiver(cs) {}
    virtual void onMessage(Message* m) override {
        bool tallyChanged = false;
#ifdef DEBUG
        time_t timer;
        char buffer[26];
        struct tm* tm_info;
        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        dbg("received message: %s\n", buffer);
        dbg("{ \n");
        dbg("  \"%s\":%d,\n", "version", (int)m->version);
        dbg("  \"%s\":%d,\n", "flags", (int)m->flags);
        dbg("  \"%s\":%d,\n", "screen", (int)m->screen);
        dbg("  \"%s\":%s,\n", "control",
               m->flags & SCONTROL ? "true" : "false");
#endif
        if (!(m->flags & SCONTROL)) {
#ifdef DEBUG
            dbg("    [\n");
#endif
            for (int i = 0; i < m->n_dmsgs; i++) {
#ifdef DEBUG
                dbg("      {\n");
                dbg("        \"%s\": %d,\n", "index",
                       (int)m->dmsgs[i]->index);
                dbg("        \"%s\": %d,\n", "rh_tally",
                       (int)m->dmsgs[i]->rh);
                dbg("        \"%s\": %d,\n", "text_tally",
                       (int)m->dmsgs[i]->text);
                dbg("        \"%s\": %d,\n", "lh_tally",
                       (int)m->dmsgs[i]->lh);
                dbg("        \"%s\": %d,\n", "brightness",
                       (int)m->dmsgs[i]->brightness);
                dbg("        \"%s\": %s", "control",
                       m->dmsgs[i]->controlData ? "true" : "false");
                if (!m->dmsgs[i]->controlData &&
                    m->dmsgs[i]->displayData.len != 0) {
                    dbg(",\n");
                    dbg("        \"%s\": \"%s\"\n", "displayData",
                           m->dmsgs[i]->displayData.text);
                } else {
                    dbg("\n");
                }
                dbg("      }");
                if (i == m->n_dmsgs) {
                    dbg(",");
                };
                dbg("\n");
#endif
                uint16_t index = m->dmsgs[i]->index;
                if (index < TALLY_MAX_NUM) {
                    tallyChanged |= setTallyState(index|TALLY_LH, m->dmsgs[i]->lh, m->dmsgs[i]->brightness, m->dmsgs[i]->displayData.text);
                    tallyChanged |= setTallyState(index|TALLY_RH, m->dmsgs[i]->rh, m->dmsgs[i]->brightness, m->dmsgs[i]->displayData.text);
                }
            }
            if (tallyChanged) {
                LoRaBCTS();
            }
#ifdef DEBUG
            dbg("    ]\n");
#endif
        }
#ifdef DEBUG
        dbg("}\n");
#endif
    }
    virtual ~ESPTslReceiver() {}
};

TcpReceiver* rcv = nullptr;
ConnectionSet * cs = nullptr; 

void tsl_setup(int tally_screen, int tally_id, const char * tsl_host, int tsl_port) {

    if (tsl_host[0] != 0){
        dbg("starting as client (%d,%d) to (%s,%d)\n",tally_screen,tally_id,tsl_host,tsl_port);
        cs = new TcpConnectionSetClient(tally_screen,tally_id,tsl_host, tsl_port); 
    } else {
        dbg("starting as server on port %d\n",tsl_port);
        cs = new TcpConnectionSetAccept(tsl_port); 
    }
    rcv = new ESPTslReceiver(cs); 
}

void tsl_loop(void) {
    if (eth_connected) {
        cs->loop();
        rcv->loop();
    }
}
