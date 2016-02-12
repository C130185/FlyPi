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

#include "NCurseUI.h"

using namespace std;

Receiver::Receiver(unique_ptr<Connection> conn, UI* ui) :
		last_packet_recv_(0), conn_(move(conn)), throttle_(0), roll_(0), pitch_(
				0), yaw_(0), ui_(ui) {
	conn_->AddCmdListener(
			[this](Connection::CommandPacket cmdPacket) {this->OnCmd(cmdPacket);});
	conn_->AddCtrlListener(
			[this](Connection::ControlPacket ctrlPacket) {this->OnCtrl(ctrlPacket);});
	conn_->AddErrorHandler([this](int errornum) {this->OnError(errornum);});
}

void Receiver::AddStartCmdListener(StartCmdListener startCmdListener) {
	start_cmd_listeners_.push_back(startCmdListener);
}

void Receiver::AddStopCmdListener(StopCmdListener stopCmdListener) {
	stop_cmd_listeners_.push_back(stopCmdListener);
}

void Receiver::AddLostConnListener(LostConnListener lostConnListener) {
	conn_->AddLostConnListener(lostConnListener);
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

void Receiver::OnCtrl(Connection::ControlPacket ctrlPacket) {
	if (last_packet_recv_ > ctrlPacket.time) { // Discard old packets
		ui_->Print("Discarding unordered packet received\n");
		return;
	}
	throttle_ = ctrlPacket.throttle;
	roll_ = ctrlPacket.roll;
	pitch_ = ctrlPacket.pitch;
	yaw_ = ctrlPacket.yaw;
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
}

void Receiver::OnCmd(Connection::CommandPacket cmdPacket) {
	switch (cmdPacket.cmd) {
	case Connection::kCommandStart:
		for (StartCmdListener start_listener : start_cmd_listeners_) {
			start_listener();
		}
		break;
	case Connection::kCommandStop:
		for (StopCmdListener stop_listener : stop_cmd_listeners_) {
			stop_listener();
		}
		break;
	}
}

void Receiver::OnError(int errornum) {
	if (errornum != EAGAIN && errornum != EWOULDBLOCK) {
		ostringstream oss;
		oss << "Error: " << strerror(errornum) << " (" << errornum << ")"
				<< endl;
		ui_->Print(oss.str());
	}
}
