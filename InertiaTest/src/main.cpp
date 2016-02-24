/*
 * main.cpp
 *
 *  Created on: Jan 22, 2016
 *      Author: teoko
 */

#include <IMU.h>
#include <pigpio.h>
#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

const int ESC_RATE = 400;

IMU g_imu;
array<short, 4> g_motorGpio = { 23, 18, 15, 14 };

void onexit() {
	gpioTerminate();
}

int main() {
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
	gpioPWM(g_motorGpio[0], 1000);
	gpioPWM(g_motorGpio[1], 1000);
	gpioPWM(g_motorGpio[2], 1000);
	gpioPWM(g_motorGpio[3], 1000);

	g_imu.init();
	int result = g_imu.init();
	if (!result) {
		cout << "Error starting IMU, error code=" << result << "\n";
		return -1;
	}
	g_imu.setRollInv(true);
	g_imu.setPitchInv(true);

	cout
			<< "This program will attempt to estimate the moment inertia by measuring the time taken for the quadcopter to rotate 10 degrees. Make sure that the quadcopter is secured to something as it will not auto stabilize during this test.\n";
	cout << "Press <ENTER> to start the test\n";
	cin.ignore();
	cout << "Calibrating zero field offset...";
	g_imu.calibrateZeroFieldOffset();
	cout << "done!\n";
	cout << "Starting inertia test\n";

	auto start_time = chrono::high_resolution_clock::now();
	gpioPWM(g_motorGpio[0], 1200);
	gpioPWM(g_motorGpio[1], 1200);
	gpioPWM(g_motorGpio[2], 1200);
	gpioPWM(g_motorGpio[3], 1200);
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
	gpioPWM(g_motorGpio[0], 1300);
	gpioPWM(g_motorGpio[1], 1100);
	gpioPWM(g_motorGpio[2], 1100);
	gpioPWM(g_motorGpio[3], 1300);
	cur_time = chrono::high_resolution_clock::now();
	time_elasped = chrono::duration_cast<chrono::milliseconds>(
			cur_time - start_time).count();
	g_imu.update();
	array<float, 3> ang_pos = g_imu.getAngularPos();
	while (time_elasped < 3000 && ang_pos[0] < 10) {
		this_thread::sleep_for(chrono::milliseconds(1));
		cur_time = chrono::high_resolution_clock::now();
		time_elasped = chrono::duration_cast<chrono::milliseconds>(
				cur_time - start_time).count();
		g_imu.update();
		ang_pos = g_imu.getAngularPos();
	}

	cout << "Time elasped: " << to_string(time_elasped) << "ms\n";
	cout << "Roll: " << to_string(ang_pos[0]) << "\n";

	gpioPWM(g_motorGpio[0], 1000);
	gpioPWM(g_motorGpio[1], 1000);
	gpioPWM(g_motorGpio[2], 1000);
	gpioPWM(g_motorGpio[3], 1000);
	this_thread::sleep_for(chrono::milliseconds(1000));
	gpioTerminate();
}
