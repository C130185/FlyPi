/*
 * UI.h
 *
 *  Created on: Feb 12, 2016
 *      Author: teoko
 */

#ifndef UI_H_
#define UI_H_

class UI {
public:
	virtual ~UI() {
	}
	virtual void init() = 0;
	virtual void end() = 0;
	virtual void updateStat(const std::string& stat) = 0;
	virtual void print(const std::string& s) = 0;
	virtual char readch() = 0;
};

#endif /* UI_H_ */
