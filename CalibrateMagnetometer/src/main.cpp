/*
 * main.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: kp
 */

#include <pigpio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>

using namespace std;

const char kMagAddr = 0x1d;

int h_mag_;
thread worker_;
bool stopping_ = false;
short max_x = 0;
short max_y = 0;
short max_z = 0;
short min_x = 0;
short min_y = 0;
short min_z = 0;

void CalibrateMag() {
	while (!stopping_) {
		char status = i2cReadByteData(h_mag_, 0x07);
		if (status & 00001111) {
			char buf[6];
			i2cReadI2CBlockData(h_mag_, 0x88, buf, 6);
			short* x = (short*) buf;
			short* y = (short*) (buf + 2);
			short* z = (short*) (buf + 4);

			if (*x > max_x) {
				max_x = *x;
			}
			if (*y > max_y) {
				max_y = *y;
			}
			if (*z > max_z) {
				max_z = *z;
			}
			if (*x < min_x) {
				min_x = *x;
			}
			if (*y < min_y) {
				min_y = *y;
			}
			if (*z < min_z) {
				min_z = *z;
			}
		}
		this_thread::yield();
	}
}

int main() {
	if (gpioInitialise() < 0) {
		cout << "Error initializing gpio!" << endl;
		return (-1);
	}

	h_mag_ = i2cOpen(1, kMagAddr, 0);
	i2cWriteByteData(h_mag_, 0x24, 0b01110100); // Set resolution to high and data rate to 100hz
	i2cWriteByteData(h_mag_, 0x26, 0b00000000); // Set continuous mode
	// Reset magnetometer offset
	i2cWriteByteData(h_mag_, 0x16, 0b00000000);
	i2cWriteByteData(h_mag_, 0x17, 0b00000000);
	i2cWriteByteData(h_mag_, 0x18, 0b00000000);
	i2cWriteByteData(h_mag_, 0x19, 0b00000000);
	i2cWriteByteData(h_mag_, 0x1A, 0b00000000);
	i2cWriteByteData(h_mag_, 0x1B, 0b00000000);

	worker_ = thread(&CalibrateMag);
	cout << "Move the magnetometer around in the roll direction\n";
	cout << "Press <ENTER> when done\n";
	cin.ignore();
	stopping_ = true;
	worker_.join();
	short offset_x = (max_x + min_x) / 2;

	worker_ = thread(&CalibrateMag);
	cout << "Move the magnetometer around in the pitch direction\n";
	cout << "Press <ENTER> when done\n";
	cin.ignore();
	stopping_ = true;
	worker_.join();
	short offset_y = (max_y + min_y) / 2;

	worker_ = thread(&CalibrateMag);
	cout << "Move the magnetometer around in the yaw direction\n";
	cout << "Press <ENTER> when done\n";
	cin.ignore();
	stopping_ = true;
	worker_.join();
	short offset_z = (max_z + min_z) / 2;

	ofstream f("mag.conf");
	f << to_string(offset_x) << "\n";
	f << to_string(offset_y) << "\n";
	f << to_string(offset_z) << "\n";
	f.close();

	cout << "Magnetometer offset\nX: " << offset_x << "\nY: " << offset_y
			<< "\nZ: " << offset_z << endl;

	gpioTerminate();
}
