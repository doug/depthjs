var WS_CONNECTING = 0;
var WS_OPEN = 1;
var WS_CLOSING = 2;
var WS_CLOSED = 3;

DepthJS.backend.eventWs = null;
DepthJS.backend.imageWs = null;
DepthJS.backend.depthWs = null;
DepthJS.backend.host = "localhost";
DepthJS.backend.port = 8000;
DepthJS.backend.connecting = false;
DepthJS.backend.connected = false;

DepthJS.backend.connect = function() {
  DepthJS.backend.connecting = true;
  DepthJS.browser.sendMessageToPopup("connecting");
  var connected = 0;
  function check() {
    connected++;
    if (connected == 3) {
      if (DepthJS.verbose) console.log("All 3 connected");
      DepthJS.browser.sendMessageToPopup("connected");
      DepthJS.backend.connecting = false;
      DepthJS.backend.connected = true;
    }
  }

  // If we do not connect within a timeout period,
  // effectively cancel it and let the popup know.
  setTimeout(function() {
    if (connected != 3) {
      DepthJS.backend.disconnect();
    }
  }, 3000);

  return _.all(_.map(["event", "image", "depth"], function(stream) {
    var path = "ws://" + DepthJS.backend.host + ":" + DepthJS.backend.port + "/" + stream;
    if (DepthJS.verbose) console.log("Connecting to " + stream + " stream on " + path);

    // Clear out any old sockets
    var oldSocket = DepthJS.backend[stream+"Ws"];
    if (oldSocket != null) {
      oldSocket.onmessage = null;
      oldSocket.onclose = null;
      oldSocket.onopen = null;

      if (oldSocket.readyState == WS_OPEN ||
          oldSocket.readyState == WS_CONNECTING) {
        oldSocket.close();
      }
    }

    var socket = new WebSocket(path);
    DepthJS.backend[stream+"Ws"] = socket;

    socket.onmessage = function(data){
      DepthJS.backend.onMessage(stream, data);
    };

    socket.onclose = function() {
      DepthJS.backend.onDisconnect(stream);
    };

    socket.onopen = function() {
      DepthJS.backend.onConnect(stream);
      check();
    };

    return true;
  }));
};

DepthJS.backend.onMessage = function (stream, data) {
  if (stream == "event") {
    if (data === undefined || data.data == null) {
      return;
    }
    var msg = JSON.parse(data.data);
    if (!$.isPlainObject(msg)) {
      if (DepthJS.verbose) console.log('Unknown message: ' + data);
      return;
    }
    DepthJS.logSortaVerbose(msg.type, msg);
    var handler = DepthJS.eventHandlers["on"+msg.type];
    if (handler != null) {
      handler(msg.data);
    }

    msg.jsonRep = data.data;
    // Don't send to all--send to only the current tab.
    DepthJS.browser.sendMessageToActiveTab(msg);
  } else if (stream == "image") {
    /*DepthJS.eventHandlers.onImageMsg(data);*/
  } else if (stream == "depth") {
    /*DepthJS.eventHandlers.onDepthMsg(data);*/
  }
};

DepthJS.backend.disconnect = function() {
  DepthJS.backend.connected = false;
  if (DepthJS.verbose) console.log("Disconnecting");
  DepthJS.browser.sendMessageToPopup("disconnected");
  return _.map(["event", "image", "depth"], function(stream) {
    var oldSocket = DepthJS.backend[stream+"Ws"];
    if (oldSocket != null) {
      oldSocket.onmessage = null;
      oldSocket.onclose = null;
      oldSocket.onopen = null;

      if (oldSocket.readyState == WS_OPEN ||
          oldSocket.readyState == WS_CONNECTING) {
        oldSocket.close();
      }
    }
    DepthJS.backend[stream+"Ws"] = null;
  });
};

DepthJS.backend.onDisconnect = function (stream) {
  if (DepthJS.verbose) console.log("Disconnected on " + stream + " stream");
  // If one is closed, close them all.
  DepthJS.backend.disconnect();
};

DepthJS.backend.onConnect = function (stream) {
  if (DepthJS.verbose) console.log("Connect on " + stream + " stream");
};
