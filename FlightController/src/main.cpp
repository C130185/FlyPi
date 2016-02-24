/*
 * main.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: kp
 */

#include <getopt.h>
#include <IMU.h>
#include <ncurses.h>
#include <pigpio.h>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "DummyLogger.h"
#include "DummyUI.h"
#include "JsConnection.h"
#include "Logger.h"
#include "NCurseUI.h"
#include "Receiver.h"
#include "WifiConnection.h"

using namespace std;

enum State {
	RUNNING, STOP
};

static const int ESC_RATE = 400; // hz
static const float MAX_MOTOR = 0.8;
static const float MAX_THROTTLE = 2; // m/s
static const float MAX_ROLLPITCH = 10; // deg
static const float MAX_YAW = 45; // deg/s
static const float THROTTLE_THRESHOLD = 0.2; // m/s
static const float THROTTLE_P = 0.2;
static const float THROTTLE_D = 0.12;
static const float ROLLPITCH_P = 0.003;
static const float ROLLPITCH_D = 0.001;
static const float YAW_P = 0.001;

bool g_jsMode = false;
bool g_dummyUi = false;
bool g_dummyLogger = true;
State g_state = STOP;
array<short, 4> g_motorGpio = { 23, 18, 15, 14 };
array<float, 4> g_motorSpeed = { 0, 0, 0, 0 };
float g_hover = 0;
Receiver* g_receiver;
IMU g_imu;
UI* g_ui;
ILogger* g_logger;
float g_targetThrottle;
float g_targetRoll;
float g_targetPitch;
float g_targetYaw;

void onexit() {
	g_ui->end();
	delete g_receiver;
	delete g_ui;
	gpioTerminate();
}

void uiLoop() {
	while (true) {
		ostringstream oss;

		// print motors
		int offset = [](string s) -> int {return s.length();}("Motor X: ");
		int n = COLS / 4 - offset;
		oss.setf(oss.left);
		oss.precision(3);
		oss << "Motor 1: " << setw(n) << g_motorSpeed[0] * 100;
		oss << "Motor 2: " << setw(n) << g_motorSpeed[1] * 100;
		oss << "Motor 3: " << setw(n) << g_motorSpeed[2] * 100;
		oss << "Motor 4: " << g_motorSpeed[3] * 100 << "\n\n";

		// print imu
		array<float, 3> angular_pos = g_imu.getAngularVelocity();
		array<float, 3> linear_velocity = g_imu.getLinVelocity();

		oss << setw(COLS / 2) << "Angular Position (deg)";
		oss << "Linear Velocities (m/s)\n";
		offset = [](string s) -> int {return s.length();}("Roll: ");
		n = COLS / 2 - offset;
		oss << "Roll: " << setw(n) << angular_pos[0];
		oss << "X: " << linear_velocity[0] << endl;
		offset = [](string s) -> int {return s.length();}("Pitch: ");
		n = COLS / 2 - offset;
		oss << "Pitch: " << setw(n) << angular_pos[1];
		oss << "Y: " << linear_velocity[1] << endl;
		offset = [](string s) -> int {return s.length();}("Yaw: ");
		n = COLS / 2 - offset;
		oss << "Yaw: " << setw(n) << angular_pos[2];
		oss << "Z: " << linear_velocity[2] << "\n\n";

		oss << "Targets\n";
		offset = [](string s) -> int {return s.length();}("Throttle: ");
		n = COLS / 4 - offset;
		oss << "Throttle: " << setw(n) << g_targetThrottle;
		offset = [](string s) -> int {return s.length();}("Roll: ");
		n = COLS / 4 - offset;
		oss << "Roll: " << setw(n) << g_targetRoll;
		offset = [](string s) -> int {return s.length();}("Pitch: ");
		n = COLS / 4 - offset;
		oss << "Pitch: " << setw(n) << g_targetPitch;
		offset = [](string s) -> int {return s.length();}("Yaw: ");
		n = COLS / 4 - offset;
		oss << "Yaw: " << setw(n) << g_targetYaw;

		g_ui->updateStat(oss.str());
		this_thread::sleep_for(chrono::milliseconds(100));
	}
}

void log(string msg) {
	g_logger->write(msg);
}

inline float max(float a, float b) {
	return a >= b ? a : b;
}

inline float min(float a, float b) {
	return a <= b ? a : b;
}

