/*
 * Logger.cpp
 *
 *  Created on: Feb 24, 2016
 *      Author: teoko
 */

#include "Logger.h"

#include <asm-generic/errno-base.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <ctime>

using namespace std;

const string Logger::LOG_DIR = "log/";

Logger::Logger() {
	// create dir if not exists
	struct stat statStruct;
	if (stat(LOG_DIR.c_str(), &statStruct) != 0) {
		if (errno == ENOENT) {
			mkdir(LOG_DIR.c_str(), 0777);
		}
	}
	if (!S_ISDIR(statStruct.st_mode)) {
		mkdir(LOG_DIR.c_str(), 0777);
	}
}

Logger::~Logger() {
	oss->close();
	delete oss;
}

int Logger::open(string suffix) {
	if (oss->is_open()) {
		return -1;
	}

	time_t curTime = time(nullptr);
	char fileName[suffix.length() + 1]; // time string + sep + suffix + ext + null terminator
	strftime(fileName, 19, "%Y-%m-%d-%H-%M-%S", localtime(&curTime));
	strcat(fileName, "-");
	strcat(fileName, suffix.c_str());
	strcat(fileName, ".log");
	string filePath = LOG_DIR + string(fileName);
	if (isFileExists(filePath)) {
		return -1;
	}

	oss = new ofstream(filePath);
	oss->open(filePath);
	return 0;
}

bool Logger::isOpen() {
	return oss->is_open();
}

void Logger::write(string msg) {
	*oss << msg << endl;
}

void Logger::close() {
	oss->close();
}

bool Logger::isFileExists(string filePath) {
	return ifstream(filePath);
}
