/*
 * NCurseUI.h
 *
 *  Created on: 27 Sep 2015
 *      Author: kp
 */

#ifndef NCURSEUI_H_
#define NCURSEUI_H_

#include <ncurses.h>
#include <string>

#include "UI.h"

class NCurseUI: public UI {
public:
	static NCurseUI* getInstance();
	void Init();
	void End();
	void UpdateStat(const std::string& stat);
	void Print(const std::string& s);
private:
	NCurseUI();

	WINDOW* stat_win_ = nullptr;
	WINDOW* log_win_ = nullptr;
};

#endif /* NCURSEUI_H_ */
