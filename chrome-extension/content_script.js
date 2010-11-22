/*
DepthJS
Copyright (C) 2010 Aaron Zinman, Doug Fritz, Roy Shilkrot

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

var DepthJS = {
  verbose: true,
  eventHandlers: {},
  canvasLink: {},
  eventLink: {},
  selectorBox: {},
  depthose: {},
  MAX_HANDPLANE_WIDTH: 100,
  MAX_HANDPLANE_HEIGHT: 100
};

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
DepthJS.state = null;

DepthJS.eventHandlers.onSwipeLeft = function() {
  // We interpret as "back".
  history.go(-1);
};

DepthJS.eventHandlers.onSwipeRight = function() {
  // We interpret as "forward".
  history.go(1);
};

DepthJS.eventHandlers.onSwipeDown = function() {
  // We interpret as "scroll down 75% of window".
  var scrollAmount = Math.floor($(window).height() * 0.75);
  $("html, body").animate({
    scrollTop: ($(document).scrollTop() + scrollAmount)
  });
};

DepthJS.eventHandlers.onSwipeUp = function() {
  // We interpret as "scroll up 75% of window".
  var scrollAmount = Math.floor($(window).height() * 0.75);
  $("html, body").animate({
    scrollTop: ($(document).scrollTop() - scrollAmount)
  });
};

// Occurs when the user puts their hand out flat onto a plane
DepthJS.eventHandlers.onRegister = function() {
  console.log("DepthJS: User registered their hand");
  $(window).trigger("touchstart");
  DepthJS.state = "selectorBox";
  DepthJS.selectorBox.show();
};

DepthJS.eventHandlers.onUnregister = function() {
  console.log("DepthJS. User removed their hand");
  $(window).trigger("touchend");
  DepthJS.state = null;
  DepthJS.selectorBox.hide();
  DepthJS.depthose.hide();
};

DepthJS.eventHandlers.onPush = function() {
  if (DepthJS.state == "selectorBox") {
    DepthJS.selectorBox.activate();
  } else if (DepthJS.state == "depthose") {
    DepthJS.depthose.select();
  }

  // They are now "unregistered"
  setTimeout(DepthJS.eventHandlers.onUnregister, 200);
};

// Right now users can either push or pull
DepthJS.eventHandlers.onPull = function() {
  DepthJS.state = "depthose";
  DepthJS.depthose.start();
}

DepthJS.eventHandlers.onMove = function(data) {
  if (data.x == null || data.y == null) {
    console.log(["Could not understand data", data]);
    return;
  }

  if (DepthJS.state == "depthose") {
    DepthJS.depthose.move(data.x, data.y);
  } else if (DepthJS.state == "selectorBox") {
    DepthJS.selectorBox.move(data.x * $(window).width() / 100,
                             data.y * $(window).height() / 100);
  } else {
    console.log("Ignoring move in state " + DepthJS.state);
  }
}

// SELECTOR BOX ------------------------------------------------------------------------------------

DepthJS.selectorBox.init = function() {
  var $box = $("<div id='DepthJS_box'></div>");
  $box.css("height", "100px")
      .css("width", "100px")
      .css("border", "2px solid #586F82")
      .css("background-color", "#B8FF71")
      .css("opacity", "0.5")
      .css("z-index", "100000")
      .css("position", "fixed")
      .css("left", "0")
      .css("top", "0")
      .appendTo("body").hide();
  DepthJS.selectorBox.$box = $box;
};

DepthJS.selectorBox.show = function() {
  DepthJS.selectorBox.$box.show();
};

DepthJS.selectorBox.hide = function() {
  DepthJS.selectorBox.$box.hide();
};

DepthJS.selectorBox.move = function(x, y) {
  console.log("move");
  var $box = DepthJS.selectorBox.$box;
   // Constrain to window
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  x = Math.min(x, $(window).width() - $box.width());
  y = Math.min(y, $(window).height() - $box.height());

  if (x != $box.css("left") || y != $box.css("top")) {
    $box.animate({
      left: x,
      top: y
    });
  }
};

DepthJS.selectorBox.activate = function() {
  console.log("DepthJS: Activating underneath selectorBox");
  // Lame code for now...

  var $intersectingLinks = $("a").filter(function() {
    var $a = $(this);
    var ax = $a.offset().left + $(window).scrollLeft();
    var aw = $a.width();
    var ay = $a.offset().top + $(window).scrollTop();
    var ah = $a.height();

    var $box = DepthJS.selectorBox.$box;
    var bx = $box.position().left;
    var by = $box.position().top;
    var bw = $box.width();
    var bh = $box.height();

    if (by > ay + ah || // box-top is lower than link-bottom
        by + bh < ay || // box-bottom is higher than link-top
        bx > ax + aw || // box-left is right of link right
        bx + bw < aw) { // box-right is left of link left
      return false;
    }
    return true;
  });

  console.log("Got " + $intersectingLinks.length + " links");
  if ($intersectingLinks.length > 0) {
    // Trigger a click on them
    console.log($intersectingLinks);
    $intersectingLinks.eq(0).click();
  }
};

// EVENT LINK --------------------------------------------------------------------------------------

DepthJS.eventLink.initPort = function() {
  console.log("DepthJS: Event link init");
  var $DepthJS_eventPort = $("<div id='DepthJS_eventPort' style='display:none'></div>");
  $DepthJS_eventPort.appendTo("body");
  var port = chrome.extension.connect({name: "event"});
  port.onMessage.addListener(DepthJS.eventLink.onEvent);
  DepthJS.eventLink.port = port;
};

DepthJS.eventLink.onEvent = function (msg) {
  if (DepthJS.verbose) console.log("DepthJS: event " + msg.type);
  var handler = DepthJS.eventHandlers["on"+msg.type];
  if (handler != null) {
    handler(msg.data);
  }
  // jQuery's trigger doesn't seem to work here for some reason
  var event = document.createEvent("Event");
  event.initEvent(msg.type, false, false);
  $("#DepthJS_eventPort").text(msg.jsonRep);
  $("#DepthJS_eventPort").get(0).dispatchEvent(event);
};

// CANVAS LINK -------------------------------------------------------------------------------------

DepthJS.canvasLink.initDepth = function() {
  var $depthCanvas = $("canvas#DepthJS_depth");
  if ($depthCanvas.length > 0) {
    console.log("DepthJS: Will write to depth canvas");
    var depthCanvas = $depthCanvas.get(0);
    var c = depthCanvas.getContext("2d");

    // read the width and height of the canvas
    var w = 640;
    var h = 480;
    var imageData = c.createImageData(w, h);

    var port = chrome.extension.connect({name: "depth"});
    port.onMessage.addListener(function(depthData) {
      // depthData is 255-valued depth repeated 640x480 times
      var imgPtr = 0;
      for (var ptr = 0; ptr < depthData.length; ptr++) {
        imageData.data[imgPtr++] = depthData[ptr]; // R
        imageData.data[imgPtr++] = depthData[ptr]; // G
        imageData.data[imgPtr++] = depthData[ptr]; // B
        imageData.data[imgPtr++] = 0xFF; // Alpha
      }
      c.putImageData(imageData, 0, 0);
    });

    // Start with all gray
    for (var i = 0; i < imageData.data.length; i+=4) {
      imageData.data[i+0] = 0x55; // R
      imageData.data[i+1] = 0x55; // G
      imageData.data[i+2] = 0x55; // B
      imageData.data[i+3] = 0xFF; // Alpha
    }
    c.putImageData(imageData, 0, 0);
  }
};

DepthJS.canvasLink.initImage = function () {
  var $imageCanvas = $("canvas#DepthJS_image");
  if ($imageCanvas.length > 0) {
    console.log("DepthJS: Will write to image canvas");
    var imageCanvas = $imageCanvas.get(0);
    var c = imageCanvas.getContext("2d");

    // read the width and height of the canvas
    var w = 640;
    var h = 480;
    var imageData = c.createImageData(w, h);

    var port = chrome.extension.connect({name: "image"});
    port.onMessage.addListener(function(rawData) {
      // rawData is RGB repeated 640x480 times
      var imgPtr = 0;
      for (var ptr = 0; i < rawData; i+=3) {
        imageData.data[imgPtr++] = rawData[ptr+0]; // R
        imageData.data[imgPtr++] = rawData[ptr+1]; // G
        imageData.data[imgPtr++] = rawData[ptr+2]; // B
        imageData.data[imgPtr++] = 0xFF; // Alpha
      }
      c.putImageData(imageData, 0, 0);
    });

    // Start with all blue
    for (var i = 0; i < imageData.data.length; i+=4) {
      imageData.data[i+0] = 0; // R
      imageData.data[i+1] = 0; // G
      imageData.data[i+2] = 0xFF; // B
      imageData.data[i+3] = 0xFF; // Alpha
    }
    c.putImageData(imageData, 0, 0);
  }
};




// DEPTHOSE

DepthJS.depthose.$div = null;
DepthJS.depthose.windows = null;
DepthJS.depthose.start = function() {
  DepthJS.eventLink.port.postMessage({type: "getThumbnailUrls"});
};
DepthJS.eventHandlers.onThumbnailUrls = function(response) {
  console.log(["REPSONE", response]);
  var windows = response.windows;
  DepthJS.depthose.windows = windows; // Make a local copy in case these objects are reused
  console.log(["got windows", windows]);
  DepthJS.depthose.show();
};


DepthJS.depthose.show = function() {
  if (DepthJS.depthose.windows == null) {
    DepthJS.depthose.start();
    return;
  }

  console.log("DepthJS: Entering Depthose");
  console.log(["Starting depthose with", DepthJS.depthose.windows]);
  $("#DepthJS_depthose").remove();
  DepthJS.depthose.$div = $("<div id='DepthJS_depthose'></div>");
  var $div = DepthJS.depthose.$div;
  $div.css("position", "fixed")
      .css("width", "100%")
      .css("height", "100%")
      .css("background-color", "#333")
      .css("z-index", "10000")
      .css("top", "0")
      .css("left", "0")
      .addClass("zflow")
      .appendTo("body");

  $div.append("<div class='centering'><div id='DepthJS_tray' class='tray'></div></div>");
  var images = _.pluck(DepthJS.depthose.windows, "dataUrl");
  images = _.reject(images, function(el) { el == null; });
  console.log("after filtering we have " + images.length + " done.");
  if (images.length > 0) {
    console.log("starting zflow");
    zflow(images, "#DepthJS_tray");

  } else {
    console.log("Not showing Depthose--no windows to show");
  }
};

DepthJS.depthose.hide = function() {
  if (DepthJS.depthose.$div == null) return;
  console.log("DepthJS: Exiting Depthose");
  DepthJS.depthose.$div.remove();
  DepthJS.depthose.$div = null;
};

DepthJS.depthose.move = function(x, y) {
  console.log("DepthJS: Move depthose");
}

DepthJS.depthose.select = function() {
  console.log("DepthJS: Selecting");
}

// Do the initialization
DepthJS.selectorBox.init();
DepthJS.eventLink.initPort();
DepthJS.canvasLink.initDepth();
DepthJS.canvasLink.initImage();
