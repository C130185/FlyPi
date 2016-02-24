/*
 * DummyLogger.h
 *
 *  Created on: Feb 24, 2016
 *      Author: teoko
 */

#ifndef DUMMYLOGGER_H_
#define DUMMYLOGGER_H_

#include <string>

#include "ILogger.h"

class DummyLogger: public ILogger {
public:
	DummyLogger();
	virtual ~DummyLogger();
	int open(std::string suffix);
	bool isOpen();
	void write(std::string msg);
	void close();
};

#endif /* DUMMYLOGGER_H_ */
