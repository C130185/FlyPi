/*
 * IMU.h
 *
 *  Created on: 8 Oct 2015
 *      Author: kp
 */

#ifndef IMU_H_
#define IMU_H_

#include <array>
#include <chrono>
#include <cmath>
#include <queue>

class IMU {
public:
	IMU();
	int init();
	int update();
	void close();
	int calibrateZeroFieldOffset();
	int hasNewData(bool& out);
	std::array<float, 3> getAngularPos();
	std::array<float, 3> getAngularVelocity();
	std::array<float, 3> getLinVelocity();
	std::array<float, 3> getLinAccel();
	bool isRollInv();
	bool isPitchInv();
	bool isYawInv();
	bool isZInv();
	void setRollInv(bool inv);
	void setPitchInv(bool inv);
	void setYawInv(bool inv);
	void setZInv(bool inv);
private:
	const char GYRO_ADDR = 0x6b;
	const char ACCELMAG_ADDR = 0x1d;
	const short GYRO_RANGE = 245;
	const short ACCEL_RANGE = 2;
	const short MAG_RANGE = 4;
	const float GRAVITY = 9.81;
	const float ACCEL_THRESHOLD = 0.02;

	inline int processData();
	int loadConf();

	int hGyro = 0;
	int hAccelMag = 0;
	std::array<signed char, 4> inverted = { { 1, 1, 1, 1 } };
	std::array<float, 3> zeroAngOffset = { { 0 } };
	std::array<float, 3> zeroAngRateOffset = { { 0 } };
	std::array<float, 3> zeroGOffset = { { 0 } };
	std::array<float, 3> hardIronOffset = { { 0 } };
	std::array<float, 3> accelG = { { 0 } };
	std::array<float, 3> accelGSmoothed = { { 0 } };
	std::array<float, 2> accelOrientation = { { 0 } };
	std::array<float, 3> gravity = { { 0 } };
	std::array<float, 3> angPos = { { 0 } };
	std::array<float, 3> angVelocity = { { 0 } };
	std::array<float, 3> angVelocitySmoothed = { { 0 } };
	std::array<float, 3> linVelocityMovingsum = { { 0 } };
	std::array<float, 3> angVeloMovingsum = { { 0 } };
	std::array<float, 3> accelGMovingsum = { { 0 } };
	std::array<float, 3> linVelocity = { { 0 } };
	std::array<float, 3> linAccel = { { 0 } };
	std::queue<std::array<float, 4>> linAccelSamples;
	std::queue<std::array<float, 4>> angVeloSamples;
	std::queue<std::array<float, 4>> accelGSamples;
	float linVeloTime = 0;
	float angVeloTime = 0;
	float accelGTime = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock,
			std::chrono::high_resolution_clock::duration> lastUpdate;

	inline float rad2deg(float rad) {
		return rad * 180 / M_PI;
	}

	inline float deg2rad(float deg) {
		return deg / 180 * M_PI;
	}
};

#endif /* IMU_H_ */
