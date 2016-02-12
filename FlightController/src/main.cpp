/*
 * main.cpp
 *
 *  Created on: 16 Sep 2015
 *      Author: kp
 */

#include <getopt.h>
#include <imu.h>
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

#include "DummyUI.h"
#include "js_connection.h"
#include "NCurseUI.h"
#include "receiver.h"
#include "wifi_connection.h"

using namespace std;

enum State {
	RUNNING, STOP
};

static const int kEscRate = 400; // hz
static const float kMaxMotor = 0.8;
static const float kMaxThrottle = 2; // m/s
static const float kMaxRollPitch = 10; // deg
static const float kMaxYaw = 45; // deg/s
static const float kThrottleThreshold = 0.2; // m/s
static const float kPThrotle = 0.2;
static const float kDThrottle = 0.12;
static const float kPRollPitch = 0.003;
static const float kDRollPitch = 0.001;
static const float kPYaw = 0.001;

bool js_mode_ = false;
bool dummy_ui_ = false;
State state_ = STOP;
array<short, 4> motor_gpio_ = { 23, 18, 15, 14 };
array<float, 4> motor_speed_ = { 0, 0, 0, 0 };
float hover_ = 0;
Receiver* receiver;
IMU imu;
UI* ui_;
float target_throttle_;
float target_roll_;
float target_pitch_;
float target_yaw_;

void onexit() {
	delete receiver;
	ui_->End();
	gpioTerminate();
}

void UILoop() {
	while (true) {
		ostringstream oss;

		// print motors
		int offset = [](string s) -> int {return s.length();}("Motor X: ");
		int n = COLS / 4 - offset;
		oss.setf(oss.left);
		oss.precision(3);
		oss << "Motor 1: " << setw(n) << motor_speed_[0] * 100;
		oss << "Motor 2: " << setw(n) << motor_speed_[1] * 100;
		oss << "Motor 3: " << setw(n) << motor_speed_[2] * 100;
		oss << "Motor 4: " << motor_speed_[3] * 100 << "\n\n";

		// print imu
		array<float, 3> angular_pos = imu.get_angular_velocity();
		array<float, 3> linear_velocity = imu.get_lin_velocity();

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
		oss << "Throttle: " << setw(n) << target_throttle_;
		offset = [](string s) -> int {return s.length();}("Roll: ");
		n = COLS / 4 - offset;
		oss << "Roll: " << setw(n) << target_roll_;
		offset = [](string s) -> int {return s.length();}("Pitch: ");
		n = COLS / 4 - offset;
		oss << "Pitch: " << setw(n) << target_pitch_;
		offset = [](string s) -> int {return s.length();}("Yaw: ");
		n = COLS / 4 - offset;
		oss << "Yaw: " << setw(n) << target_yaw_;

		ui_->UpdateStat(oss.str());
		this_thread::sleep_for(chrono::milliseconds(100));
	}
}

inline float max(float a, float b) {
	return a >= b ? a : b;
}

inline float min(float a, float b) {
	return a <= b ? a : b;
}

void ResetMotors() {
	hover_ = 0;
	motor_speed_[0] = 0;
	motor_speed_[1] = 0;
	motor_speed_[2] = 0;
	motor_speed_[3] = 0;
	gpioPWM(motor_gpio_[0], motor_speed_[0] * 1000 + 1000);
	gpioPWM(motor_gpio_[1], motor_speed_[1] * 1000 + 1000);
	gpioPWM(motor_gpio_[2], motor_speed_[2] * 1000 + 1000);
	gpioPWM(motor_gpio_[3], motor_speed_[3] * 1000 + 1000);
}

void OnStart() {
	state_ = RUNNING;
	ui_->Print("Received START signal\n");
}

void OnStop() {
	state_ = STOP;
	ui_->Print("STOP signal received\n");
	ResetMotors();
	receiver->Reset();
	ui_->Print("Waiting for START signal\n");
}

void OnLostConn() {
	if (state_ == RUNNING) {
		ui_->Print("E-STOP: Lost connection\n");
		ResetMotors();
		receiver->Reset();
		state_ = STOP;
		ui_->Print("Waiting for START signal\n");
	}
}

void ParseArgs(int argc, char* argv[]) {
	int c;
	while ((c = getopt(argc, argv, "jb")) != -1) {
		switch (c) {
		case 'j':
			js_mode_ = true;
			break;
		case 'b':
			dummy_ui_ = true;
			break;
		}
	}
}

