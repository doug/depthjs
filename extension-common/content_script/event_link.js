// EVENT LINK --------------------------------------------------------------------------------------
if (window.top === window) {
console.log("DepthJS: Loading Event Link");

DepthJS.eventLink.initPort = function() {
  if (DepthJS.verbose) console.log("DepthJS: Event link init");
  DepthJS.eventLink.domPort = null;
  var checkForEventPort = function() {
    var $eventPort = $("#DepthJS_eventPort")
    if ($eventPort.length > 0 ) {
      console.log("DepthJS: Registering eventPort on DepthJS supported web page");
      DepthJS.eventLink.domPort = $eventPort;
      DepthJS.eventLink.onEvent({type: "KinectInit", data:{}});
      return true;
    }
    return false;
  };
  $(function() {
    if (!checkForEventPort()) {
      // try again in 300msec
      setTimeout(checkForEventPort, 300);
    }
  });
  DepthJS.browser.addContentScriptListener("event", DepthJS.eventLink.onEvent);
};

DepthJS.eventLink.onEvent = function (msg) {
  DepthJS.logSortaVerbose(msg.type, "DepthJS: event " + msg.type);
  if (DepthJS.eventLink.domPort != null) {
    // jQuery's trigger doesn't seem to work here for some reason
    var e = document.createEvent("Event");
    e.initEvent("DepthJSEvent", false, false);
    if (msg.jsonRep == null) {
      msg.jsonRep = JSON.stringify(msg);
    };
    DepthJS.eventLink.domPort.text(msg.jsonRep);
    DepthJS.eventLink.domPort.get(0).dispatchEvent(e);
  } else {
    var handler = DepthJS.eventHandlers["on"+msg.type];
    if (handler != null) {
      handler(msg.data);
    }
  }
};
}
