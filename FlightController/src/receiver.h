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

class Receiver {
public:
	static const char kCommandStart = 1;
	static const char kCommandStop = 2;

	typedef std::function<void(char cmd)> CmdListener;
	typedef std::function<void()> LostConnListener;

	Receiver(std::unique_ptr<Connection> conn);
	int get_throttle();
	int get_roll();
	int get_pitch();
	int get_yaw();
	int Start();
	void Stop();
	void Reset();
	void AddOnCmdListener(CmdListener cmdListener);
	void AddOnLostConnListener(LostConnListener lostConnListener);
private:
	static const char kPacketTypeControl = 1;
	static const char kPacketTypeCommand = 2;

	struct ControlPacket;
	struct CommandPacket;
	long last_packet_recv_;
	const std::unique_ptr<Connection> conn_;
	int throttle_, roll_, pitch_, yaw_; // -100 to 100
	std::vector<CmdListener> cmd_listeners_;
	std::vector<LostConnListener> lost_conn_listeners_;

	void OnMsgRecv(char* msg, unsigned int length);
	void OnError(int errornum);
};

#endif /* RECEIVER_H_ */
