/*
 * Receiver.cpp
 *
 *  Created on: 20 Sep 2015
 *      Author: kp
 */

#include "receiver.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <sstream>

#include "ui.h"

using namespace std;

struct Receiver::ControlPacket {
	long long time;
	signed char type;
	signed char throttle;
	signed char roll;
	signed char pitch;
	signed char yaw;
};

struct Receiver::CommandPacket {
	long long time;
	signed char type;
	signed char cmd;
};

Receiver::Receiver(unique_ptr<Connection> conn) :
		last_packet_recv_(0), conn_(move(conn)), throttle_(0), roll_(0), pitch_(
				0), yaw_(0) {
	conn_->AddOnRecvListener(
			[this](char* msg, unsigned int length) {this->OnMsgRecv(msg, length);});
	conn_->AddErrorHandler([this](int errornum) {this->OnError(errornum);});
}

void Receiver::AddOnCmdListener(CmdListener cmdListener) {
	cmd_listeners_.push_back(cmdListener);
}

void Receiver::AddOnLostConnListener(LostConnListener lostConnListener) {
	lost_conn_listeners_.push_back(lostConnListener);
}

int Receiver::Start() {
	return conn_->Start();
}

void Receiver::Stop() {
	conn_->Stop();
}

void Receiver::Reset() {
	throttle_ = 0;
	roll_ = 0;
	pitch_ = 0;
	yaw_ = 0;
}

int Receiver::get_throttle() {
	return throttle_;
}

int Receiver::get_roll() {
	return roll_;
}

int Receiver::get_pitch() {
	return pitch_;
}

int Receiver::get_yaw() {
	return yaw_;
}

void Receiver::OnMsgRecv(char* msg, unsigned int length) {
	try {
		long long time = *((long long*) msg);
		if (last_packet_recv_ > time) { // Discard old packets
			UI::Print("Discarding unordered packet received\n");
			return;
		}
		last_packet_recv_ = time;

		char type = msg[8];
		ostringstream oss;
		switch (type) {
		case kPacketTypeControl:
			ControlPacket control_packet;
			control_packet.throttle = msg[9];
			control_packet.roll = msg[10];
			control_packet.pitch = msg[11];
			control_packet.yaw = msg[12];
			throttle_ = control_packet.throttle;
			roll_ = control_packet.roll;
			pitch_ = control_packet.pitch;
			yaw_ = control_packet.yaw;
			if (throttle_ > 100)
				throttle_ = 100;
			if (throttle_ < -100)
				throttle_ = -100;
			if (roll_ > 100)
				roll_ = 100;
			if (roll_ < -100)
				roll_ = -100;
			if (pitch_ > 100)
				pitch_ = 100;
			if (pitch_ < -100)
				pitch_ = -100;
			if (yaw_ > 100)
				yaw_ = 100;
			if (yaw_ < -100)
				yaw_ = -100;
			break;
		case kPacketTypeCommand:
			CommandPacket cmd_packet;
			cmd_packet.cmd = msg[9];
			for (CmdListener cmd_listener : cmd_listeners_) {
				cmd_listener(cmd_packet.cmd);
			}
			break;
		default:
			UI::Print("Unknown packet type received\n");
			break;
		}
	} catch (exception& e) {
		ostringstream oss;
		oss << "Error: " << e.what() << endl;
		UI::Print(oss.str());
	}
}

void Receiver::OnError(int errornum) {
	if (errornum != EAGAIN && errornum != EWOULDBLOCK) {
		ostringstream oss;
		oss << "Error: " << strerror(errornum) << " (" << errornum << ")"
				<< endl;
		UI::Print(oss.str());
	} else {
		for (LostConnListener lost_conn_listener : lost_conn_listeners_) {
			lost_conn_listener();
		}
	}
}
