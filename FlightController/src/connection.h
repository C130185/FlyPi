/*
 * Connection.h
 *
 *  Created on: 20 Sep 2015
 *      Author: kp
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <functional>
#include <vector>

class Connection {
public:
	typedef std::function<void(char* msg, unsigned int length)> OnRecvListener;
	typedef std::function<void(int errornum)> ErrorHandler;

	Connection();
	virtual ~Connection() = default;
	virtual int Start() = 0;
	virtual void Stop() = 0;
	void AddOnRecvListener(OnRecvListener listener);
	void AddErrorHandler(ErrorHandler handler);
protected:
	std::vector<OnRecvListener> recv_listeners_;
	std::vector<ErrorHandler> error_handlers_;
};

#endif /* CONNECTION_H_ */
