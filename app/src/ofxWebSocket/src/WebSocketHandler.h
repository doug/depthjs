#pragma once

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/StreamSocket.h"

using Poco::Net::SocketReactor;
using Poco::Net::SocketAcceptor;
using Poco::Net::ReadableNotification;
using Poco::Net::WritableNotification;
using Poco::Net::ErrorNotification;
using Poco::Net::ShutdownNotification;
using Poco::Net::IdleNotification;
using Poco::Net::StreamSocket;
using Poco::AutoPtr;

class WebSocketHandler
{
public:
	WebSocketHandler(StreamSocket& socket, SocketReactor& reactor);
	~WebSocketHandler();
	
	int sendBytes(const char* buffer, int bufferSize);
	int send(std::string& data);

	enum readyStates {
		CONNECTING	= 0,
		OPEN		= 1,
		CLOSED		= 2,
	} readyState;

protected:
	bool bUseSizePreamble;
	
	virtual void ready()		{};
	virtual void idle()			{};
	virtual void onmessage(const std::string& frame)	{};
	virtual void onopen()		{};
	virtual void onclose()		{};

	std::string	origin;
	std::string	service;
	std::string	host;
	std::string	resource;
	unsigned short port;
	
private:
	void onReadable	(const AutoPtr<ReadableNotification>&	pNf);
	void onWritable	(const AutoPtr<WritableNotification>&	pNf);
	void onShutdown	(const AutoPtr<ShutdownNotification>&	pNf);
	void onError	(const AutoPtr<ErrorNotification>&		pNf);
	void onIdle		(const AutoPtr<IdleNotification>&		pNf);
	
	const char* handshake(const char* buffer, int n);

	enum
	{
		BUFFER_SIZE = 1024
	};

	std::string readHeader(const char* buffer=NULL, int n=0, std::string header="");
	std::string readResource(const char* buffer=NULL, int n=0);

	StreamSocket	_socket;
	SocketReactor&	_reactor;

	char*		buffer;	
	bool		sentHandshake, gotHandshake;
	bool		bReplicateHeaders;
};
