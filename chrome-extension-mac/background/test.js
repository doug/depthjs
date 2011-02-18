DepthJS.test.depthoseTest = function () {
  DepthJS.test.runTestSequence([
    {type:"DepthoseMode", data:{}},
    {type:"Register", data:{}},
    {type:"Pull", data:{}},
    {type:"Move", data:{x: 15, y: 15}},
    {type:"HandClick", data:{}},
    {type:"Unregister", data:{}}
    //{type:"Register", data:{}},
    //{type:"Pull", data:{}},
    //{type:"Move", data:{x: -15, y: 15}},
    //{type:"Move", data:{x: 40, y: 80}},
    //{type:"Push", data:{}}
  ].reverse());
};

DepthJS.test.pannerTest = function () {
  DepthJS.test.runTestSequence([
    {type:"PannerMode", data:{}},
    {type:"Register", data:{}},
    {type:"Move", data:{x: 60, y: 0}},
    {type:"Move", data:{x: 60, y: 15}},
    {type:"Move", data:{x: 60, y: 25}},
    {type:"Move", data:{x: 25, y: 25}},
    {type:"Unregister", data:{}}
  ].reverse());
};

DepthJS.test.selectorBoxTest = function () {
  DepthJS.test.runTestSequence([
     {type:"SelectorBoxMode", data:{}},
     {type:"Register", data:{}},
     {type:"Move", data:{x: 50, y: 50}},
     {type:"HandClick", data:{}},
     {type:"Move", data:{x: 60, y: 0}},
     {type:"Move", data:{x: 60, y: 15}},
     {type:"Move", data:{x: 60, y: 25}},
     {type:"Move", data:{x: 60, y: 35}},
     {type:"Move", data:{x: 60, y: 45}},
     {type:"Move", data:{x: 60, y: 55}},
     {type:"Move", data:{x: 60, y: 65}},
     {type:"Move", data:{x: 60, y: 75}},
     {type:"Move", data:{x: 60, y: 85}},
     {type:"Move", data:{x: 60, y: 95}},
     {type:"Move", data:{x: 60, y: 100}},
     {type: "SwipeLeft", data: {}},
     {type:"Unregister", data:{}}
   ].reverse());
};

DepthJS.test.gestureTest = function() {
  DepthJS.test.runTestSequence([
    {type:"SwipeDown", data: {}},
    {type:"SwipeUp", data: {}},
    {type:"SwipeRight", data: {}},
    {type:"SwipeLeft", data: {}}
  ].reverse());
};

DepthJS.test.runTestSequence = function (testEvents) {
  if (DepthJS.verbose) console.log("Starting new test sequence");
  // Cancel all other ones
  for (var i = 0; i < 20; i++) { DepthJS.test.currentTest.pop(); }
  // Make new test with closure
  DepthJS.test.currentTest = testEvents;
  function next() {
    var event = testEvents.pop();
    if (event != undefined) {
      DepthJS.test.sendTestEvent(event, function() {
        setTimeout(next, 1000);
      });
    }
  }
  next();
};

DepthJS.test.currentTest = [];

DepthJS.test.sendTestEvent = function(msg, callback) {
  if (DepthJS.verbose) console.log("Sending out test event " + msg.type);
  msg.jsonRep = JSON.stringify(msg);
  DepthJS.browser.sendMessageToActiveTab(msg);
  if (callback != null) callback();
};
