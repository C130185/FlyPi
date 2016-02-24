/*
 * DummyLogger.cpp
 *
 *  Created on: Feb 24, 2016
 *      Author: teoko
 */

#include "DummyLogger.h"

DummyLogger::DummyLogger() {
}

DummyLogger::~DummyLogger() {
}

int DummyLogger::open(std::string suffix) {
	return 0;
}

bool DummyLogger::isOpen() {
	return true;
}

void DummyLogger::write(std::string msg) {
}

void DummyLogger::close() {
}
