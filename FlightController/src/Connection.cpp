/*
 * Connection.cpp
 *
 *  Created on: 20 Sep 2015
 *      Author: kp
 */

#include "Connection.h"

using namespace std;

Connection::Connection() {
}

void Connection::addCtrlListener(CtrlListener listener) {
	ctrlListeners.push_back(listener);
}

void Connection::addCmdListener(CmdListener listener) {
	cmdListeners.push_back(listener);
}

void Connection::addLostConnListener(LostConnListener listener) {
	lostConnListeners.push_back(listener);
}

void Connection::addErrorHandler(ErrorHandler handler) {
	errorHandlers.push_back(handler);
}
