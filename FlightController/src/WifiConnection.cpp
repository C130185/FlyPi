/*
 * wificonnection.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: kp
 */

#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "WifiConnection.h"

using namespace std;

WifiConnection::WifiConnection(int port) :
		state(CREATED), port(port), buffer { 0 } {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		handleErrors(errno);
		return;
	}
}

WifiConnection::~WifiConnection() {
	stop();
}

int WifiConnection::start() { // return -1 if error, else return 0
	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(port);
	timeval timeo;
	timeo.tv_sec = 0;
	timeo.tv_usec = 100000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));

	if (bind(sockfd, (sockaddr*) &servAddr, sizeof(servAddr)) < 0) {
		handleErrors(errno);
		return -1;
	}

	recvThread = thread(&WifiConnection::doRecv, this);
	state = RUNNING;
	return 0;
}

void WifiConnection::stop() {
	if (state == RUNNING) {
		state = STOPPING;
		recvThread.join();
	}
}

void WifiConnection::doRecv() {
	while (state != STOPPING) {
		int n = recv(sockfd, buffer, BUFFER_LENGTH, 0);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				for (LostConnListener listener : lostConnListeners) {
					listener();
				}
			} else {
				handleErrors(errno);
			}
			continue;
		}
		char type = buffer[8];
		switch (type) {
		case PCK_TYPE_COMMAND:
			memcpy(&cmdPacket, buffer, sizeof(cmdPacket));
			for (CmdListener listener : cmdListeners) {
				listener(cmdPacket);
			}
			break;
		case PCK_TYPE_CONTROL:
			memcpy(&ctrlPacket, buffer, sizeof(ctrlPacket));
			for (CtrlListener listener : ctrlListeners) {
				listener(ctrlPacket);
			}
			break;
		}
	}
	close(sockfd);
}

void WifiConnection::handleErrors(int errornum) {
	for (ErrorHandler handler : errorHandlers) {
		handler(errornum);
	}
}