void resetMotors() {
	g_hover = 0;
	g_motorSpeed[0] = 0;
	g_motorSpeed[1] = 0;
	g_motorSpeed[2] = 0;
	g_motorSpeed[3] = 0;
	gpioPWM(g_motorGpio[0], g_motorSpeed[0] * 1000 + 1000);
	gpioPWM(g_motorGpio[1], g_motorSpeed[1] * 1000 + 1000);
	gpioPWM(g_motorGpio[2], g_motorSpeed[2] * 1000 + 1000);
	gpioPWM(g_motorGpio[3], g_motorSpeed[3] * 1000 + 1000);
}

void onStart() {
	g_state = RUNNING;
	g_ui->print("Received START signal\n");
	if (g_logger->isOpen()) {
		g_logger->close();
	}
	g_logger->open("angrate");
}

void onStop() {
	g_state = STOP;
	g_ui->print("STOP signal received\n");
	resetMotors();
	g_receiver->reset();
	g_logger->close();
	g_ui->print("Waiting for START signal\n");
}

void onLostConn() {
	if (g_state == RUNNING) {
		g_ui->print("E-STOP: Lost connection\n");
		resetMotors();
		g_receiver->reset();
		g_state = STOP;
		g_ui->print("Waiting for START signal\n");
	}
}

void parseArgs(int argc, char* argv[]) {
	int c;
	while ((c = getopt(argc, argv, "jbl")) != -1) {
		switch (c) {
		case 'j':
			g_jsMode = true;
			break;
		case 'b':
			g_dummyUi = true;
			break;
		case 'l':
			g_dummyLogger = false;
			break;
		}
	}
}

