#pragma once

#define WEBSOCKET_ACCEPT_ALL_CONNECTIONS true
#define DEFAULT_WEBSOCKET_HOST		"ws://new-dilly.local:9000/fiducials"
#define DEFAULT_WEBSOCKET_PORT		9000
#define DEFAULT_WEBSOCKET_PORT_STR	"9000"
#define DEFAULT_WEBSOCKET_RESOURCE	"fiducials.pb"
#define DEFAULT_WEBSOCKET_SERVICE	"ws://"DEFAULT_WEBSOCKET_HOST":"DEFAULT_WEBSOCKET_PORT_STR"/"DEFAULT_WEBSOCKET_RESOURCE
#define DEFAULT_WEBSOCKET_ORIGIN	"http://robo-p-rimes.appspot.com"

#define HANDSHAKE_TERMINATION "\r\n\r\n"

#define HANDSHAKE_PREAMBLE "\
HTTP/1.1 101 Web Socket Protocol Handshake\r\n\
Upgrade: WebSocket\r\n\
Connection: Upgrade\r\n"

#define WEBSOCKET_HEADER(header, value) "WebSocket-"header": "value"\r\n"

#define UNSET -1
