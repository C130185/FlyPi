/*
 * IMU.cpp
 *
 *  Created on: 8 Oct 2015
 *      Author: kp
 */

#include "imu.h"

#include <pigpio.h>
#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std;

IMU::IMU() {
}

array<float, 3> IMU::get_angular_pos() {
	return ang_pos_;
}

array<float, 3> IMU::get_angular_velocity() {
	return ang_velocity_smoothed_;
}

array<float, 3> IMU::get_lin_velocity() {
	return lin_velocity_;
}

array<float, 3> IMU::get_lin_accel() {
	return lin_accel_;
}

bool IMU::is_roll_inv() {
	return !inverted_[0];
}

bool IMU::is_pitch_inv() {
	return !inverted_[1];
}

bool IMU::is_yaw_inv() {
	return !inverted_[2];
}

bool IMU::is_z_inv() {
	return !inverted_[3];
}

void IMU::set_roll_inv(bool inv) {
	if (inv)
		inverted_[0] = -1;
	else
		inverted_[0] = 1;
}

void IMU::set_pitch_inv(bool inv) {
	if (inv)
		inverted_[1] = -1;
	else
		inverted_[1] = 1;
}

void IMU::set_yaw_inv(bool inv) {
	if (inv)
		inverted_[2] = -1;
	else
		inverted_[2] = 1;
}

void IMU::set_z_inv(bool inv) {
	if (inv)
		inverted_[3] = -1;
	else
		inverted_[3] = 1;
}

int IMU::Init() {
	h_gyro_ = i2cOpen(1, kGyroAddr, 0);
	if (h_gyro_ < 0)
		return h_gyro_;
	h_accelmag_ = i2cOpen(1, kAccelAddr, 0);
	if (h_accelmag_ < 0)
		return h_accelmag_;

	if (LoadConf() != 0) {
		return -1;
	}

	int result;
	result = i2cWriteByteData(h_gyro_, 0x20, 0b10001111); // Set data rate to 400hz and enable all axis
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x20, 0b10001111); // Set data rate to 400hz and enable all axis
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x24, 0b01110100); // Set resolution to high and data rate to 100hz
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x26, 0b00000000); // Set continuous mode
	if (result < 0)
		return result;

	// Reset magnetometer offset
	result = i2cWriteByteData(h_accelmag_, 0x16, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x17, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x18, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x19, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x1A, 0b00000000);
	if (result < 0)
		return result;
	result = i2cWriteByteData(h_accelmag_, 0x1B, 0b00000000);
	if (result < 0)
		return result;

	last_update_ = chrono::high_resolution_clock::now();
	return 1;
}

int IMU::Update() {
	int result = ProcessData();
	if (result < 0)
		return result;
	return 1;
}

void IMU::Close() {
	i2cClose(h_gyro_);
	i2cClose(h_accelmag_);
}

int IMU::HasNewData(bool& out) {
	out = false;
	char status_gyro = i2cReadByteData(h_gyro_, 0x27);
	if (status_gyro < 0)
		return status_gyro;
	char status_accel = i2cReadByteData(h_accelmag_, 0x27);
	if (status_accel < 0)
		return status_accel;
	char status_mag = i2cReadByteData(h_accelmag_, 0x07);
	if (status_mag < 0)
		return status_mag;

	out = (status_gyro & 0b00001111) || (status_accel & 0b00001111)
			|| (status_mag & 0b00001111);
	return 1;
}

