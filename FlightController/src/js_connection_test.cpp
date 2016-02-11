/*
 * ds4_connection_test.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: teoko
 */

#include <iostream>
#include <memory>
#include <string>

#include "js_connection.h"
#include "receiver.h"

using namespace std;

Receiver jsreceiver(unique_ptr<JSConnection>(new JSConnection()));

void OnEvent() {
	cout << to_string(jsreceiver.get_throttle()) << "\n";
}

int test() {
	jsreceiver.AddStartCmdListener(&OnEvent);
	jsreceiver.AddStopCmdListener(&OnEvent);
	jsreceiver.Start();

	cin.ignore();
	return 1;
}
