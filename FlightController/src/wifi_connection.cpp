/*
 * wificonnection.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: kp
 */

#include "wifi_connection.h"

#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

using namespace std;

WifiConnection::WifiConnection(int port) :
		state_(Created), port_(port), buffer_ { 0 } {
	sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd_ < 0) {
		HandleErrors(errno);
		return;
	}
}

WifiConnection::~WifiConnection() {
	Stop();
}

int WifiConnection::Start() { // return -1 if error, else return 0
	sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port_);
	timeval timeo;
	timeo.tv_sec = 0;
	timeo.tv_usec = 100000;
	setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));

	if (bind(sockfd_, (sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		HandleErrors(errno);
		return -1;
	}

	recv_thread_ = thread(&WifiConnection::DoRecv, this);
	state_ = Running;
	return 0;
}

void WifiConnection::Stop() {
	if (state_ == Running) {
		state_ = Stopping;
		recv_thread_.detach();
	}
}

void WifiConnection::DoRecv() {
	while (state_ != Stopping) {
		int n = recv(sockfd_, buffer_, kBufferLength, 0);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				for (LostConnListener listener : lost_conn_listeners_) {
					listener();
				}
			} else {
				HandleErrors(errno);
			}
			continue;
		}
		char type = buffer_[8];
		switch (type) {
		case kPacketTypeCommand:
			memcpy(&cmdPacket_, buffer_, sizeof(cmdPacket_));
			for (CmdListener listener : cmd_listeners_) {
				listener(cmdPacket_);
			}
			break;
		case kPacketTypeControl:
			memcpy(&ctrlPacket_, buffer_, sizeof(ctrlPacket_));
			for (CtrlListener listener : ctrl_listeners_) {
				listener(ctrlPacket_);
			}
			break;
		}
	}
	close(sockfd_);
}

void WifiConnection::HandleErrors(int errornum) {
	for (ErrorHandler handler : error_handlers_) {
		handler(errornum);
	}
}
