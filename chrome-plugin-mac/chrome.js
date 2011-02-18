if (DepthJS) {
console.log("DepthJS Loading Chrome shit");

DepthJS.initBrowserBackground = function() {
  console.log("DepthJS: Initing Chrome background");
  // Choose the NPAPI-based backend
  DepthJS.backend.connect = DepthJS.npBackend.connect;
  DepthJS.backend.disconnect = DepthJS.npBackend.disconnect;

  console.log("DepthJS: Initing port listener");
  chrome.extension.onConnect.addListener(function(port) {
    var name = port.name;
    if (DepthJS.verbose) console.assert(name == "event" || name == "image" || name == "depth");
    if (DepthJS.verbose) console.log(name + " port connected");
    //var listeners =  DepthJS[name + "Listeners"];
    //listeners.push(port);

    var tabId = port.sender.tab.id;
    var tabPorts = DepthJS.portsByTabId[tabId];
    if (tabPorts == null) {
      tabPorts = {}; DepthJS.portsByTabId[tabId] = tabPorts;
    }
    tabPorts[name] = port;

    port.onDisconnect.addListener(function (e) {
      if (DepthJS.verbose) console.log(name + " port disconnected on tab " + tabId);
      //DepthJS[name + "Listeners"] = _.reject(
        //listeners, function(el) { el === port; });
      var _tabPorts = DepthJS.portsByTabId[tabId];
      if (_tabPorts) {
        delete _tabPorts[name];
        if (_.isEmpty(_tabPorts)) {
          if (DepthJS.verbose) console.log("for all ports on this tab");
          delete DepthJS.portsByTabId[tabId];
        }
      }
    });
  });
};

if (DepthJS.tabs) {
DepthJS.tabs.selectTab = function(tabId) {
  console.log("Selecting tabId " + tabId);
  chrome.tabs.update(tabId, {selected: true});
};

DepthJS.tabs.populateActiveWindowCache = function(callback) {
  chrome.windows.getCurrent(function(windowObj) {
    DepthJS.tabs.activeWindowTabCache = _.filter(_.map(windowObj.tabs, function(tab) {
      return DepthJS.tabs.thumbnailCache[tab.id];
    }), function(obj) { return obj != null; });

    callback(DepthJS.tabs.activeWindowTabCache);
  });
};

DepthJS.tabs.thumbnailCache = {};
DepthJS.tabs.tabCache = {};
DepthJS.tabs.windowCache = {};

DepthJS.tabs.init = function() {
  DepthJS.tabs.populateCaches();

  // Subscribe to event handlers
  chrome.tabs.onCreated.addListener(function(tab) {
    // console.log('new tab created: ' + obj_repr(tab, 'Tab'));
    // But not yet loaded--wait for onUpdated
    DepthJS.tabs.tabCache[tab.id] = tab;
  });

  chrome.tabs.onRemoved.addListener(DepthJS.tabs.onClosedTab);

  chrome.tabs.onUpdated.addListener(DepthJS.tabs.onTabUpdated);

  chrome.windows.onRemoved.addListener(function(windowId) {
    // console.log('window removed: id=' + windowId);
  });
};

DepthJS.tabs.populateCaches = function() {
  var obj_repr = DepthJS.tools.obj_repr;
  // Go through all existing tabs/windows and add to cache
  chrome.windows.getAll({populate: true}, function(windows) {
    for (var i = 0; i < windows.length; ++i) {
      var window = windows[i];
      if (DepthJS.verbose) console.log('Adding existing window '+ obj_repr(window, 'Window'));
      for (var j = 0; j < window.tabs.length; ++j) {
        var tab = window.tabs[j];
        if (DepthJS.verbose) console.log('Adding existing tab ' + obj_repr(tab, 'Tab'));
        DepthJS.tabs.tabCache[tab.id] = tab;

        if (tab.status == 'complete') {
          // We should add this already loaded tab
          if (tab.selected) { // since we are only doing a screenshot on new
            DepthJS.tabs.onNewVisibleTab(tab.url, tab.title, window.id, tab.id);
          }
        }
      }
      delete window.tabs; // Unnecessary... it'll be too stale to do anything with.
      DepthJS.tabs.windowCache[window.id] = window;
    }
  });
};

DepthJS.tabs.onTabUpdated = function(tabId, changeInfo, tab) {
  var cachedTab = DepthJS.tabs.tabCache[tabId];
  if (!cachedTab) {
    DepthJS.logSortaVerbose('Could not find tabId ' + tabId + ' in cache: ' + cachedTab);
    return;
  }

  if (changeInfo.status == 'complete' && cachedTab.status == 'loading') {
    if (tab.selected) DepthJS.tabs.onNewVisibleTab(tab.url, tab.title, tab.id);
  } else if (tab.url != cachedTab.url) {
    DepthJS.tabs.onClosedTab(cachedTab.id);
    if (tab.status == 'complete') {
      // Never the case?
      if (tab.selected) DepthJS.tabs.onNewVisibleTab(tab.url, tab.title, tab.id);
    }
  }
  // Save state
  delete DepthJS.tabs.tabCache[tabId]; // help GC
  DepthJS.tabs.tabCache[tabId] = tab;
};

DepthJS.tabs.onNewVisibleTab = function(url, title, windowId, tabId) {
  // Take screenshot
  var capture = function(windowId) {
    chrome.tabs.captureVisibleTab(windowId, null, function(dataUrl) {
      if (DepthJS.verbose) console.log("captured thumbnail for tabId " + tabId);
      DepthJS.tabs.thumbnailCache[tabId] = {
        dataUrl: dataUrl,
        title: title};
    });
  }
  if (tabId != null) {
    capture(windowId);
  } else {
    tabId = windowId; // variable num args in function call, jquery style.
    chrome.tabs.get(tabId, function(tab) { capture(tab.windowId); });
  }
};

DepthJS.tabs.onClosedTab = function(tabId) {
  if (DepthJS.portsByTabId[tabId] != null) {
    if (DepthJS.verbose) console.log("Had ports by closed tab laying around, deleting");
    delete DepthJS.portsByTabId[tabId];
  }
  // Close page
  var cachedTab = DepthJS.tabs.tabCache[tabId];
  if (!cachedTab) {
    if (DepthJS.verbose) console.log('Could not find tabId ' + tabId + ' in cache: ' + cachedTab);
    return;
  }

  delete DepthJS.tabs.tabCache[tabId];
  if (DepthJS.tabs.thumbnailCache[tabId] != null) {
    delete DepthJS.tabs.thumbnailCache[tabId];
  }
};

DepthJS.tabs.onReaccessedTab = function(url, tabId) {
};
} // end if DepthJS.tabs




if(!DepthJS.tools) DepthJS.tools = {};
DepthJS.tools.obj_repr = function (obj, className) {
  var buf = [];
  if (className === undefined) {
    buf.push('[Object ');
  } else {
    buf.push('[' + className + ' ');
  }
  for (var key in obj) {
    buf.push(key + '=' + obj[key]);
    buf.push(', ');
  }
  buf.pop();
  buf.push(']');
  return buf.join('');
};


/**
 * Typically used for background.html. When a message is passed in, it passes to the
 * callback two parameters: action & data.
 */
DepthJS.browser.addBackgroundListener = function(callback) {
  chrome.extension.onRequest.addListener(function(req, sender, sendResponse) {
    callback(req.action, req.data, sendResponse);
  });

  /*
  safari.application.addEventListener("message", function(e) {
    var action = e.name;
    var data = e.message;
    var reply = function(messageType, data) {
      e.target.page.dispatchMessage(messageType, data);
    };
    callback(action, data, reply);
  });
  */
};

/**
 * Typically used for content scripts. When a message is passed in, it passes to the
 * callback the data attached to the messageType.
 */
(function() {
var cache = {};
DepthJS.browser.addContentScriptListener = function(messageType, callback) {
  if (cache[messageType] && cache[messageType][0] == callback) {
    console.log("Ignoring existing port subscription on " + messageType);
    return;
  }
  var port = chrome.extension.connect({name: messageType});
  port.onMessage.addListener(callback);
  cache[messageType] = [callback, port];
  /*
  safari.self.addEventListener("message", function(e) {
    if (e.name == messageType) callback(e.message);
  }, false);
  */
};

DepthJS.browser.readdContentScriptListeners = function() {
  console.log("DepthJS: Unimplemented for chrome: readdContentScriptListeners");
  /*
  _.each(stuff, function(callback, messageType) {
    console.log("Reading event listener for " + messageType);
    safari.self.addEventListener("message", function(e) {
      if (e.name == messageType) callback(e.message);
    }, false);
  });
  */
};
})();

DepthJS.browser.sendMessageToPopup = function(msg) {
  chrome.extension.sendRequest({action: msg});
};

DepthJS.browser.sendMessageToActiveTab = function(message) {
  chrome.tabs.getSelected(null, function(tab) {
    var tabId = tab.id;
    var tabPorts = DepthJS.portsByTabId[tabId];
    if (tabPorts == null) {
      DepthJS.logSortaVerbose("Could not find ports for tabId " + tabId);
      return;
    }
    var eventPort = tabPorts.event;
    if (eventPort == null) {
      DepthJS.logSortaVerbose("Could not find event port for tabId " + tabId);
      return;
    }

    DepthJS.logSortaVerbose("sending_" + message.type, message);
    eventPort.postMessage(message);
  });
};

DepthJS.browser.sendMessageToBackground = function(messageType, data) {
  chrome.extension.sendRequest({action: messageType, data: data});
};

} // if (DepthJS)
