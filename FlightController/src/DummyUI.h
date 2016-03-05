/*
 * DummyUI.h
 *
 *  Created on: Feb 12, 2016
 *      Author: teoko
 */

#ifndef DUMMYUI_H_
#define DUMMYUI_H_

#include <string>

#include "UI.h"

class DummyUI: public UI {
public:
	static DummyUI* getInstance();
	void init();
	void end();
	void updateStat(const std::string& stat);
	void print(const std::string& s);
	char readch();
private:
	DummyUI();
};

#endif /* DUMMYUI_H_ */
