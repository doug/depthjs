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
if (window.top === window) {
console.log("DepthJS: Loading Root");

var NOT_VERBOSE = 0;
var VERBOSE = 1;
var STUPID_VERBOSE = 2;

var DepthJS = {
  verbose: VERBOSE,
  registerMode: "selectorBox",
  eventHandlers: {},
  eventLink: {},
  selectorBox: {},
  selectorBoxPopup: {},
  panner: {},
  depthose: {},
  browser: {},
  MAX_HANDPLANE_WIDTH: 100,
  MAX_HANDPLANE_HEIGHT: 100
};

(function() {
var lastMessages = [];
DepthJS.logSortaVerbose = function(type, fullMessage) {
  lastMessages.push({type: type, data:fullMessage});
};

function print() {
  setTimeout(print, 1000);
  if (lastMessages.length == 0) return;
  var counts = {};
  var lastByType = {};
  _.each(lastMessages, function(msg) {
    if (counts[msg.type] == null) counts[msg.type] = 0;
    counts[msg.type] = counts[msg.type] + 1;
    lastByType[msg.type] = msg.data;
  });
  
  var alphabeticalKeys = _.keys(counts).sort();
  // console.log("------" + (new Date() + ""));
  _.each(alphabeticalKeys, function(type) {
    console.log(["   " + counts[type] + " " + type + "; last = ", lastByType[type]]);
  });
  
  lastMessages = [];
}
setTimeout(print, 1000);

})();
}