/*
 * NCurseUI.cpp
 *
 *  Created on: 27 Sep 2015
 *      Author: kp
 */

#include "NCurseUI.h"

using namespace std;

NCurseUI::NCurseUI() {
}

NCurseUI* NCurseUI::getInstance() {
	static NCurseUI* instance = new NCurseUI();
	return instance;
}

void NCurseUI::init() {
	initscr();
	curs_set(0);
	noecho();
	statWin = newwin(9, COLS, LINES - 9, 0);
	logWin = newwin(getbegy(statWin), COLS, 0, 0);
	scrollok(logWin, true);
}

void NCurseUI::end() {
	delwin(statWin);
	delwin(logWin);
	endwin();
}

void NCurseUI::updateStat(const string& stat) {
	mtx.lock();
	wclear(statWin);
	mvwaddstr(statWin, 0, 0, stat.c_str());
	wrefresh(statWin);
	mtx.unlock();
}

void NCurseUI::print(const string& s) {
	mtx.lock();
	waddstr(logWin, s.c_str());
	wrefresh(logWin);
	mtx.unlock();
}

char NCurseUI::readch() {
	return getch();
}
