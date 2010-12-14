if (DepthJS) {
console.log("DepthJS Loading Safari shit");
DepthJS.initBrowserBackground = function() {
  console.log("DepthJS: Initing Safari background");
  DepthJS.toolbar.init = function() {
    
    var modes = ["PannerMode", "SelectorBoxMode", "DepthoseMode"];
    var index = 0;
    
    safari.application.addEventListener("command", function(msgEvent) {
      if (msgEvent.command == "toolbarClick") {
        var mode = index % 3;
        console.log(["Toolbar clicked, entering mode", mode]);
        DepthJS.test.runTestSequence([{type:modes[mode], data:{}}]);
        index++;

        // msgEvent.target.image = safari.extension.baseURI + "alpha_background.gif";
        // event.target.browserWindow
        // DepthJS.test.gestureTest();
      }
    });
  };
  
  DepthJS.tabs.populateActiveWindowCache = function(callback) {
    var tabs = safari.application.activeBrowserWindow.tabs;
    DepthJS.tabs.activeWindowTabCache = _.map(tabs, function(tab) {
      tab.tabId = _.uniqueId();
      tab.dataUrl = tab.visibleContentsAsDataURL();
      return tab;
    });
    callback(DepthJS.tabs.activeWindowTabCache);
  };
  
  DepthJS.toolbar.init();
};

/**
 * Typically used for background.html. When a message is passed in, it passes to the
 * callback two parameters: action & data.
 */
  
DepthJS.browser.addBackgroundListener = function(callback) {
  safari.application.addEventListener("message", function(e) {
    var action = e.name;
    var data = e.message;
    var reply = function(messageType, data) {
      e.target.page.dispatchMessage(messageType, data);
    };
    callback(action, data, reply);
  });
};

/**
 * Typically used for content scripts. When a message is passed in, it passes to the
 * callback the data attached to the messageType.
 */
(function() {
var stuff = {};
DepthJS.browser.addContentScriptListener = function(messageType, callback) {
  stuff[messageType] = callback;
  safari.self.addEventListener("message", function(e) {
    if (e.name == messageType) callback(e.message);
  }, false);
};

DepthJS.browser.readdContentScriptListeners = function() {
  _.each(stuff, function(callback, messageType) {
    console.log("Readding event listener for " + messageType);
    safari.self.addEventListener("message", function(e) {
      // console.log(e);
      if (e.name == messageType) callback(e.message);
    }, false);
  });
};
})();

DepthJS.browser.sendMessageToPopup = function(msg) {
  console.log(["Sent popup", msg]);
};

DepthJS.browser.sendMessageToActiveTab = function(message) {
  var activeWindow = safari.application.activeBrowserWindow;
  if (activeWindow != null) {
    var activeTab = activeWindow.activeTab;
    if (activeTab != null && activeTab.page != null) {
      DepthJS.logSortaVerbose("sending_" + message.type, message);
      activeTab.page.dispatchMessage("event", message);
    }
  }
};

DepthJS.browser.sendMessageToBackground = function(messageType, data) {
  safari.self.tab.dispatchMessage(messageType, data);
};

DepthJS.browser.sendMessageToPopup = function(msg) {
  console.log(["Sending message to popup: ", msg]);
  // safari.self.tab.dispatchMessage("DepthJS_popup", msg);
};
}