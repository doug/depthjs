// EVENT LINK --------------------------------------------------------------------------------------
if (window.top === window) {
console.log("DepthJS: Loading Event Link");

DepthJS.eventLink.initPort = function() {
  if (DepthJS.verbose) console.log("DepthJS: Event link init");
  DepthJS.eventLink.domPort = $("<div id='DepthJS_eventPort' style='display:none'></div>");
  $(function() {
    DepthJS.eventLink.domPort.appendTo("body");
  });
  DepthJS.browser.addContentScriptListener("event", DepthJS.eventLink.onEvent);
};

DepthJS.eventLink.onEvent = function (msg) {
  DepthJS.logSortaVerbose("DepthJS: event " + msg.type);
  var handler = DepthJS.eventHandlers["on"+msg.type];
  if (handler != null) {
    handler(msg.data);
  }
  // jQuery's trigger doesn't seem to work here for some reason
  var event = document.createEvent("Event");
  event.initEvent(msg.type, false, false);
  DepthJS.eventLink.domPort.text(msg.jsonRep);
  DepthJS.eventLink.domPort.get(0).dispatchEvent(event);
};
}