/*
 * DummyUI.cpp
 *
 *  Created on: Feb 12, 2016
 *      Author: teoko
 */

#include "DummyUI.h"

DummyUI::DummyUI() {
}

DummyUI* DummyUI::getInstance() {
	static DummyUI instance;
	return &instance;
}

void DummyUI::Init() {
}

void DummyUI::End() {
}

void DummyUI::UpdateStat(const std::string& stat) {
}

void DummyUI::Print(const std::string& s) {
}