int IMU::CalibrateZeroFieldOffset() {
	vector<float> angrate_x_samples(100);
	vector<float> angrate_y_samples(100);
	vector<float> angrate_z_samples(100);
	for (int i = 0; i < 100; i++) {
		int result = ProcessData();
		if (result < 0)
			return result;
		angrate_x_samples[i] = -ang_velocity_[0];
		angrate_y_samples[i] = -ang_velocity_[1];
		angrate_z_samples[i] = -ang_velocity_[2];
		zero_g_offset_[0] += -accel_g_[0];
		zero_g_offset_[1] += -accel_g_[1];
		zero_g_offset_[2] += -accel_g_[2] + 1;

		result = ProcessData();
		if (result < 0)
			return result;
		zero_ang_offset_[0] += -deg2rad(accel_orientation_[0]);
		zero_ang_offset_[1] += -deg2rad(accel_orientation_[1]);
		result = ProcessData();
		if (result < 0)
			return result;
		zero_ang_offset_[2] += -deg2rad(ang_pos_[2]);
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	sort(angrate_x_samples.begin(), angrate_x_samples.end());
	sort(angrate_y_samples.begin(), angrate_y_samples.end());
	sort(angrate_z_samples.begin(), angrate_z_samples.end());
	zero_angrate_offset_[0] = angrate_x_samples[50];
	zero_angrate_offset_[1] = angrate_y_samples[50];
	zero_angrate_offset_[2] = angrate_z_samples[50];
	return 1;
}

inline int IMU::ProcessData() {
	auto cur_time = chrono::high_resolution_clock::now();
	float delta = chrono::duration_cast<chrono::milliseconds>(
			cur_time - last_update_).count() / 1000.0;
	last_update_ = cur_time;

	char buf[6];
	short* raw_x;
	short* raw_y;
	short* raw_z;
	int result;

	result = i2cReadI2CBlockData(h_accelmag_, 0xA8, buf, 6);
	if (result < 0)
		return result;
	raw_x = (short*) buf;
	raw_y = (short*) (buf + 2);
	raw_z = (short*) (buf + 4);

	accel_g_[0] = ((float) *raw_x / numeric_limits<short>::max()) * kAccelRange
			* inverted_[0] + zero_g_offset_[0];
	accel_g_[1] = ((float) *raw_y / numeric_limits<short>::max()) * kAccelRange
			* inverted_[1] + zero_g_offset_[1];
	accel_g_[2] = ((float) *raw_z / numeric_limits<short>::max()) * kAccelRange
			* inverted_[3] + zero_g_offset_[2];
	array<float, 4> sample;
	sample[0] = accel_g_[0];
	sample[1] = accel_g_[1];
	sample[2] = accel_g_[2];
	sample[3] = delta;
	accel_g_samples_.push(sample);
	accel_g_movingsum_[0] += sample[0];
	accel_g_movingsum_[1] += sample[1];
	accel_g_movingsum_[2] += sample[2];
	accel_g_time_ += sample[3];
	while (accel_g_time_ > 0.05 && accel_g_samples_.size() > 0) {
		sample = accel_g_samples_.front();
		accel_g_samples_.pop();
		accel_g_movingsum_[0] -= sample[0];
		accel_g_movingsum_[1] -= sample[1];
		accel_g_movingsum_[2] -= sample[2];
		accel_g_time_ -= sample[3];
	}
	accel_g_smoothed_[0] = accel_g_movingsum_[0] / accel_g_samples_.size();
	accel_g_smoothed_[1] = accel_g_movingsum_[1] / accel_g_samples_.size();
	accel_g_smoothed_[2] = accel_g_movingsum_[2] / accel_g_samples_.size();

	accel_orientation_[0] = rad2deg(
			atan2(accel_g_smoothed_[1], accel_g_smoothed_[2]))
			+ zero_ang_offset_[0];
	accel_orientation_[1] = rad2deg(
			atan2(-accel_g_smoothed_[0],
					sqrt(
							accel_g_smoothed_[1] * accel_g_smoothed_[1]
									+ accel_g_smoothed_[2]
											* accel_g_smoothed_[2])))
			+ zero_ang_offset_[1];

	result = i2cReadI2CBlockData(h_gyro_, 0xA8, buf, 6);
	if (result < 0)
		return result;
	raw_x = (short*) buf;
	raw_y = (short*) (buf + 2);
	raw_z = (short*) (buf + 4);

	ang_velocity_[0] = (((float) *raw_x / numeric_limits<short>::max())
			* kGyroRange) * inverted_[0] + zero_angrate_offset_[0];
	ang_velocity_[1] = (((float) *raw_y / numeric_limits<short>::max())
			* kGyroRange) * inverted_[1] + zero_angrate_offset_[1];
	ang_velocity_[2] = (((float) *raw_z / numeric_limits<short>::max())
			* kGyroRange) * inverted_[2] + zero_angrate_offset_[2];
	sample[0] = ang_velocity_[0];
	sample[1] = ang_velocity_[1];
	sample[2] = ang_velocity_[2];
	sample[3] = delta;
	ang_velo_samples_.push(sample);
	ang_velo_movingsum_[0] += sample[0];
	ang_velo_movingsum_[1] += sample[1];
	ang_velo_movingsum_[2] += sample[2];
	ang_velo_time_ += sample[3];
	while (ang_velo_time_ > 0.05 && ang_velo_samples_.size() > 0) {
		sample = ang_velo_samples_.front();
		ang_velo_samples_.pop();
		ang_velo_movingsum_[0] -= sample[0];
		ang_velo_movingsum_[1] -= sample[1];
		ang_velo_movingsum_[2] -= sample[2];
		ang_velo_time_ -= sample[3];
	}
	ang_velocity_smoothed_[0] = ang_velo_movingsum_[0]
			/ ang_velo_samples_.size();
	ang_velocity_smoothed_[1] = ang_velo_movingsum_[1]
			/ ang_velo_samples_.size();
	ang_velocity_smoothed_[2] = ang_velo_movingsum_[2]
			/ ang_velo_samples_.size();

	float factor = delta;
	ang_pos_[0] = (1 - factor)
			* (ang_pos_[0] + ang_velocity_smoothed_[0] * delta)
			+ factor * accel_orientation_[0];
	ang_pos_[1] = (1 - factor)
			* (ang_pos_[1] + ang_velocity_smoothed_[1] * delta)
			+ factor * accel_orientation_[1];

//	gravity_[0] = sin(deg2rad(angular_pos_[1])) * kGravity;
//	gravity_[1] = -sin(deg2rad(angular_pos_[0])) * kGravity;
//	float tmp = kGravity * kGravity
//			- (gravity_[0] * gravity_[0] + gravity_[1] * gravity_[1]);
//	if (tmp < 0)
//		tmp = 0;
//	gravity_[2] = sqrt(tmp);

	gravity_[0] = sin(deg2rad(ang_pos_[1])) * kGravity;
	gravity_[1] = -cos(deg2rad(ang_pos_[1])) * sin(deg2rad(ang_pos_[0]))
			* kGravity;
	gravity_[2] = cos(deg2rad(ang_pos_[1])) * cos(deg2rad(ang_pos_[0]))
			* kGravity;

	result = i2cReadI2CBlockData(h_accelmag_, 0x88, buf, 6);
	if (result < 0)
		return result;
	raw_x = (short*) buf;
	raw_y = (short*) (buf + 2);
	raw_z = (short*) (buf + 4);

	float bx = (((float) *raw_x - hard_iron_offset_[0])
			/ numeric_limits<short>::max()) * kMagRange * inverted_[0];
	float by = (((float) *raw_y - hard_iron_offset_[1])
			/ numeric_limits<short>::max()) * kMagRange * inverted_[1];
	float bz = (((float) *raw_z - hard_iron_offset_[2])
			/ numeric_limits<short>::max()) * kMagRange * inverted_[2];

	float phi = deg2rad(ang_pos_[0]);
	float theta = deg2rad(ang_pos_[1]);
	ang_pos_[2] = rad2deg(
			atan2(bz * sin(phi) - by * cos(phi),
					bx * cos(theta) + by * sin(phi) * sin(theta)
							+ bz * sin(theta) * cos(phi)) * inverted_[2]
					+ zero_ang_offset_[2]);

	lin_accel_[0] = -accel_g_smoothed_[0] * kGravity - gravity_[0];
	lin_accel_[1] = -accel_g_smoothed_[1] * kGravity - gravity_[1];
	lin_accel_[2] = accel_g_smoothed_[2] * kGravity - gravity_[2];

	if (abs(lin_accel_[0]) < kAccelThreshold)
		sample[0] = 0;
	else
		sample[0] = lin_accel_[0] * delta;
	if (abs(lin_accel_[1]) < kAccelThreshold)
		sample[1] = 0;
	else
		sample[1] = lin_accel_[1] * delta;
	if (abs(lin_accel_[2]) < kAccelThreshold)
		sample[2] = 0;
	else
		sample[2] = lin_accel_[2] * delta;
	sample[3] = delta;
	lin_accel_samples_.push(sample);
	lin_velocity_movingsum_[0] += sample[0];
	lin_velocity_movingsum_[1] += sample[1];
	lin_velocity_movingsum_[2] += sample[2];
	lin_velo_time_ += delta;
	while (lin_velo_time_ > 1 && lin_accel_samples_.size() > 0) {
		sample = lin_accel_samples_.front();
		lin_accel_samples_.pop();
		lin_velocity_movingsum_[0] -= sample[0];
		lin_velocity_movingsum_[1] -= sample[1];
		lin_velocity_movingsum_[2] -= sample[2];
		lin_velo_time_ -= sample[3];
	}

	// Update new velocities
	if (lin_velo_time_ != 0) {
		lin_velocity_[0] = lin_velocity_movingsum_[0] / lin_velo_time_;
		lin_velocity_[1] = lin_velocity_movingsum_[1] / lin_velo_time_;
		lin_velocity_[2] = lin_velocity_movingsum_[2] / lin_velo_time_;
	} else {
		lin_velocity_[0] = 0;
		lin_velocity_[1] = 0;
		lin_velocity_[2] = 0;
		lin_accel_[0] = 0;
		lin_accel_[1] = 0;
		lin_accel_[2] = 0;
	}
	return 1;
}

int IMU::LoadConf() {
	ifstream conf_file("mag.conf");
	if (!conf_file.is_open())
		return -1;
	string line;
	try {
		for (int i = 0; i < 3; i++) {
			getline(conf_file, line);
			hard_iron_offset_[i] = stoi(line);
		}
	} catch (invalid_argument& e) {
		conf_file.close();
		return -1;
	}
	conf_file.close();
	return 0;
}
