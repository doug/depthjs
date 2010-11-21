#include "WebSocketHandler.h"
#include "WebSocketConfig.h"

#include "Poco/NObserver.h"
//#include "Poco/Exception.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include <strnstr.h>

using Poco::Net::SocketReactor;
using Poco::Net::SocketAcceptor;
using Poco::Net::ReadableNotification;
using Poco::Net::WritableNotification;
using Poco::Net::ShutdownNotification;
using Poco::Net::ErrorNotification;
using Poco::Net::ServerSocket;
using Poco::Net::StreamSocket;
using Poco::NObserver;
using Poco::AutoPtr;

//--------------------------------------------------------------
WebSocketHandler::WebSocketHandler(StreamSocket& socket, SocketReactor& reactor):
	_socket(socket),
	_reactor(reactor),
	buffer(new char[BUFFER_SIZE])
{
	gotHandshake	= sentHandshake = false;
	origin			= DEFAULT_WEBSOCKET_ORIGIN;
	service			= DEFAULT_WEBSOCKET_SERVICE;
	resource		= DEFAULT_WEBSOCKET_RESOURCE;
	host			= DEFAULT_WEBSOCKET_HOST;
	port			= DEFAULT_WEBSOCKET_PORT;
	readyState		= CONNECTING;

	bReplicateHeaders = WEBSOCKET_ACCEPT_ALL_CONNECTIONS;
	bUseSizePreamble = false;
	
	_reactor.addEventHandler(_socket, NObserver<WebSocketHandler, ReadableNotification>	(*this, &WebSocketHandler::onReadable));
	_reactor.addEventHandler(_socket, NObserver<WebSocketHandler, WritableNotification>	(*this, &WebSocketHandler::onWritable));
	_reactor.addEventHandler(_socket, NObserver<WebSocketHandler, ShutdownNotification>	(*this, &WebSocketHandler::onShutdown));
	_reactor.addEventHandler(_socket, NObserver<WebSocketHandler, ErrorNotification>	(*this, &WebSocketHandler::onError));
	_reactor.addEventHandler(_socket, NObserver<WebSocketHandler, IdleNotification>		(*this, &WebSocketHandler::onIdle));
}

//--------------------------------------------------------------
WebSocketHandler::~WebSocketHandler()
{
	std::cout << "Remove[delete] handler" << std::endl;
	readyState = CLOSED;
	_reactor.removeEventHandler(_socket, NObserver<WebSocketHandler, ReadableNotification>	(*this, &WebSocketHandler::onReadable));
	_reactor.removeEventHandler(_socket, NObserver<WebSocketHandler, WritableNotification>	(*this, &WebSocketHandler::onWritable));
	_reactor.removeEventHandler(_socket, NObserver<WebSocketHandler, ShutdownNotification>	(*this, &WebSocketHandler::onShutdown));
	_reactor.removeEventHandler(_socket, NObserver<WebSocketHandler, ErrorNotification>		(*this, &WebSocketHandler::onError));
	_reactor.removeEventHandler(_socket, NObserver<WebSocketHandler, IdleNotification>		(*this, &WebSocketHandler::onIdle));
	delete [] buffer;
}

//--------------------------------------------------------------
void
WebSocketHandler::onReadable(const AutoPtr<ReadableNotification>& pNf)
{
	int n = _socket.receiveBytes(buffer, BUFFER_SIZE);
	const char* packet = handshake(buffer, n);

	if (packet)
	{
		int begin_idx=UNSET;
		for (int i=0; i<n; i++)
		{
			if (begin_idx==UNSET && packet[i] == 0x00)
				begin_idx = i;
			else if (begin_idx!=UNSET && (unsigned char)packet[i] == 0xFF)
			{
				std::string frame(packet+begin_idx+1,	//skip 0x00
								  i-begin_idx-1);		//skip 0xFF

				onmessage(frame);

				begin_idx = UNSET;
			}
		}
	}
}

//--------------------------------------------------------------
void
WebSocketHandler::onWritable(const AutoPtr<WritableNotification>& pNf)
{
	if (readyState==OPEN)
		ready();

//	if (!sentHandshake)
//		sendHandshake();
	
//	if (!gotHandshake)
//		getHandshake();
}

