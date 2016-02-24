/*
 * js_connection.h
 *
 *  Created on: Feb 10, 2016
 *      Author: teoko
 */

#ifndef JSCONNECTION_H_
#define JSCONNECTION_H_

#include <asm-generic/int-ll64.h>
#include <joystick.h>
#include <functional>
#include <thread>
#include <vector>

#include "Connection.h"

class JsConnection: public Connection {
public:
	typedef std::function<void(js_event)> JsEventListener;

	static const __u8 BTN_SQUARE = 0;
	static const __u8 BTN_CROSS = 1;
	static const __u8 BTN_CIRCLE = 2;
	static const __u8 BTN_TRIANGLE = 3;
	static const __u8 BTN_L1 = 4;
	static const __u8 BTN_R1 = 5;
	static const __u8 BTN_OPTION = 9;
	static const __u8 BTN_SHARE = 8;
	static const __u8 BTN_L3 = 10;
	static const __u8 BTN_R3 = 11;
	static const __u8 AXIS_DPAD_UP_DOWN = 7;
	static const __u8 AXIS_DPAD_LEFT_RIGHT = 6;
	static const __u8 AXIS_LEFT_ANALOG_X = 0;
	static const __u8 AXIS_LEFT_ANALOG_Y = 1;
	static const __u8 AXIS_RIGHT_ANALOG_X = 2;
	static const __u8 AXIS_RIGHT_ANALOG_Y = 5;
	static const __u8 AXIS_L3 = 3;
	static const __u8 AXIS_R3 = 4;

	JsConnection();
	int start();
	void stop();
	void addJSEventListener(JsEventListener jse);
private:
	enum State {
		RUNNING, STOPPING, READY, CREATED
	};

	int fd = 0;
	std::chrono::system_clock::time_point lastRecvTime =
			std::chrono::system_clock::now();
	State state;
	std::thread bgThread;
	CommandPacket cmdPacket;
	ControlPacket ctrlPacket;
	std::vector<JsEventListener> jseListeners;

	void readJsEvent();
};

#endif /* JSCONNECTION_H_ */