int main(int argc, char* argv[]) {
	ParseArgs(argc, argv);
	if (dummy_ui_) {
		ui_ = DummyUI::getInstance();
	} else {
		ui_ = NCurseUI::getInstance();
	}
	if (js_mode_) {
		receiver = new Receiver(unique_ptr<Connection>(new JSConnection()),
				ui_);
	} else {
		receiver = new Receiver(
				unique_ptr<Connection>(new WifiConnection(43123)), ui_);
	}

	if (gpioInitialise() < 0) {
		cout << "Error initializing gpio!" << endl;
		return (-1);
	}
	atexit(onexit);
	gpioSetPWMfrequency(motor_gpio_[0], kEscRate);
	gpioSetPWMrange(motor_gpio_[0], 1000000 / kEscRate);
	gpioSetPWMfrequency(motor_gpio_[1], kEscRate);
	gpioSetPWMrange(motor_gpio_[1], 1000000 / kEscRate);
	gpioSetPWMfrequency(motor_gpio_[2], kEscRate);
	gpioSetPWMrange(motor_gpio_[2], 1000000 / kEscRate);
	gpioSetPWMfrequency(motor_gpio_[3], kEscRate);
	gpioSetPWMrange(motor_gpio_[3], 1000000 / kEscRate);

	ui_->Init();
	thread ui_thread_(UILoop);

	int result = imu.Init();
	if (!result) {
		cout << "Error starting IMU, error code=" << result << "\n";
		return -1;
	}
	imu.set_roll_inv(true);
	imu.set_pitch_inv(true);
	ui_->Print("Starting zero field calibration\n");
	result = imu.CalibrateZeroFieldOffset();
	if (result < 0)
		ui_->Print(
				"IMU: Error when calibrating zero field offset, error code="
						+ to_string(result) + "\n");
	else
		ui_->Print("Finished zero field calibration\n");

	gpioPWM(motor_gpio_[0], 1000);
	gpioPWM(motor_gpio_[1], 1000);
	gpioPWM(motor_gpio_[2], 1000);
	gpioPWM(motor_gpio_[3], 1000);

	receiver->AddStartCmdListener(&OnStart);
	receiver->AddStopCmdListener(&OnStop);
	receiver->AddLostConnListener(&OnLostConn);
	receiver->Start();

	auto cur_time = chrono::high_resolution_clock::now();
	auto prev_time = cur_time;

	ui_->Print("Waiting for START signal\n");

	while (true) {
		cur_time = chrono::high_resolution_clock::now();
		float delta = chrono::duration_cast<chrono::milliseconds>(
				cur_time - prev_time).count() / 1000.0;
		prev_time = cur_time;

		imu.Update();
		if (state_ == RUNNING) {
			array<float, 3> lin_velocity = imu.get_lin_velocity();
			array<float, 3> lin_accel = imu.get_lin_accel();
			array<float, 3> ang_pos = imu.get_angular_pos();
			array<float, 3> ang_velocity = imu.get_angular_velocity();

			if (abs(lin_velocity[2]) < kThrottleThreshold) {
				lin_velocity[2] = 0;
				lin_accel[2] = 0;
			}

			if (abs(ang_pos[0]) > 10 || abs(ang_pos[1] > 10)
					|| motor_speed_[0] > 0.25 || motor_speed_[1] > 0.25
					|| motor_speed_[2] > 0.25 || motor_speed_[3] > 0.25) {
				ui_->Print("E-STOP: Threshold exceeded\n");
				ui_->Print(
						"Roll: " + to_string(ang_pos[0]) + ", "
								+ to_string(ang_velocity[0]) + "\n");
				ui_->Print(
						"Pitch: " + to_string(ang_pos[1]) + ", "
								+ to_string(ang_velocity[1]) + "\n");
				ui_->Print(
						"Yaw: " + to_string(ang_pos[2]) + ", "
								+ to_string(ang_velocity[2]) + "\n");
				ui_->Print(
						"Throttle: " + to_string(lin_velocity[2]) + ", "
								+ to_string(lin_accel[2]) + "\n");
				ui_->Print(
						"Motors: " + to_string(motor_speed_[0]) + ", "
								+ to_string(motor_speed_[1]) + ", "
								+ to_string(motor_speed_[2]) + ", "
								+ to_string(motor_speed_[3]) + "\n");
				OnStop();
				continue;
			}

			target_throttle_ = receiver->get_throttle() / 100.0 * kMaxThrottle;
			target_roll_ = receiver->get_roll() / 100.0 * kMaxRollPitch;
			target_pitch_ = receiver->get_pitch() / 100.0 * kMaxRollPitch;
			target_yaw_ = receiver->get_yaw() / 100.0 * kMaxYaw;
			float err_throttle = target_throttle_ - lin_velocity[2];
			float errd_throttle = -lin_accel[2];
			float err_roll = target_roll_ - ang_pos[0];
			float errd_roll = -ang_velocity[0];
			float err_pitch = target_pitch_ - ang_pos[1];
			float errd_pitch = -ang_velocity[1];
			float err_yaw = target_yaw_ - ang_velocity[2];

//			// control velocity instead of ang pos when targets == 0
//			if (target_roll_ == 0 && target_pitch_ == 0) {
//				err_roll = lin_velocity[1] * 4 - ang_pos[0];
//				err_pitch = -lin_velocity[0] * 4 - ang_pos[1];
//			}

			float uz = (kPThrotle * err_throttle + kDThrottle * errd_throttle)
					* delta;
			float uroll = kPRollPitch * err_roll + kDRollPitch * errd_roll;
			float upitch = kPRollPitch * err_pitch + kDRollPitch * errd_pitch;
			float uyaw = kPYaw * err_yaw;
//			hover_ += uz;
//			hover_ = max(min(hover_, kMaxMotor), 0);
			hover_ = receiver->get_throttle() / 100.0;
			uroll = max(min(uroll, 1 - kMaxMotor), -1 + kMaxMotor);
			upitch = max(min(upitch, 1 - kMaxMotor), -1 + kMaxMotor);
			uyaw = max(min(uyaw, 1 - kMaxMotor), -1 + kMaxMotor);
			motor_speed_[0] = max(min(hover_ + uroll - upitch + uyaw, 1), 0);
			motor_speed_[1] = max(min(hover_ - uroll - upitch - uyaw, 1), 0);
			motor_speed_[2] = max(min(hover_ - uroll + upitch + uyaw, 1), 0);
			motor_speed_[3] = max(min(hover_ + uroll + upitch - uyaw, 1), 0);
			gpioPWM(motor_gpio_[0], motor_speed_[0] * 1000 + 1000);
			gpioPWM(motor_gpio_[1], motor_speed_[1] * 1000 + 1000);
			gpioPWM(motor_gpio_[2], motor_speed_[2] * 1000 + 1000);
			gpioPWM(motor_gpio_[3], motor_speed_[3] * 1000 + 1000);
		}
//		this_thread::yield();
		this_thread::sleep_for(chrono::milliseconds(1));
	}
}
