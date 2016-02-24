/*
 * Receiver.h
 *
 *  Created on: 20 Sep 2015
 *      Author: kp
 */

#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <functional>
#include <memory>
#include <vector>

#include "Connection.h"
#include "UI.h"

class Receiver {
public:
	enum Mode {
		JOYSTICK, WIFI
	};
	typedef std::function<void()> StartCmdListener;
	typedef std::function<void()> StopCmdListener;
	typedef std::function<void()> LostConnListener;

	Receiver(std::unique_ptr<Connection> conn, UI* ui);
	int getThrottle();
	int getRoll();
	int getPitch();
	int getYaw();
	int start();
	void stop();
	void reset();
	void addStartCmdListener(StartCmdListener startCmdListener);
	void addStopCmdListener(StopCmdListener stopCmdListener);
	void addLostConnListener(LostConnListener lostConnListener);
private:
	long lastPacketRecv;
	const std::unique_ptr<Connection> Conn;
	int throttle, roll, pitch, yaw; // -100 to 100
	std::vector<StartCmdListener> startCmdListeners;
	std::vector<StopCmdListener> stopCmdListeners;
	UI* ui;

	void OnCtrl(Connection::ControlPacket);
	void OnCmd(Connection::CommandPacket);
	void OnError(int errornum);
};

#endif /* RECEIVER_H_ */