//--------------------------------------------------------------
void
WebSocketHandler::onIdle(const AutoPtr<IdleNotification>& pNf)
{
	idle();
}

//--------------------------------------------------------------
void
WebSocketHandler::onError(const AutoPtr<ErrorNotification>& pNf)
{
	std::cout << "Error'd" << std::endl;
}

//--------------------------------------------------------------
void
WebSocketHandler::onShutdown(const AutoPtr<ShutdownNotification>& pNf)
{
	std::cout << "Shutting down." << std::endl;
	readyState = CLOSED;
	onclose();
	delete this;
}

//--------------------------------------------------------------
int
WebSocketHandler::sendBytes(const char* buffer, int bufferSize)
{
	return _socket.sendBytes(buffer, bufferSize);
}

//--------------------------------------------------------------
int
WebSocketHandler::send(std::string& data)
{
	if (bUseSizePreamble)
	{
		std::string sze_bytes;
		sze_bytes = (char)0x80;

		int s = data.size();
		while (s > 0x7F) {
			sze_bytes += (char)(s & 0x7F | 0x80);
			s >>= 7;
		}
		sze_bytes += s & 0x7F;
		
		data = sze_bytes + data;
	}
	else {
		data = (char)0x00 + data + (char)0xFF;
	}

	return _socket.sendBytes(data.c_str(), data.size());
}

//--------------------------------------------------------------
std::string
WebSocketHandler::readHeader(const char* buffer, int n, std::string header)
{
	char *header_from=NULL, *header_to=NULL;
	header_from = strnstr(buffer, (header+": ").c_str(), n);
	if (header_from != NULL)
	{
		header_from += (header+": ").size();
		header_to = strnstr(header_from, "\r\n", (buffer+n)-header_from);
		if (header_to != NULL)
			return std::string(header_from, header_to - header_from);
	}
	return "";
}

//--------------------------------------------------------------
std::string
WebSocketHandler::readResource(const char* buffer, int n)
{
	char *resource_from=NULL, *resource_to=NULL;		
	resource_from = strnstr(buffer, "GET ", n);

	if (resource_from != NULL)
	{
		resource_from += strlen("GET ");
		resource_to = strnstr(resource_from, " ", (buffer+n)-resource_from);
		if (resource_to != NULL)
			return std::string(resource_from, resource_to - resource_from);
	}
	return "";
}

//--------------------------------------------------------------
const char*
WebSocketHandler::handshake(const char* buffer, int n)
{
	if (!sentHandshake)
	{
		std::cout << "New Client" << std::endl;
//		std::cout ": <" << std::string(buffer, n) << ">" << std::endl;
		sentHandshake = true;

		if (bReplicateHeaders)
		{
			std::string socket = readHeader(buffer, n, "Host");			
			std::string::size_type colon = socket.find_last_of(":");

			if (colon != std::string::npos)
			{
				host		= socket.substr(0, colon);
				port		= atoi(socket.substr(colon+1).c_str());
			}
			origin		= readHeader(buffer, n, "Origin");
			resource	= readResource(buffer, n);

			std::stringstream service_stream;
			service_stream << "ws://" << host << ":" << port << resource;
			service = service_stream.str();
		}

		std::string websocketHandshake = std::string(HANDSHAKE_PREAMBLE)
		+ "WebSocket-Origin: "	+ origin + "\r\n"
		+ "WebSocket-Location: "+ service+ "\r\n\r\n";

		std::cout << "sent handshake." << std::endl;
//		std::cout << ": <" << websocketHandshake << std::endl;
		_socket.sendBytes(websocketHandshake.c_str(), websocketHandshake.size());
	}

	if (!gotHandshake)
	{
		char* found = NULL;
		found = strnstr(buffer, HANDSHAKE_TERMINATION, n);
		if (found != NULL)
		{
			std::cout << "got handshake." << std::endl;
			gotHandshake = true;
			readyState = OPEN;

			found += strlen(HANDSHAKE_TERMINATION);
		}
		return found;
	}

	return buffer;
}
