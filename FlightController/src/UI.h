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
	virtual void Init() = 0;
	virtual void End() = 0;
	virtual void UpdateStat(const std::string& stat) = 0;
	virtual void Print(const std::string& s) = 0;
};

#endif /* UI_H_ */
