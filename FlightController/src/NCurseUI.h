/*
 * NCurseUI.h
 *
 *  Created on: 27 Sep 2015
 *      Author: kp
 */

#ifndef NCURSEUI_H_
#define NCURSEUI_H_

#include <ncurses.h>
#include <mutex>
#include <string>

#include "UI.h"

class NCurseUI: public UI {
public:
	static NCurseUI* getInstance();
	void init();
	void end();
	void updateStat(const std::string& stat);
	void print(const std::string& s);
	char readch();
private:
	WINDOW* statWin = nullptr;
	WINDOW* logWin = nullptr;
	std::mutex mtx;

	NCurseUI();
};

#endif /* NCURSEUI_H_ */
