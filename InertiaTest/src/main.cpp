/*
 * main.cpp
 *
 *  Created on: Jan 22, 2016
 *      Author: teoko
 */

#include <imu.h>
#include <pigpio.h>
#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

const int kEscRate = 400;

IMU imu;
array<short, 4> motor_gpio_ = { 23, 18, 15, 14 };

void onexit() {
	gpioTerminate();
}

int main() {
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
	gpioPWM(motor_gpio_[0], 1000);
	gpioPWM(motor_gpio_[1], 1000);
	gpioPWM(motor_gpio_[2], 1000);
	gpioPWM(motor_gpio_[3], 1000);

	imu.Init();
	int result = imu.Init();
	if (!result) {
		cout << "Error starting IMU, error code=" << result << "\n";
		return -1;
	}
	imu.set_roll_inv(true);
	imu.set_pitch_inv(true);

	cout
			<< "This program will attempt to estimate the moment inertia by measuring the time taken for the quadcopter to rotate 10 degrees. Make sure that the quadcopter is secured to something as it will not auto stabilize during this test.\n";
	cout << "Press <ENTER> to start the test\n";
	cin.ignore();
	cout << "Calibrating zero field offset...";
	imu.CalibrateZeroFieldOffset();
	cout << "done!\n";
	cout << "Starting inertia test\n";

	auto start_time = chrono::high_resolution_clock::now();
	gpioPWM(motor_gpio_[0], 1200);
	gpioPWM(motor_gpio_[1], 1200);
	gpioPWM(motor_gpio_[2], 1200);
	gpioPWM(motor_gpio_[3], 1200);
	auto cur_time = chrono::high_resolution_clock::now();
	auto time_elasped = chrono::duration_cast<chrono::milliseconds>(
			cur_time - start_time).count();
	while (time_elasped < 2000) {
		this_thread::sleep_for(chrono::milliseconds(1));
		cur_time = chrono::high_resolution_clock::now();
		time_elasped = chrono::duration_cast<chrono::milliseconds>(
				cur_time - start_time).count();
	}

	start_time = chrono::high_resolution_clock::now();
	gpioPWM(motor_gpio_[0], 1300);
	gpioPWM(motor_gpio_[1], 1100);
	gpioPWM(motor_gpio_[2], 1100);
	gpioPWM(motor_gpio_[3], 1300);
	cur_time = chrono::high_resolution_clock::now();
	time_elasped = chrono::duration_cast<chrono::milliseconds>(
			cur_time - start_time).count();
	imu.Update();
	array<float, 3> ang_pos = imu.get_angular_pos();
	while (time_elasped < 3000 && ang_pos[0] < 10) {
		this_thread::sleep_for(chrono::milliseconds(1));
		cur_time = chrono::high_resolution_clock::now();
		time_elasped = chrono::duration_cast<chrono::milliseconds>(
				cur_time - start_time).count();
		imu.Update();
		ang_pos = imu.get_angular_pos();
	}

	cout << "Time elasped: " << to_string(time_elasped) << "ms\n";
	cout << "Roll: " << to_string(ang_pos[0]) << "\n";

	gpioPWM(motor_gpio_[0], 1000);
	gpioPWM(motor_gpio_[1], 1000);
	gpioPWM(motor_gpio_[2], 1000);
	gpioPWM(motor_gpio_[3], 1000);
	this_thread::sleep_for(chrono::milliseconds(1000));
	gpioTerminate();
}
