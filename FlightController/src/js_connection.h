/*
 * ds4_connection.h
 *
 *  Created on: Feb 10, 2016
 *      Author: teoko
 */

#ifndef JS_CONNECTION_H_
#define JS_CONNECTION_H_

#include <asm-generic/int-ll64.h>
#include <thread>

#include "connection.h"

class JSConnection: public Connection {
public:
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

	JSConnection();
	int Start();
	void Stop();
private:
	enum State {
		Running, Stopping, Ready, Created
	};

	int ds4fd = 0;
	State state_;
	std::thread bgthread_;
	CommandPacket cmd_packet_;
	ControlPacket ctrl_packet_;

	void ReadJSE();
};

#endif /* JS_CONNECTION_H_ */
