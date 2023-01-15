#include "TSLReceiverCommon.h"

int TcpConnectionSetAccept::read(uint8_t * buf, int len){
	bool disconnected = false;
	int ret = -1;
	if (state == ESTABLISHED){
		ret = ::read(connection.connfd,buf,len);
		if (ret < 0) {
			if (!(errno == EAGAIN || errno == EWOULDBLOCK) ) {
				disconnected = true;
			}
		}
		if (ret == 0) {
			disconnected = true;
		}
	}
	if (disconnected){
		state = DISCONNECTED;
		onConnectionChange(connection,false);
		close(connection.connfd);
		connection.connfd = -1;
		dbg ("disconnected\n");
	}
	return ret;
}


TcpConnectionSetAccept::TcpConnectionSetAccept(int port ) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("sock");
            exit(1);
        }
        fcntl(sockfd, F_SETFL, O_NONBLOCK);
        int val = 1;
        setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR, &val,sizeof(val));
        memset(&serv, 0, sizeof(struct sockaddr_in));
        serv.sin_family = AF_INET;
        serv.sin_port = htons(port);
        serv.sin_addr.s_addr = inet_addr("0.0.0.0");
        if (bind(sockfd, (struct sockaddr*)&serv, sizeof(struct sockaddr_in)) ==
            -1) {
            perror("TcpConnectionSet - bind");
            exit(3);
        }

        // Now server is ready to listen and verification
        if ((listen(sockfd, 5)) != 0) {
            perror("Listen failed...");
            exit(6);
        } else if (debug)
            dbg("Server listening..\n");
	}


void TcpConnectionSetAccept::loop ()  {
		if (state == DISCONNECTED){
            socklen_t l = sizeof(client);
			int connfd = accept(sockfd, (struct sockaddr*)&client, &l);
	        fcntl(sockfd, F_SETFL, O_NONBLOCK);
			if (connfd < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) return;
				return;
			} else if (debug) {
				dbg("server accept the client...\n");
			}
			connection.connfd = connfd;
			onConnectionChange(connection,true);
			state = ESTABLISHED;
		}
	}


TcpConnectionSetAccept::~TcpConnectionSetAccept() {
	if (sockfd > 0) { close(sockfd); sockfd = -1; }
	if  (connection.connfd > 0) { close(connection.connfd) ; connection.connfd = -1; }
}




TcpReceiver::TcpReceiver(ConnectionSet * _cs) {
	cs = _cs ;
    active = true;
    state = UNKNOWN;
}

void TcpReceiver::printMessage(const char* hdr, uint8_t* msg, int len) {
        dbg("%s\n", hdr);
        for (int i = 0; i < len / 8 + 1; i++) {
            dbg("     ");
            for (int j = 0; j < 8; j++) {
                if (i * 8 + j < len) {
                    dbg("%02x ", (int)msg[i * 8 + j]);
                }
            }
            dbg("\n");
        }
    }


void TcpReceiver::loop() {
     char buffer[2048];

     uint8_t message[1000];
     uint8_t* p = message;
     uint8_t t;
     uint16_t len = 0;
     // Accept the data packet from client and verification
     while (cs->read(&t, 1) == 1) {
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
                             cs->read(&b1, 1);
                             cs->read(&b2, 1);
                             len = b1 + (((uint16_t)b2) << 8);
                             if (debug)
                                 dbg("received message with length %d\n",
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
                                 cs->next();
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
                                 cs->next();
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
 }


TcpReceiver::~TcpReceiver() {
	delete cs;
}


uint8_t* TslReceiver::onDmsg(uint8_t* msg, int len) {
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

void TslReceiver::onMessage(uint8_t* msg, int len)  {
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


TcpConnectionSetClient::TcpConnectionSetClient(int _screen, int _index,  const char * server, int port ) {
	connfd = 0;
    memset(&serv, 0, sizeof(struct sockaddr_in));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    serv.sin_addr.s_addr = inet_addr(server);
    connfd = 0;
    sockfd = 0;
    index = _index;
    screen = _screen;
    tv_disconnect = time(NULL);

}

TcpConnectionSetClient::~TcpConnectionSetClient() {
	close (sockfd);
}



void TcpConnectionSetClient::loop() {
	if (state == WAIT_RECONNECT){
		time_t now = time(NULL);
		if  (now - tv_disconnect > RECONNECT_TMO_SEC){
			state = DISCONNECTED;
		}
	}
	if (state == DISCONNECTED) {
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    if (sockfd < 0) {
	        perror("sock");
	        exit(1);
	    }
	    fcntl(sockfd, F_SETFL, O_NONBLOCK);
	    state = ESTABLISHING;
	}
	if (state == ESTABLISHING){
		int ret = connect(sockfd, (struct sockaddr*) &serv, sizeof(serv));
		if (ret < 0) {
			if (errno == EINPROGRESS || errno == EALREADY){
				// this is the good case
			} else if (errno == ECONNREFUSED || errno == ENOTCONN){
                dbg("connection refused\n");
                state = WAIT_RECONNECT;
        		close(sockfd);
        		tv_disconnect = time(NULL);
			}
			else  if (errno != EISCONN) {
				perror ("could not connect - 1 ");
			}
            else if (errno == EISCONN)
            {
                dbg("connected  fd %d\n", (int)sockfd);
                state = ESTABLISHED;
                char identity[45];
                snprintf(identity,sizeof(identity),"{\"screen\":%d, \"index\":%d}\n",screen,index);
                write(sockfd,identity,strlen(identity));
            }
		} else {
			dbg("connected  fd %d\n", (int)sockfd);
			state = ESTABLISHED;
            char identity[45];
            snprintf(identity,sizeof(identity),"{\"screen\":%d, \"index\":%d}\n",screen,index);
			write(sockfd,identity,strlen(identity));
		}
	}
}

int TcpConnectionSetClient::read(uint8_t * buf, int len){
	bool disconnected = false;
	int ret = -1;
	if (state == ESTABLISHED){
		ret = ::read(sockfd,buf,len);
		if (ret < 0) {
			if (!(errno == EAGAIN || errno == EWOULDBLOCK) ) {
				disconnected = true;
			}
		}
		if (ret == 0) {
			disconnected = true;
		}
	}
	if (disconnected){
		state = WAIT_RECONNECT;
		tv_disconnect = time(NULL);
		printf ("disconnect for %d - try to reconnect \n",sockfd);
		close(sockfd);
		perror("disconnected ");
	}
	return ret;
}
