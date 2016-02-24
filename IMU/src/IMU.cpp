/*
 * IMU.cpp
 *
 *  Created on: 8 Oct 2015
 *      Author: kp
 */

#include <pigpio.h>
#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include "IMU.h"

using namespace std;

IMU::IMU() {
}

array<float, 3> IMU::getAngularPos() {
	return angPos;
}

array<float, 3> IMU::getAngularVelocity() {
	return angVelocity;
}

array<float, 3> IMU::getLinVelocity() {
	return linVelocity;
}

array<float, 3> IMU::getLinAccel() {
	return linAccel;
}

bool IMU::isRollInv() {
	return !inverted[0];
}

bool IMU::isPitchInv() {
	return !inverted[1];
}

bool IMU::isYawInv() {
	return !inverted[2];
}

bool IMU::isZInv() {
	return !inverted[3];
}

void IMU::setRollInv(bool inv) {
	if (inv)
		inverted[0] = -1;
	else
		inverted[0] = 1;
}

void IMU::setPitchInv(bool inv) {
	if (inv)
		inverted[1] = -1;
	else
		inverted[1] = 1;
}

void IMU::setYawInv(bool inv) {
	if (inv)
		inverted[2] = -1;
	else
		inverted[2] = 1;
}

void IMU::setZInv(bool inv) {
	if (inv)
		inverted[3] = -1;
	else
		inverted[3] = 1;
}

int IMU::init() {
	hGyro = i2cOpen(1, GYRO_ADDR, 0);
	if (hGyro < 0)
		return hGyro;
	hAccelMag = i2cOpen(1, ACCELMAG_ADDR, 0);
	if (hAccelMag < 0)
		return hAccelMag;

	if (loadConf() != 0) {
		return -1;
	}

	int result;
	result = i2cWriteByteData(hGyro, 0x20, 0b10001111); // Set data rate to 400hz and enable all axis
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x20, 0b10001111); // Set data rate to 400hz and enable all axis
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x24, 0b01110100); // Set resolution to high and data rate to 100hz
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x26, 0b00000000); // Set continuous mode
	if (result < 0)
		return result;

	// Reset magnetometer offset
	result = i2cWriteByteData(hAccelMag, 0x16, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x17, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x18, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x19, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x1A, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(hAccelMag, 0x1B, 0b00000000);
	if (result < 0)
		return result;

	lastUpdate = chrono::high_resolution_clock::now();
	return 1;
}

int IMU::update() {
	int result = processData();
	if (result < 0)
		return result;
	return 1;
}

void IMU::close() {
	i2cClose(hGyro);
	i2cClose(hAccelMag);
}

int IMU::hasNewData(bool& out) {
	out = false;
	char status_gyro = i2cReadByteData(hGyro, 0x27);
	if (status_gyro < 0)
		return status_gyro;
	char status_accel = i2cReadByteData(hAccelMag, 0x27);
	if (status_accel < 0)
		return status_accel;
	char status_mag = i2cReadByteData(hAccelMag, 0x07);
	if (status_mag < 0)
		return status_mag;

	out = (status_gyro & 0b00001111) || (status_accel & 0b00001111)
			|| (status_mag & 0b00001111);
	return 1;
}

