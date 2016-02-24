/*
 * ILogger.h
 *
 *  Created on: Feb 24, 2016
 *      Author: teoko
 */

#ifndef ILOGGER_H_
#define ILOGGER_H_

#include <string>

class ILogger {
public:
	virtual ~ILogger();
	virtual int open(std::string suffix) = 0;
	virtual bool isOpen() = 0;
	virtual void write(std::string msg) = 0;
	virtual void close() = 0;
protected:
	ILogger();
};

#endif /* ILOGGER_H_ */
