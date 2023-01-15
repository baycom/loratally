#ifndef __TSL_RECEIVER_COMMON_H__
#define __TSL_RECEIVER_COMMON_H__

#ifdef ESP32

#include <main.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>


#else

#include <stdint.h>
#include <stdio.h>
#include<iostream>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/fcntl.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "Logger.h"
#define dbg Logger::debug

#endif


class AsynchSocket {
public:
	virtual void loop () = 0;
	virtual ~AsynchSocket(){}
};

class ConnectionSet  : public AsynchSocket {

public:
	enum State {
		DISCONNECTED, ESTABLISHING, ESTABLISHED, WAIT_RECONNECT
	};
	virtual int read(uint8_t * buf, int len) = 0 ;
	virtual void loop () = 0;
	virtual void next() = 0 ; /* a complete message was read - currently not needed as we have only one conn*/
	virtual ~ConnectionSet() {}

protected:
	State state;
public:
	ConnectionSet () : state(DISCONNECTED){}
};

class TcpConnectionSetAccept  : public ConnectionSet {
protected:
    int sockfd;
    struct sockaddr_in serv, client;
    bool debug = true;
    struct Connection {
    	int connfd;
    	Connection () { connfd = -1; };
    };
    Connection connection;
public:
	int read(uint8_t * buf, int len) override ;
	void next() override {} ; /* a complete message was read - currently not needed as we have only one conn*/
	TcpConnectionSetAccept(int port );
	virtual void onConnectionChange(Connection & c, bool added){
		// intentionally left blank
	}

	void loop () override;
	virtual ~TcpConnectionSetAccept();
};

class TcpReceiver {
    bool debug = false;
    bool active;
    ConnectionSet * cs;
    enum { UNKNOWN, DELIMITER_1_START, MESSAGE, DELIMITER_1_END } state;
protected:
   public:
    TcpReceiver(ConnectionSet * _cs);
    void printMessage(const char* hdr, uint8_t* msg, int len);
    virtual void onMessage(uint8_t* msg, int len) {}
    void loop();
    virtual ~TcpReceiver();
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
    uint8_t* onDmsg(uint8_t* msg, int len);

   public:
    virtual void onMessage(Message* m) = 0;
    ~TslReceiver() {}
    TslReceiver(ConnectionSet * cs) : TcpReceiver(cs) { currentMessage = nullptr; }
    virtual void onMessage(uint8_t* msg, int len) override ;
};

class TcpConnectionSetClient : public ConnectionSet {
public:
	virtual ~TcpConnectionSetClient();

    int sockfd;
    struct sockaddr_in serv;
    bool debug = false;
    int connfd;
    int screen;
    int index;
    time_t tv_disconnect;
    static const int RECONNECT_TMO_SEC = 10;
public:
	int read(uint8_t * buf, int len) override ;
	void next() override {} ; /* a complete message was read - currently not needed as we have only one conn*/
	TcpConnectionSetClient(int screen, int index, const char * server, int port );

	void loop() override ;


};

#endif
