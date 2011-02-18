// EVENT HANDLERS ----------------------------------------------------------------------------------

/**
 * DepthJS here maps gestures to interactions with this web page.
 *
 * Users interact with the system in two different "modes".
 *
 * 1) Lazy gestures
 * 2) Virtual pointer
 *
 * LAZY GESTURES:
 * By default, the user can lazily make swiping motions with their whole hand:
 * up, down, left and right.
 *
 * The user does not leave her hand still in the air, but instead rests it hand out of frame.
 *
 * The swipes provide basic level navigation--moving forward and backward in history and
 * moving the web page up and down.
 *
 * VIRTUAL POINTER:
 * Once the hand is registered, the user manipulates a cursor along a parallel 2D plane to the monitor.
 *
 * The cursor can be moved, "pushed", and "pulled". Moving the cursor moves a visible selection box
 * around the web page. Pushing and pulling activate (click) any links below the selection box.
 **/

if (window.top === window) {
console.log("DepthJS: Loading event handlers");

DepthJS.state = null;

DepthJS.eventHandlers.onSwipeLeft = function() {
   // history.back();
};

DepthJS.eventHandlers.onSwipeRight = function() {
  // We interpret as "forward".
  // history.forward();
};

DepthJS.eventHandlers.onSwipeDown = function() {
  // We interpret as "scroll down 75% of window".
  // var scrollAmount = Math.floor($(window).height() * 0.75);
  // $("html, body").animate({
  //   scrollTop: ($(document).scrollTop() + scrollAmount)
  // });
};

DepthJS.eventHandlers.onSwipeUp = function() {
  // We interpret as "scroll up 75% of window".
  // var scrollAmount = Math.floor($(window).height() * 0.75);
  // $("html, body").animate({
  //   scrollTop: ($(document).scrollTop() - scrollAmount)
  // });
};

DepthJS.eventHandlers.onHandPointer = function(){
  if (DepthJS.verbose) console.log("DepthJS. Hand Pointer");
  DepthJS.eventHandlers.onUnregister();
  DepthJS.state = "selectorBox";
};

DepthJS.eventHandlers.onHandOpen = function(){
  if (DepthJS.verbose) console.log("DepthJS. Hand Open");
  DepthJS.eventHandlers.onUnregister();
  DepthJS.state = "panner";
  DepthJS.panner.show();
};

DepthJS.eventHandlers.onDepthoseMode = function() {
  DepthJS.registerMode = "depthose";
};

DepthJS.eventHandlers.onPannerMode = function() {
  DepthJS.registerMode = "panner";
};

DepthJS.eventHandlers.onSelectorBoxMode = function() {
  DepthJS.registerMode = "selectorBox";
};

// POINTER -----------------------------------------------------------------------------------------
DepthJS.eventHandlers.onRegister = function(data) {
  if (DepthJS.verbose) console.log("DepthJS: User registered their hand");
  $(window).trigger("touchstart");
  if (data.mode == "theforce") {
    DepthJS.registerMode = "selectorBox";
  } else if (data.mode == "twohands") {
    DepthJS.registerMode = "depthose";
  } else if (data.mode == "openhand") {
    DepthJS.registerMode = "panner";
  } else {
    console.log(["DID NOT UNDERSTAND MODE: ", data.mode]);
  }
  DepthJS.state = DepthJS.registerMode;
  DepthJS[DepthJS.registerMode].show();
};

DepthJS.eventHandlers.onUnregister = function() {
  if (DepthJS.verbose) console.log("DepthJS. User removed their hand");
  DepthJS.state = null;
  DepthJS.panner.hide();
  DepthJS.selectorBox.hide();
  DepthJS.selectorBoxPopup.hide();
  DepthJS.depthose.hide();
};

DepthJS.eventHandlers.onHandClick = function() {
  if (DepthJS.state == "selectorBoxPopup") {
    DepthJS.selectorBoxPopup.openHighlightedLink();
  } else if (DepthJS.state == "selectorBox") {
    DepthJS.selectorBox.activate();
  } else if (DepthJS.state == "depthose") {
    DepthJS.depthose.select();
  }

  // They are now "unregistered"
  // setTimeout(DepthJS.eventHandlers.onUnregister, 200);
};

// Right now users can either push or pull
DepthJS.eventHandlers.onPull = function() {
  DepthJS.state = "depthose";
  DepthJS.depthose.start();
};

(function() {
var accumulatedX = null;
var accumulatedY = null;
var accumulatedZ = null;
var smoothing = 0.95;
DepthJS.eventHandlers.onMove = function(data) {
  if (data.x == null || data.y == null || data.z == null) {
    if (DepthJS.verbose) console.log(["Could not understand data", data]);
    return;
  }

  data.x = 100-data.x;

  if (accumulatedX == null) {
    accumulatedX = data.x;
    accumulatedY = data.y;
    accumulatedZ = data.z;
  } else {
    accumulatedX = accumulatedX * smoothing + data.x * (1-smoothing);
    accumulatedY = accumulatedY * smoothing + data.y * (1-smoothing);
    accumulatedZ = accumulatedZ * smoothing + data.z * (1-smoothing);
  }

  if (DepthJS.state == "panner"){
    DepthJS.panner.move(accumulatedX, accumulatedY, accumulatedZ);
  } else if (DepthJS.state == "depthose") {
    DepthJS.depthose.move(accumulatedX, accumulatedY, accumulatedZ);
  } else if (DepthJS.state == "selectorBox") {
    DepthJS.selectorBox.move(accumulatedX * $(window).width() / 100,
                             accumulatedY * $(window).height() / 100);
  } else if (DepthJS.state == "selectorBoxPopup") {
    DepthJS.selectorBoxPopup.move(accumulatedX, accumulatedY, accumulatedZ);
  } else {
    if (DepthJS.verbose) console.log("Ignoring move in state " + DepthJS.state);
  }
};
})();
}