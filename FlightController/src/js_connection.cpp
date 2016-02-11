/*
 * ds4_connection.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: teoko
 */

#include "js_connection.h"

#include <fcntl.h>
#include <joystick.h>
#include <unistd.h>
#include <chrono>
#include <limits>

using namespace std;

JSConnection::JSConnection() {
	state_ = Created;
}

int JSConnection::Start() {
	ds4fd = open("/dev/input/js0", O_RDONLY);
	state_ = Running;

	bgthread_ = thread(&JSConnection::ReadJSE, this);
	return 0;
}

void JSConnection::Stop() {
	state_ = Stopping;
}

void JSConnection::ReadJSE() {
	struct js_event jse;
	while (state_ == Running) {
		read(ds4fd, &jse, sizeof(jse));

		__u8 type = jse.type;
		if (type == JS_EVENT_BUTTON) {
			if (jse.value == 1) {
				switch (jse.number) {
				case BTN_OPTION:
					if (cmd_packet_.cmd == kCommandStop) {
						cmd_packet_.cmd = kCommandStart;
					} else {
						cmd_packet_.cmd = kCommandStop;
					}
					cmd_packet_.time = jse.time;
					for (CmdListener listener : cmd_listeners_) {
						listener(cmd_packet_);
					}
					break;
				}
				cmd_packet_.time = jse.time;
			}
		}
		if (type == JS_EVENT_AXIS) {
			signed char value = ((float) jse.value
					/ numeric_limits<__s16 >::max()) * 100;
			switch (jse.number) {
			case AXIS_LEFT_ANALOG_X:
				ctrl_packet_.yaw = value;
				break;
			case AXIS_LEFT_ANALOG_Y:
				ctrl_packet_.throttle = -value;
				break;
			case AXIS_RIGHT_ANALOG_X:
				ctrl_packet_.roll = value;
				break;
			case AXIS_RIGHT_ANALOG_Y:
				ctrl_packet_.pitch = -value;
				break;
			}
			ctrl_packet_.time = jse.time;
			for (CtrlListener listener : ctrl_listeners_) {
				listener(ctrl_packet_);
			}
		}
		this_thread::sleep_for(chrono::milliseconds(10));
	}
	close(ds4fd);
}
