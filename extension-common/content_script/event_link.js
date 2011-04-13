// EVENT LINK --------------------------------------------------------------------------------------
if (window.top === window) {
console.log("DepthJS: Loading Event Link");

DepthJS.eventLink.initPort = function() {
  if (DepthJS.verbose) console.log("DepthJS: Event link init");
  DepthJS.eventLink.domPort = null;
  $(function() {
    var $eventPort = $("#DepthJS_eventPort")
    if ($eventPort.length > 0 ) {
      console.log("DepthJS: Registering eventPort on DepthJS supported web page");
      DepthJS.eventLink.domPort = $eventPort;
    }
    //DepthJS.eventLink.domPort = $("<div id='DepthJS_eventPort' style='display:none'></div>");
    //DepthJS.eventLink.domPort.appendTo("body");
  });
  DepthJS.browser.addContentScriptListener("event", DepthJS.eventLink.onEvent);
};

DepthJS.eventLink.onEvent = function (msg) {
  DepthJS.logSortaVerbose(msg.type, "DepthJS: event " + msg.type);
  if (DepthJS.eventLink.domPort != null) {
    // jQuery's trigger doesn't seem to work here for some reason
    var event = document.createEvent("Event");
    event.initEvent(msg.type, false, false);
    DepthJS.eventLink.domPort.text(msg.jsonRep);
    DepthJS.eventLink.domPort.get(0).dispatchEvent(event);
  } else {
    var handler = DepthJS.eventHandlers["on"+msg.type];
    if (handler != null) {
      handler(msg.data);
    }
  }
};
}
