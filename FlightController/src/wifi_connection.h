/*
 * wificonnection.h
 *
 *  Created on: 16 Sep 2015
 *      Author: kp
 */

#ifndef WIFI_CONNECTION_H_
#define WIFI_CONNECTION_H_

#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>

#include "connection.h"

class WifiConnection: public Connection {
public:
	static const int kBufferLength = 256;

	WifiConnection(int port);
	virtual ~WifiConnection();
	int Start();
	void Stop();
private:
	enum State {
		Running, Stopping, Ready, Created
	};

	State state_;
	int sockfd_, port_;
	char buffer_[kBufferLength];
	std::thread recv_thread_;

	void DoRecv();
	void HandleErrors(int errornum);
};

#endif /* WIFI_CONNECTION_H_ */
