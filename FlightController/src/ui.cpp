/*
 * UI.cpp
 *
 *  Created on: 27 Sep 2015
 *      Author: kp
 */

#include "ui.h"

#include <mutex>

using namespace std;

WINDOW* UI::stat_win_ = nullptr;
WINDOW* UI::log_win_ = nullptr;

mutex mtx;

void UI::Init() {
	initscr();
	curs_set(0);
	noecho();
	stat_win_ = newwin(9, COLS, LINES - 9, 0);
	log_win_ = newwin(getbegy(stat_win_), COLS, 0, 0);
	scrollok(log_win_, true);
}

void UI::End() {
	delwin(stat_win_);
	delwin(log_win_);
	endwin();
}

void UI::UpdateStat(const string& stat) {
	mtx.lock();
	wclear(stat_win_);
	mvwaddstr(stat_win_, 0, 0, stat.c_str());
	wrefresh(stat_win_);
	mtx.unlock();
}

void UI::Print(const string& s) {
	mtx.lock();
	waddstr(log_win_, s.c_str());
	wrefresh(log_win_);
	mtx.unlock();
}
