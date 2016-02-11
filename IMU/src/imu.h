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
	int Init();
	int Update();
	void Close();
	int CalibrateZeroFieldOffset();
	int HasNewData(bool& out);
	std::array<float, 3> get_angular_pos();
	std::array<float, 3> get_angular_velocity();
	std::array<float, 3> get_lin_velocity();
	std::array<float, 3> get_lin_accel();
	bool is_roll_inv();
	bool is_pitch_inv();
	bool is_yaw_inv();
	bool is_z_inv();
	void set_roll_inv(bool inv);
	void set_pitch_inv(bool inv);
	void set_yaw_inv(bool inv);
	void set_z_inv(bool inv);
private:
	const char kGyroAddr = 0x6b;
	const char kAccelAddr = 0x1d;
	const short kGyroRange = 245;
	const short kAccelRange = 2;
	const short kMagRange = 4;
	const float kGravity = 9.81;
	const float kAccelThreshold = 0.02;

	inline int ProcessData();
	int LoadConf();

	int h_gyro_ = 0;
	int h_accelmag_ = 0;
	std::array<signed char, 4> inverted_ = { { 1, 1, 1, 1 } };
	std::array<float, 3> zero_ang_offset_ = { { 0 } };
	std::array<float, 3> zero_angrate_offset_ = { { 0 } };
	std::array<float, 3> zero_g_offset_ = { { 0 } };
	std::array<float, 3> hard_iron_offset_ = { { 0 } };
	std::array<float, 3> accel_g_ = { { 0 } };
	std::array<float, 3> accel_g_smoothed_ = { { 0 } };
	std::array<float, 2> accel_orientation_ = { { 0 } };
	std::array<float, 3> gravity_ = { { 0 } };
	std::array<float, 3> ang_pos_ = { { 0 } };
	std::array<float, 3> ang_velocity_ = { { 0 } };
	std::array<float, 3> ang_velocity_smoothed_ = { { 0 } };
	std::array<float, 3> lin_velocity_movingsum_ = { { 0 } };
	std::array<float, 3> ang_velo_movingsum_ = { { 0 } };
	std::array<float, 3> accel_g_movingsum_ = { { 0 } };
	std::array<float, 3> lin_velocity_ = { { 0 } };
	std::array<float, 3> lin_accel_ = { { 0 } };
	std::queue<std::array<float, 4>> lin_accel_samples_;
	std::queue<std::array<float, 4>> ang_velo_samples_;
	std::queue<std::array<float, 4>> accel_g_samples_;
	float lin_velo_time_ = 0;
	float ang_velo_time_ = 0;
	float accel_g_time_ = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock,
			std::chrono::high_resolution_clock::duration> last_update_;

	inline float rad2deg(float rad) {
		return rad * 180 / M_PI;
	}

	inline float deg2rad(float deg) {
		return deg / 180 * M_PI;
	}
};

#endif /* IMU_H_ */
