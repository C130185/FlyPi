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
	struct __attribute__((__packed__)) ControlPacket {
		long long time = 0;
		signed char type = PCK_TYPE_CONTROL;
		signed char throttle = 0;
		signed char roll = 0;
		signed char pitch = 0;
		signed char yaw = 0;
	};

	struct __attribute__((__packed__)) CommandPacket {
		long long time = 0;
		signed char type = PCK_TYPE_COMMAND;
		signed char cmd = COMMAND_STOP;
	};

	typedef std::function<void(ControlPacket)> CtrlListener;
	typedef std::function<void(CommandPacket)> CmdListener;
	typedef std::function<void()> LostConnListener;
	typedef std::function<void(int errornum)> ErrorHandler;

	static const char PCK_TYPE_CONTROL = 1;
	static const char PCK_TYPE_COMMAND = 2;
	static const char COMMAND_START = 1;
	static const char COMMAND_STOP = 2;

	Connection();
	virtual ~Connection() = default;
	virtual int start() = 0;
	virtual void stop() = 0;
	void addCtrlListener(CtrlListener listener);
	void addCmdListener(CmdListener listener);
	void addLostConnListener(LostConnListener listener);
	void addErrorHandler(ErrorHandler handler);
protected:
	std::vector<CtrlListener> ctrlListeners;
	std::vector<CmdListener> cmdListeners;
	std::vector<LostConnListener> lostConnListeners;
	std::vector<ErrorHandler> errorHandlers;
};

#endif /* CONNECTION_H_ */
