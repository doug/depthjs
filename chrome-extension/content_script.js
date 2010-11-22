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
  eventLink: {}
};

// EVENT LINK --------------------------------------------------------------------------------------

DepthJS.eventLink.initPort = function() {
  console.log("DepthJS: Event link init");
  var $DepthJS_eventPort = $("<div id='DepthJS_eventPort' style='display:none'></div>");
  $DepthJS_eventPort.appendTo("body");
  var port = chrome.extension.connect({name: "event"});
  port.onMessage.addListener(DepthJS.eventLink.onEvent);
}

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

// EVENT HANDLERS ----------------------------------------------------------------------------------

DepthJS.eventHandlers.onSwipeLeft = function() {
  // We interpret as "back".
  history.go(-1);
}

DepthJS.eventHandlers.onSwipeRight = function() {
  // We interpret as "forward".
  history.go(1);
}

DepthJS.eventHandlers.onSwipeDown = function() {
  // We interpret as "scroll down 75% of window".
  var scrollAmount = Math.floor($(window).height() * 0.75);
  $("html, body").animate({
    scrollTop: ($(document).scrollTop() + scrollAmount)
  });
}


DepthJS.eventHandlers.onSwipeUp = function() {
  // We interpret as "scroll up 75% of window".
  var scrollAmount = Math.floor($(window).height() * 0.75);
  $("html, body").animate({
    scrollTop: ($(document).scrollTop() - scrollAmount)
  });
}

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
}


// Do the initialization
DepthJS.eventLink.initPort();
DepthJS.canvasLink.initDepth();
DepthJS.canvasLink.initImage();
