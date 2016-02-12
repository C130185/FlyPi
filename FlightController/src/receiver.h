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

#include "connection.h"
#include "UI.h"

class Receiver {
public:
	enum Mode {
		JoyStick, Wifi
	};
	typedef std::function<void()> StartCmdListener;
	typedef std::function<void()> StopCmdListener;
	typedef std::function<void()> LostConnListener;

	Receiver(std::unique_ptr<Connection> conn, UI* ui);
	int get_throttle();
	int get_roll();
	int get_pitch();
	int get_yaw();
	int Start();
	void Stop();
	void Reset();
	void AddStartCmdListener(StartCmdListener startCmdListener);
	void AddStopCmdListener(StopCmdListener stopCmdListener);
	void AddLostConnListener(LostConnListener lostConnListener);
private:
	long last_packet_recv_;
	const std::unique_ptr<Connection> conn_;
	int throttle_, roll_, pitch_, yaw_; // -100 to 100
	std::vector<StartCmdListener> start_cmd_listeners_;
	std::vector<StopCmdListener> stop_cmd_listeners_;
	UI* ui_;

	void OnCtrl(Connection::ControlPacket);
	void OnCmd(Connection::CommandPacket);
	void OnError(int errornum);
};

#endif /* RECEIVER_H_ */
