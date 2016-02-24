/*
 * js_connection.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: teoko
 */

#include <fcntl.h>
#include <joystick.h>
#include <unistd.h>
#include <chrono>
#include <limits>
#include <vector>
#include "JsConnection.h"

using namespace std;

JsConnection::JsConnection() {
	state = CREATED;
}

int JsConnection::start() {
	fd = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
	state = RUNNING;

	bgThread = thread(&JsConnection::readJsEvent, this);
	return 0;
}

void JsConnection::stop() {
	state = STOPPING;
	bgThread.join();
}

void JsConnection::addJSEventListener(JsEventListener jse) {
	jseListeners.push_back(jse);
}

void JsConnection::readJsEvent() {
	struct js_event jse;

	while (state == RUNNING) {
		auto now = chrono::system_clock::now();
		auto delta = chrono::duration_cast<chrono::milliseconds>(
				now - lastRecvTime).count();
		if (delta >= 200) {
			for (LostConnListener listener : lostConnListeners) {
				listener();
			}
		}

		int result = read(fd, &jse, sizeof(jse));
		if (result == -1) {
			this_thread::sleep_for(chrono::milliseconds(10));
			continue;
		}

		lastRecvTime = std::chrono::system_clock::now();
		for (JsEventListener listener : jseListeners) {
			listener(jse);
		}

		__u8 type = jse.type;
		if (type == JS_EVENT_BUTTON) {
			if (jse.value == 1) {
				switch (jse.number) {
				case BTN_OPTION:
					if (cmdPacket.cmd == COMMAND_STOP) {
						cmdPacket.cmd = COMMAND_START;
					} else {
						cmdPacket.cmd = COMMAND_STOP;
					}
					cmdPacket.time = jse.time;
					for (CmdListener listener : cmdListeners) {
						listener(cmdPacket);
					}
					break;
				}
				cmdPacket.time = jse.time;
			}
		}
		if (type == JS_EVENT_AXIS) {
			signed char value = ((float) jse.value
					/ numeric_limits<__s16 >::max()) * 100;
			switch (jse.number) {
			case AXIS_LEFT_ANALOG_X:
				ctrlPacket.yaw = value;
				break;
			case AXIS_LEFT_ANALOG_Y:
				ctrlPacket.throttle = -value;
				break;
			case AXIS_RIGHT_ANALOG_X:
				ctrlPacket.roll = value;
				break;
			case AXIS_RIGHT_ANALOG_Y:
				ctrlPacket.pitch = -value;
				break;
			}
			ctrlPacket.time = jse.time;
			for (CtrlListener listener : ctrlListeners) {
				listener(ctrlPacket);
			}
		}
	}
	close(fd);
}
