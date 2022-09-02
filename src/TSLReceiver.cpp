#include "main.h"
#include <lwip/netdb.h>
#include <lwip/sockets.h>

class TcpReceiver {
    int sockfd;
    struct sockaddr_in serv, client;
    bool active;
    bool debug = false;
    enum { UNKNOWN, DELIMITER_1_START, MESSAGE, DELIMITER_1_END } state;

   public:
    TcpReceiver(int port) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("sock");
            exit(1);
        }
        fcntl(sockfd, F_SETFL, O_NONBLOCK);
        memset(&serv, 0, sizeof(struct sockaddr_in));
        serv.sin_family = AF_INET;
        serv.sin_port = htons(port);
        serv.sin_addr.s_addr = inet_addr("0.0.0.0");
        if (bind(sockfd, (struct sockaddr*)&serv, sizeof(struct sockaddr_in)) ==
            -1) {
            perror("bind");
            exit(3);
        }

        // Now server is ready to listen and verification
        if ((listen(sockfd, 5)) != 0) {
            perror("Listen failed...");
            exit(6);
        } else if (debug)
            printf("Server listening..\n");

        active = true;
        state = UNKNOWN;
    }

    void interrupt() { active = false; }
    void printMessage(const char* hdr, uint8_t* msg, int len) {
        printf("%s\n", hdr);
        for (int i = 0; i < len / 8 + 1; i++) {
            printf("     ");
            for (int j = 0; j < 8; j++) {
                if (i * 8 + j < len) {
                    printf("%02x ", (int)msg[i * 8 + j]);
                }
            }
            printf("\n");
        }
    }
    virtual void onMessage(uint8_t* msg, int len) {}

    void rcvLoop() {
        char buffer[2048];
        socklen_t l = sizeof(client);
        uint8_t message[1000];
        uint8_t* p = message;
        uint8_t t;
        uint16_t len = 0;
        while (active) {
            // Accept the data packet from client and verification
            int connfd = accept(sockfd, (struct sockaddr*)&client, &l);
            if (connfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                perror("server accept failed...\n");
                return;
            } else if (debug)
                printf("server accept the client...\n");
            while (read(connfd, &t, 1) == 1) {
                switch (state) {
                    case UNKNOWN: {
                        switch (t) {
                            case 0xfe:
                                state = DELIMITER_1_START;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    case DELIMITER_1_START: {
                        switch (t) {
                            case 0x02:
                                state = MESSAGE;
                                p = message;
                                uint8_t b1, b2;
                                read(connfd, &b1, 1);
                                read(connfd, &b2, 1);
                                len = b1 + (((uint16_t)b2) << 8);
                                if (debug)
                                    printf("received message with length %d\n",
                                           (int)len);
                                break;
                            default:
                                state = UNKNOWN;
                                break;
                        }
                        break;
                    }
                    case MESSAGE: {
                        switch (t) {
                            case 0xfe:
                                state = DELIMITER_1_END;
                                break;
                            default:
                                *p++ = t;
                                if (p - message == len) {
                                    state = UNKNOWN;
                                    onMessage(message, len);
                                }
                        }
                        break;
                    }
                    case DELIMITER_1_END: {
                        switch (t) {
                            case 0xfe:
                                state = MESSAGE;
                                *p++ = t;
                                if (p - message == len) {
                                    state = UNKNOWN;
                                    onMessage(message, len);
                                }
                                break;
                            case 0x2:
                                state = UNKNOWN;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                }
            }
            close(connfd);
        }
    }
    virtual ~TcpReceiver() {}
};

class TslReceiver : public TcpReceiver {
   protected:
    enum FlagsValue { UNICODE_STRINGS = 1, SCONTROL = 2 };
    struct Message {
        struct SControl {};
        struct DMsg {
            uint16_t index;
            enum TallyState { OFF = 0, RED = 1, GREEN = 2, AMBER = 3 };
            TallyState rh;
            TallyState text;
            TallyState lh;
            uint16_t brightness;
            bool controlData;
            struct DisplayData {
                uint16_t len;
                static const int max_text_length = 100;
                char text[max_text_length];
                DisplayData() {
                    len = 0;
                    memset(text, 0, sizeof(text));
                }
            } displayData;
            DMsg() {
                index = 0;
                rh = text = lh = OFF;
                controlData = false;
                brightness = 0;
            }
        };
        int version;
        int flags;
        uint16_t screen;
        SControl* scontrol;
        static const int max_dmsgs_per_message = 10;
        DMsg* dmsgs[max_dmsgs_per_message];
        int n_dmsgs;
        Message() {
            version = 0;
            flags = 0;
            screen = 0;
            scontrol = nullptr;
            n_dmsgs = 0;
            memset(dmsgs, 0, sizeof(dmsgs));
        }
        virtual ~Message() {
            for (int i = 0; i < n_dmsgs; i++) {
                if (dmsgs[i]) delete dmsgs[i];
            }
        }
    };
    Message* currentMessage;
    uint8_t* onDmsg(uint8_t* msg, int len) {
        Message::DMsg* dmsg = new Message::DMsg();
        dmsg->index = msg[0] + (((uint16_t)msg[1]) << 8);
        uint16_t control = msg[2] + (((uint16_t)msg[3]) << 8);
        dmsg->rh = (enum Message::DMsg::TallyState)(((control >> 0) & 0x3));
        dmsg->text = (enum Message::DMsg::TallyState)(((control >> 2) & 0x3));
        dmsg->lh = (enum Message::DMsg::TallyState)(((control >> 4) & 0x3));
        dmsg->brightness = (((control >> 6) & 0x3));
        dmsg->controlData = (control >> 15) != 0;
        dmsg->displayData.len = msg[4] + (((uint16_t)msg[5]) << 8);
        int __len = dmsg->displayData.len >
                            Message::DMsg::DisplayData::max_text_length - 1
                        ? Message::DMsg::DisplayData::max_text_length - 1
                        : dmsg->displayData.len;
        memcpy(dmsg->displayData.text, &msg[6], __len);
        dmsg->displayData.text[__len + 1] = 0;
        currentMessage->dmsgs[currentMessage->n_dmsgs++] = dmsg;
        return &msg[6 + dmsg->displayData.len + 1];
    }

   public:
    virtual void onMessage(Message* m) = 0;
    ~TslReceiver() {}
    TslReceiver(int port) : TcpReceiver(port) {}
    virtual void onMessage(uint8_t* msg, int len) override {
        currentMessage = new Message();
        currentMessage->version = msg[0];
        currentMessage->flags = msg[1];
        currentMessage->screen = msg[2] + (((uint16_t)msg[3]) << 8);
        if (currentMessage->flags & SCONTROL) {
            // not defined in V5 of the protocol
        } else {
            uint8_t* next_msg = &msg[4];
            int _len = len;
            while (true) {
                _len = len - (next_msg - msg);
                if (_len <= 0) break;
                next_msg = onDmsg(next_msg, _len);
            }
        }
        onMessage(currentMessage);
        delete currentMessage;
        currentMessage = nullptr;
    }
};

class ESPTslReceiver : public TslReceiver {
   public:
    ESPTslReceiver(int port) : TslReceiver(port) {}
    virtual void onMessage(Message* m) override {
        bool tallyChanged = false;
#ifdef DEBUG
        time_t timer;
        char buffer[26];
        struct tm* tm_info;
        timer = time(NULL);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        printf("received message: %s\n", buffer);
        printf("{ \n");
        printf("  \"%s\":%d,\n", "version", (int)m->version);
        printf("  \"%s\":%d,\n", "flags", (int)m->flags);
        printf("  \"%s\":%d,\n", "screen", (int)m->screen);
        printf("  \"%s\":%s,\n", "control",
               m->flags & SCONTROL ? "true" : "false");
#endif
        if (!(m->flags & SCONTROL)) {
#ifdef DEBUG
            printf("    [\n");
#endif
            for (int i = 0; i < m->n_dmsgs; i++) {
#ifdef DEBUG
                printf("      {\n");
                printf("        \"%s\": %d,\n", "index",
                       (int)m->dmsgs[i]->index);
                printf("        \"%s\": %d,\n", "rh_tally",
                       (int)m->dmsgs[i]->rh);
                printf("        \"%s\": %d,\n", "text_tally",
                       (int)m->dmsgs[i]->text);
                printf("        \"%s\": %d,\n", "lh_tally",
                       (int)m->dmsgs[i]->lh);
                printf("        \"%s\": %d,\n", "brightness",
                       (int)m->dmsgs[i]->brightness);
                printf("        \"%s\": %s", "control",
                       m->dmsgs[i]->controlData ? "true" : "false");
                if (!m->dmsgs[i]->controlData &&
                    m->dmsgs[i]->displayData.len != 0) {
                    printf(",\n");
                    printf("        \"%s\": \"%s\"\n", "displayData",
                           m->dmsgs[i]->displayData.text);
                } else {
                    printf("\n");
                }
                printf("      }");
                if (i == m->n_dmsgs) {
                    printf(",");
                };
                printf("\n");
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
            printf("    ]\n");
#endif
        }
#ifdef DEBUG
        printf("}\n");
#endif
    }
    virtual ~ESPTslReceiver() {}
};

TcpReceiver* rcv = nullptr;

void tsl_setup(int tsl_port) { rcv = new ESPTslReceiver(tsl_port); }

void tsl_loop(void) {
    if (eth_connected) {
        rcv->rcvLoop();
    }
}
