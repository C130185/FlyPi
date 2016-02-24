/*
 * Logger.h
 *
 *  Created on: Feb 24, 2016
 *      Author: teoko
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <fstream>
#include <string>

#include "ILogger.h"

class Logger : public ILogger {
public:
	Logger();
	virtual ~Logger();
	int open(std::string suffix);
	bool isOpen();
	void write(std::string msg);
	void close();
private:
	static const std::string LOG_DIR;
	std::ofstream* oss = nullptr;

	bool isFileExists(std::string filePath);
};

#endif /* LOGGER_H_ */
