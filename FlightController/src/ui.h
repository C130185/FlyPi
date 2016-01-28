/*
 * UI.h
 *
 *  Created on: 27 Sep 2015
 *      Author: kp
 */

#ifndef UI_H_
#define UI_H_

#include <ncurses.h>
#include <string>

class UI {
public:
	UI() = delete;
	static void Init();
	static void End();
	static void UpdateStat(const std::string& stat);
	static void Print(const std::string& s);
private:
	static WINDOW* stat_win_;
	static WINDOW* log_win_;
};

#endif /* UI_H_ */
