#pragma once

#include "ofxThread.h"

#include "Poco/Net/SocketReactor.h"
using Poco::Net::SocketReactor;

class WebSocketServer : public ofxThread
{
public:
	WebSocketServer();
	~WebSocketServer();	

	void run();
	void stop();

	template<class HandlerT>
	void threadedFunction();
	
	unsigned short port;
	
private:
	SocketReactor reactor;
};