int IMU::calibrateZeroFieldOffset() {
	vector<float> angRateXSamples(100);
	vector<float> angRateYSamples(100);
	vector<float> angRateZSamples(100);
	for (int i = 0; i < 100; i++) {
		int result = processData();
		if (result < 0)
			return result;
		angRateXSamples[i] = -angVelocity[0];
		angRateYSamples[i] = -angVelocity[1];
		angRateZSamples[i] = -angVelocity[2];
		zeroGOffset[0] += -accelG[0];
		zeroGOffset[1] += -accelG[1];
		zeroGOffset[2] += -accelG[2] + 1;

		result = processData();
		if (result < 0)
			return result;
		zeroAngOffset[0] += -deg2rad(accelOrientation[0]);
		zeroAngOffset[1] += -deg2rad(accelOrientation[1]);
		result = processData();
		if (result < 0)
			return result;
		zeroAngOffset[2] += -deg2rad(angPos[2]);
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	sort(angRateXSamples.begin(), angRateXSamples.end());
	sort(angRateYSamples.begin(), angRateYSamples.end());
	sort(angRateZSamples.begin(), angRateZSamples.end());
	zeroAngRateOffset[0] = angRateXSamples[50];
	zeroAngRateOffset[1] = angRateYSamples[50];
	zeroAngRateOffset[2] = angRateZSamples[50];
	return 1;
}

inline int IMU::processData() {
	auto curTime = chrono::high_resolution_clock::now();
	float delta = chrono::duration_cast<chrono::milliseconds>(
			curTime - lastUpdate).count() / 1000.0;
	lastUpdate = curTime;

	char buf[6];
	short* rawX;
	short* rawY;
	short* rawZ;
	int result;

	result = i2cReadI2CBlockData(hAccelMag, 0xA8, buf, 6);
	if (result < 0)
		return result;
	rawX = (short*) buf;
	rawY = (short*) (buf + 2);
	rawZ = (short*) (buf + 4);

	accelG[0] = ((float) *rawX / numeric_limits<short>::max()) * ACCEL_RANGE
			* inverted[0] + zeroGOffset[0];
	accelG[1] = ((float) *rawY / numeric_limits<short>::max()) * ACCEL_RANGE
			* inverted[1] + zeroGOffset[1];
	accelG[2] = ((float) *rawZ / numeric_limits<short>::max()) * ACCEL_RANGE
			* inverted[3] + zeroGOffset[2];
	array<float, 4> sample;
	sample[0] = accelG[0];
	sample[1] = accelG[1];
	sample[2] = accelG[2];
	sample[3] = delta;
	accelGSamples.push(sample);
	accelGMovingsum[0] += sample[0];
	accelGMovingsum[1] += sample[1];
	accelGMovingsum[2] += sample[2];
	accelGTime += sample[3];
	while (accelGTime > 0.05 && accelGSamples.size() > 0) {
		sample = accelGSamples.front();
		accelGSamples.pop();
		accelGMovingsum[0] -= sample[0];
		accelGMovingsum[1] -= sample[1];
		accelGMovingsum[2] -= sample[2];
		accelGTime -= sample[3];
	}
	accelGSmoothed[0] = accelGMovingsum[0] / accelGSamples.size();
	accelGSmoothed[1] = accelGMovingsum[1] / accelGSamples.size();
	accelGSmoothed[2] = accelGMovingsum[2] / accelGSamples.size();

	accelOrientation[0] = rad2deg(
			atan2(accelGSmoothed[1], accelGSmoothed[2]))
			+ zeroAngOffset[0];
	accelOrientation[1] = rad2deg(
			atan2(-accelGSmoothed[0],
					sqrt(
							accelGSmoothed[1] * accelGSmoothed[1]
									+ accelGSmoothed[2]
											* accelGSmoothed[2])))
			+ zeroAngOffset[1];

	result = i2cReadI2CBlockData(hGyro, 0xA8, buf, 6);
	if (result < 0)
		return result;
	rawX = (short*) buf;
	rawY = (short*) (buf + 2);
	rawZ = (short*) (buf + 4);

	angVelocity[0] = (((float) *rawX / numeric_limits<short>::max())
			* GYRO_RANGE) * inverted[0] + zeroAngRateOffset[0];
	angVelocity[1] = (((float) *rawY / numeric_limits<short>::max())
			* GYRO_RANGE) * inverted[1] + zeroAngRateOffset[1];
	angVelocity[2] = (((float) *rawZ / numeric_limits<short>::max())
			* GYRO_RANGE) * inverted[2] + zeroAngRateOffset[2];
	sample[0] = angVelocity[0];
	sample[1] = angVelocity[1];
	sample[2] = angVelocity[2];
	sample[3] = delta;
	angVeloSamples.push(sample);
	angVeloMovingsum[0] += sample[0];
	angVeloMovingsum[1] += sample[1];
	angVeloMovingsum[2] += sample[2];
	angVeloTime += sample[3];
	while (angVeloTime > 0.05 && angVeloSamples.size() > 0) {
		sample = angVeloSamples.front();
		angVeloSamples.pop();
		angVeloMovingsum[0] -= sample[0];
		angVeloMovingsum[1] -= sample[1];
		angVeloMovingsum[2] -= sample[2];
		angVeloTime -= sample[3];
	}
	angVelocitySmoothed[0] = angVeloMovingsum[0]
			/ angVeloSamples.size();
	angVelocitySmoothed[1] = angVeloMovingsum[1]
			/ angVeloSamples.size();
	angVelocitySmoothed[2] = angVeloMovingsum[2]
			/ angVeloSamples.size();

	float factor = delta;
	angPos[0] = (1 - factor)
			* (angPos[0] + angVelocitySmoothed[0] * delta)
			+ factor * accelOrientation[0];
	angPos[1] = (1 - factor)
			* (angPos[1] + angVelocitySmoothed[1] * delta)
			+ factor * accelOrientation[1];

//	gravity_[0] = sin(deg2rad(angular_pos_[1])) * kGravity;
//	gravity_[1] = -sin(deg2rad(angular_pos_[0])) * kGravity;
//	float tmp = kGravity * kGravity
//			- (gravity_[0] * gravity_[0] + gravity_[1] * gravity_[1]);
//	if (tmp < 0)
//		tmp = 0;
//	gravity_[2] = sqrt(tmp);

	gravity[0] = sin(deg2rad(angPos[1])) * GRAVITY;
	gravity[1] = -cos(deg2rad(angPos[1])) * sin(deg2rad(angPos[0]))
			* GRAVITY;
	gravity[2] = cos(deg2rad(angPos[1])) * cos(deg2rad(angPos[0]))
			* GRAVITY;

	result = i2cReadI2CBlockData(hAccelMag, 0x88, buf, 6);
	if (result < 0)
		return result;
	rawX = (short*) buf;
	rawY = (short*) (buf + 2);
	rawZ = (short*) (buf + 4);

	float bx = (((float) *rawX - hardIronOffset[0])
			/ numeric_limits<short>::max()) * MAG_RANGE * inverted[0];
	float by = (((float) *rawY - hardIronOffset[1])
			/ numeric_limits<short>::max()) * MAG_RANGE * inverted[1];
	float bz = (((float) *rawZ - hardIronOffset[2])
			/ numeric_limits<short>::max()) * MAG_RANGE * inverted[2];

	float phi = deg2rad(angPos[0]);
	float theta = deg2rad(angPos[1]);
	angPos[2] = rad2deg(
			atan2(bz * sin(phi) - by * cos(phi),
					bx * cos(theta) + by * sin(phi) * sin(theta)
							+ bz * sin(theta) * cos(phi)) * inverted[2]
					+ zeroAngOffset[2]);

	linAccel[0] = -accelGSmoothed[0] * GRAVITY - gravity[0];
	linAccel[1] = -accelGSmoothed[1] * GRAVITY - gravity[1];
	linAccel[2] = accelGSmoothed[2] * GRAVITY - gravity[2];

	if (abs(linAccel[0]) < ACCEL_THRESHOLD)
		sample[0] = 0;
	else
		sample[0] = linAccel[0] * delta;
	if (abs(linAccel[1]) < ACCEL_THRESHOLD)
		sample[1] = 0;
	else
		sample[1] = linAccel[1] * delta;
	if (abs(linAccel[2]) < ACCEL_THRESHOLD)
		sample[2] = 0;
	else
		sample[2] = linAccel[2] * delta;
	sample[3] = delta;
	linAccelSamples.push(sample);
	linVelocityMovingsum[0] += sample[0];
	linVelocityMovingsum[1] += sample[1];
	linVelocityMovingsum[2] += sample[2];
	linVeloTime += delta;
	while (linVeloTime > 1 && linAccelSamples.size() > 0) {
		sample = linAccelSamples.front();
		linAccelSamples.pop();
		linVelocityMovingsum[0] -= sample[0];
		linVelocityMovingsum[1] -= sample[1];
		linVelocityMovingsum[2] -= sample[2];
		linVeloTime -= sample[3];
	}

	// Update new velocities
	if (linVeloTime != 0) {
		linVelocity[0] = linVelocityMovingsum[0] / linVeloTime;
		linVelocity[1] = linVelocityMovingsum[1] / linVeloTime;
		linVelocity[2] = linVelocityMovingsum[2] / linVeloTime;
	} else {
		linVelocity[0] = 0;
		linVelocity[1] = 0;
		linVelocity[2] = 0;
		linAccel[0] = 0;
		linAccel[1] = 0;
		linAccel[2] = 0;
	}
	return 1;
}

int IMU::loadConf() {
	ifstream confFile("mag.conf");
	if (!confFile.is_open())
		return -1;
	string line;
	try {
		for (int i = 0; i < 3; i++) {
			getline(confFile, line);
			hardIronOffset[i] = stoi(line);
		}
	} catch (invalid_argument& e) {
		confFile.close();
		return -1;
	}
	confFile.close();
	return 0;
}
