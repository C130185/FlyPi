/*
 * DummyUI.cpp
 *
 *  Created on: Feb 12, 2016
 *      Author: teoko
 */

#include "DummyUI.h"

#include <cstdio>

using namespace std;

DummyUI::DummyUI() {
}

DummyUI* DummyUI::getInstance() {
	static DummyUI instance;
	return &instance;
}

void DummyUI::init() {
}

void DummyUI::end() {
}

void DummyUI::updateStat(const std::string& stat) {
}

void DummyUI::print(const std::string& s) {
}

char DummyUI::readch() {
	return getchar();
}
