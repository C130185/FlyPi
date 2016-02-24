/*
 * Receiver.cpp
 *
 *  Created on: 20 Sep 2015
 *      Author: kp
 */

#include <algorithm>
#include <cstring>
#include <exception>
#include <sstream>

#include "NCurseUI.h"
#include "Receiver.h"

using namespace std;

Receiver::Receiver(unique_ptr<Connection> conn, UI* ui) :
		lastPacketRecv(0), Conn(move(conn)), throttle(0), roll(0), pitch(
				0), yaw(0), ui(ui) {
	Conn->addCmdListener(
			[this](Connection::CommandPacket cmdPacket) {this->OnCmd(cmdPacket);});
	Conn->addCtrlListener(
			[this](Connection::ControlPacket ctrlPacket) {this->OnCtrl(ctrlPacket);});
	Conn->addErrorHandler([this](int errornum) {this->OnError(errornum);});
}

void Receiver::addStartCmdListener(StartCmdListener startCmdListener) {
	startCmdListeners.push_back(startCmdListener);
}

void Receiver::addStopCmdListener(StopCmdListener stopCmdListener) {
	stopCmdListeners.push_back(stopCmdListener);
}

void Receiver::addLostConnListener(LostConnListener lostConnListener) {
	Conn->addLostConnListener(lostConnListener);
}

int Receiver::start() {
	return Conn->start();
}

void Receiver::stop() {
	Conn->stop();
}

void Receiver::reset() {
	throttle = 0;
	roll = 0;
	pitch = 0;
	yaw = 0;
}

int Receiver::getThrottle() {
	return throttle;
}

int Receiver::getRoll() {
	return roll;
}

int Receiver::getPitch() {
	return pitch;
}

int Receiver::getYaw() {
	return yaw;
}

void Receiver::OnCtrl(Connection::ControlPacket ctrlPacket) {
	if (lastPacketRecv > ctrlPacket.time) { // Discard old packets
		ui->print("Discarding unordered packet received\n");
		return;
	}
	throttle = ctrlPacket.throttle;
	roll = ctrlPacket.roll;
	pitch = ctrlPacket.pitch;
	yaw = ctrlPacket.yaw;
	if (throttle > 100)
		throttle = 100;
	if (throttle < -100)
		throttle = -100;
	if (roll > 100)
		roll = 100;
	if (roll < -100)
		roll = -100;
	if (pitch > 100)
		pitch = 100;
	if (pitch < -100)
		pitch = -100;
	if (yaw > 100)
		yaw = 100;
	if (yaw < -100)
		yaw = -100;
}

void Receiver::OnCmd(Connection::CommandPacket cmdPacket) {
	switch (cmdPacket.cmd) {
	case Connection::COMMAND_START:
		for (StartCmdListener start_listener : startCmdListeners) {
			start_listener();
		}
		break;
	case Connection::COMMAND_STOP:
		for (StopCmdListener stop_listener : stopCmdListeners) {
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
		ui->print(oss.str());
	}
}