int main(int argc, char* argv[]) {
	parseArgs(argc, argv);
	if (g_dummyUi) {
		g_ui = DummyUI::getInstance();
	} else {
		g_ui = NCurseUI::getInstance();
	}
	if (g_jsMode) {
		g_receiver = new Receiver(unique_ptr<Connection>(new JsConnection()),
				g_ui);
	} else {
		g_receiver = new Receiver(
				unique_ptr<Connection>(new WifiConnection(43123)), g_ui);
	}
	if (g_dummyLogger) {
		g_logger = new DummyLogger();
	} else {
		g_logger = new Logger();
	}

	if (gpioInitialise() < 0) {
		cout << "Error initializing gpio!" << endl;
		return (-1);
	}
	atexit(onexit);
	gpioSetPWMfrequency(g_motorGpio[0], ESC_RATE);
	gpioSetPWMrange(g_motorGpio[0], 1000000 / ESC_RATE);
	gpioSetPWMfrequency(g_motorGpio[1], ESC_RATE);
	gpioSetPWMrange(g_motorGpio[1], 1000000 / ESC_RATE);
	gpioSetPWMfrequency(g_motorGpio[2], ESC_RATE);
	gpioSetPWMrange(g_motorGpio[2], 1000000 / ESC_RATE);
	gpioSetPWMfrequency(g_motorGpio[3], ESC_RATE);
	gpioSetPWMrange(g_motorGpio[3], 1000000 / ESC_RATE);

	if (!g_dummyUi) {
		g_ui->init();
		thread uiThread(&uiLoop);
	}

	int result = g_imu.init();
	if (!result) {
		cout << "Error starting IMU, error code=" << result << "\n";
		return -1;
	}
	g_imu.setRollInv(true);
	g_imu.setPitchInv(true);
	g_ui->print("Starting zero field calibration\n");
	result = g_imu.calibrateZeroFieldOffset();
	if (result < 0)
		g_ui->print(
				"IMU: Error when calibrating zero field offset, error code="
						+ to_string(result) + "\n");
	else
		g_ui->print("Finished zero field calibration\n");

	gpioPWM(g_motorGpio[0], 1000);
	gpioPWM(g_motorGpio[1], 1000);
	gpioPWM(g_motorGpio[2], 1000);
	gpioPWM(g_motorGpio[3], 1000);

	g_receiver->addStartCmdListener(&onStart);
	g_receiver->addStopCmdListener(&onStop);
	g_receiver->addLostConnListener(&onLostConn);
	g_receiver->start();

	auto curTime = chrono::high_resolution_clock::now();
	auto prevTime = curTime;

	g_ui->print("Waiting for START signal\n");

	while (true) {
		curTime = chrono::high_resolution_clock::now();
		float delta = chrono::duration_cast<chrono::milliseconds>(
				curTime - prevTime).count() / 1000.0;
		prevTime = curTime;

		g_imu.update();
		if (g_state == RUNNING) {
			array<float, 3> linVelocity = g_imu.getLinVelocity();
			array<float, 3> linAccel = g_imu.getLinAccel();
			array<float, 3> angPos = g_imu.getAngularPos();
			array<float, 3> angVelocity = g_imu.getAngularVelocity();

			if (abs(linVelocity[2]) < THROTTLE_THRESHOLD) {
				linVelocity[2] = 0;
				linAccel[2] = 0;
			}

			if (abs(angPos[0]) > 10 || abs(angPos[1] > 10)
					|| g_motorSpeed[0] > 0.25 || g_motorSpeed[1] > 0.25
					|| g_motorSpeed[2] > 0.25 || g_motorSpeed[3] > 0.25) {
				g_ui->print("E-STOP: Threshold exceeded\n");
				g_ui->print(
						"Roll: " + to_string(angPos[0]) + ", "
								+ to_string(angVelocity[0]) + "\n");
				g_ui->print(
						"Pitch: " + to_string(angPos[1]) + ", "
								+ to_string(angVelocity[1]) + "\n");
				g_ui->print(
						"Yaw: " + to_string(angPos[2]) + ", "
								+ to_string(angVelocity[2]) + "\n");
				g_ui->print(
						"Throttle: " + to_string(linVelocity[2]) + ", "
								+ to_string(linAccel[2]) + "\n");
				g_ui->print(
						"Motors: " + to_string(g_motorSpeed[0]) + ", "
								+ to_string(g_motorSpeed[1]) + ", "
								+ to_string(g_motorSpeed[2]) + ", "
								+ to_string(g_motorSpeed[3]) + "\n");
				onStop();
				continue;
			}

			g_targetThrottle = g_receiver->getThrottle() / 100.0 * MAX_THROTTLE;
			g_targetRoll = g_receiver->getRoll() / 100.0 * MAX_ROLLPITCH;
			g_targetPitch = g_receiver->getPitch() / 100.0 * MAX_ROLLPITCH;
			g_targetYaw = g_receiver->getYaw() / 100.0 * MAX_YAW;
			float errThrottle = g_targetThrottle - linVelocity[2];
			float errdThrottle = -linAccel[2];
			float errRoll = g_targetRoll - angPos[0];
			float errdRoll = -angVelocity[0];
			float errPitch = g_targetPitch - angPos[1];
			float errdPitch = -angVelocity[1];
			float errYaw = g_targetYaw - angVelocity[2];

//			// control velocity instead of ang pos when targets == 0
//			if (target_roll_ == 0 && target_pitch_ == 0) {
//				err_roll = lin_velocity[1] * 4 - ang_pos[0];
//				err_pitch = -lin_velocity[0] * 4 - ang_pos[1];
//			}

			float uz = (THROTTLE_P * errThrottle + THROTTLE_D * errdThrottle)
					* delta;
			float uroll = ROLLPITCH_P * errRoll + ROLLPITCH_D * errdRoll;
			float upitch = ROLLPITCH_P * errPitch + ROLLPITCH_D * errdPitch;
			float uyaw = YAW_P * errYaw;
//			hover_ += uz;
//			hover_ = max(min(hover_, kMaxMotor), 0);
			g_hover = g_receiver->getThrottle() / 100.0;
			uroll = max(min(uroll, 1 - MAX_MOTOR), -1 + MAX_MOTOR);
			upitch = max(min(upitch, 1 - MAX_MOTOR), -1 + MAX_MOTOR);
			uyaw = max(min(uyaw, 1 - MAX_MOTOR), -1 + MAX_MOTOR);
			g_motorSpeed[0] = max(min(g_hover + uroll - upitch + uyaw, 1), 0);
			g_motorSpeed[1] = max(min(g_hover - uroll - upitch - uyaw, 1), 0);
			g_motorSpeed[2] = max(min(g_hover - uroll + upitch + uyaw, 1), 0);
			g_motorSpeed[3] = max(min(g_hover + uroll + upitch - uyaw, 1), 0);
			gpioPWM(g_motorGpio[0], g_motorSpeed[0] * 1000 + 1000);
			gpioPWM(g_motorGpio[1], g_motorSpeed[1] * 1000 + 1000);
			gpioPWM(g_motorGpio[2], g_motorSpeed[2] * 1000 + 1000);
			gpioPWM(g_motorGpio[3], g_motorSpeed[3] * 1000 + 1000);

			stringstream ss;
			ss << angVelocity[0] << ",";
			ss << angVelocity[1] << ",";
			ss << angVelocity[2];
			g_logger->write(ss.str());
		}
//		this_thread::yield();
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}
