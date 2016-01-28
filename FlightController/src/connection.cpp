/*
 * Connection.cpp
 *
 *  Created on: 20 Sep 2015
 *      Author: kp
 */

#include "connection.h"

using namespace std;

Connection::Connection() {
}

void Connection::AddOnRecvListener(OnRecvListener listener) {
	recv_listeners_.push_back(listener);
}

void Connection::AddErrorHandler(ErrorHandler handler) {
	error_handlers_.push_back(handler);
}
