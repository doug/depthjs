/*
DepthJS
Copyright (C) 2010 Aaron Zinman, Doug Fritz, Roy Shilkrot, and Greg Elliott

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


console.log('background.html Starting DepthJS');
var DepthJS = {
  __VERSION__: '0.3',
  verbose: true,
  backend: {},
  eventHandlers: {},
  cv: {},
  tools: {},
  portsByTabId: {},
  tabs: {},
  test: {},
  toolbar: {},
  browser: {},
  background: {},
  registerMode: "selectorBox",
  pluginObj: null
};

DepthJS.init = function (pluginObj) {
  console.log("Initing DepthJS background");
  DepthJS.pluginObj = pluginObj;
  DepthJS.initBrowserBackground();
  DepthJS.browser.addBackgroundListener(DepthJS.background.handleMessage);
  if (DepthJS.verbose) console.log("Connecting to Backend");
  if (!DepthJS.backend.connect()) {
    if (DepthJS.verbose) console.log("Couldn't connect... aborting");
    return false;
  }
  console.log("Init complete");
  return true;
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
  console.log("------" + (new Date() + ""));
  _.each(alphabeticalKeys, function(type) {
    console.log(["   " + counts[type] + " " + type + "; last = ", lastByType[type]]);
  });

  lastMessages = [];
}
setTimeout(print, 1000);

})();
