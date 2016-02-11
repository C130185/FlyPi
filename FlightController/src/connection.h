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
		signed char type = kPacketTypeControl;
		signed char throttle = 0;
		signed char roll = 0;
		signed char pitch = 0;
		signed char yaw = 0;
	};

	struct __attribute__((__packed__)) CommandPacket {
		long long time = 0;
		signed char type = kPacketTypeCommand;
		signed char cmd = kCommandStop;
	};

	typedef std::function<void(ControlPacket)> CtrlListener;
	typedef std::function<void(CommandPacket)> CmdListener;
	typedef std::function<void(int errornum)> ErrorHandler;

	static const char kPacketTypeControl = 1;
	static const char kPacketTypeCommand = 2;
	static const char kCommandStart = 1;
	static const char kCommandStop = 2;

	Connection();
	virtual ~Connection() = default;
	virtual int Start() = 0;
	virtual void Stop() = 0;
	void AddCtrlListener(CtrlListener listener);
	void AddCmdListener(CmdListener listener);
	void AddErrorHandler(ErrorHandler handler);
protected:
	std::vector<CtrlListener> ctrl_listeners_;
	std::vector<CmdListener> cmd_listeners_;
	std::vector<ErrorHandler> error_handlers_;
};

#endif /* CONNECTION_H_ */
