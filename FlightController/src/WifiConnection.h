/*
 * wificonnection.h
 *
 *  Created on: 16 Sep 2015
 *      Author: kp
 */

#ifndef WIFICONNECTION_H_
#define WIFICONNECTION_H_

#include <thread>

#include "Connection.h"

class WifiConnection: public Connection {
public:
	static const int BUFFER_LENGTH = 256;

	WifiConnection(int port);
	virtual ~WifiConnection();
	int start();
	void stop();
private:
	enum State {
		RUNNING, STOPPING, READY, CREATED
	};

	State state;
	int sockfd, port;
	char buffer[BUFFER_LENGTH];
	std::thread recvThread;
	CommandPacket cmdPacket;
	ControlPacket ctrlPacket;

	void doRecv();
	void handleErrors(int errornum);
};

#endif /* WIFICONNECTION_H_ */
