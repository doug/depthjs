/*
DepthJS
Copyright (C) 2010 Aaron Zinman, Doug Fritz, Roy Shilkrot, Greg Elliott

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

/**
 * Include this file in your HTML pages to access the Kinect via DepthJS plugins.
 * If the user has the DepthJS plugin installed in their browser, then your webpage will be
 * able to get high-level events through this simple API.
 *
 * Just override the definition of the event handlers in the global DepthJS object with your own
 * functions.
 *
 * See BasicDemo.html or our "interesting" CatBucket game.
 *
 * NOTE: depth.js requires jQuery. You can use it in noConflict mode.
 */

// Override window.DepthJS
window.DepthJS = {
  onKinectInit: function() {},
  onRegister: function(x, y, z, data) {},
  onUnregister: function() {},
  onMove: function(x, y, z) {},
  onSwipeLeft: function() {},
  onSwipeRight: function() {},
  onSwipeDown: function() {},
  onSwipeUp: function() {},
  onPush: function() {},
  onPull: function() {}
};

// Requires jQuery
(function($){
  $(function() {
    var $domPort = $("<div id='DepthJS_eventPort' style='display:none'></div>");
    $domPort.appendTo("body");
    $domPort.bind("DepthJSEvent", function() {
      var json = $domPort.text();
      var eventObj = JSON.parse(json);
      var type = eventObj.type;
      if (type == null) {
        console.log("DepthJS: No type found in event; ignoring");
        return;
      }
      if (DepthJS["on" + type] == null) {
        console.log("DepthJS: Could not find handler for event type " + type + "; ignoring");
        return;
      }
      var data = eventObj.data;
      if (type == "Register") DepthJS.onRegister(data.x, data.y, data.z, data.data);
      else if (type == "Move") DepthJS.onMove(data.x, data.y, data.z);
      else DepthJS["on" + type](data.data);
    });
  }); // wait until body is ready
})(jQuery); // no conflict
