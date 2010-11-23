#include "WebSocketServer.h"
#include "WebSocketHandler.h"

#include "WebSocketConfig.h"

using Poco::Net::SocketAcceptor;
using Poco::Net::ServerSocket;

//--------------------------------------------------------------
WebSocketServer::WebSocketServer()
{
	port = DEFAULT_WEBSOCKET_PORT;
}

//--------------------------------------------------------------
WebSocketServer::~WebSocketServer()
{
	stop();
}

//--------------------------------------------------------------
void
WebSocketServer::run()
{
	if (!isThreadRunning())
		startThread(false, false); // blocking, non-verbose
}

//--------------------------------------------------------------
void
WebSocketServer::stop()
{
	if (isThreadRunning())
	{
		reactor.stop();
		stopThread();
	}
}

//--------------------------------------------------------------
template<class HandlerT>
void
WebSocketServer::threadedFunction()
{
	ServerSocket svs(port);
	SocketAcceptor<HandlerT> acceptor(svs, reactor);

	reactor.run();
}
