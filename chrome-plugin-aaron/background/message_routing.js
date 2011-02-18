console.log("Initing background.html message handling");
DepthJS.background.handleMessage = function(action, data, reply) {
  var handlers = {
    connect: DepthJS.backend.connect,

    disconnect: DepthJS.backend.disconnect,

    getConnectState: function() {
      if (DepthJS.backend.connecting) {
        reply("connectState", {state: "connecting", mode: DepthJS.registerMode});
      } else {
        var state = DepthJS.backend.connected ? "connected" : "disconnected";
        reply("connectState", {state: state, mode: DepthJS.registerMode});
      }
    },

    depthoseTest: DepthJS.test.depthoseTest,

    pannerTest: DepthJS.test.pannerTest,

    selectorBoxTest: DepthJS.test.selectorBoxTest,

    getThumbnails: function() {
      DepthJS.tabs.populateActiveWindowCache(function(tabs) {
        tabs = _.map(tabs, DepthJS.tabs.sanitizeTabObject);
        if (DepthJS.verbose) console.log(["sending thumbnails", tabs]);
        reply("thumbnails", {tabs: tabs});
      });
    },

    selectTab: function() {
      var tabId = data.tabId;
      var tab = _.select(DepthJS.tabs.activeWindowTabCache, function(t) { return t.tabId == tabId; });
      if (tab.length == 0) {
        console.log("Couldn't find tabId " + tabId);
        return;
      }
      tab = tab[0];
      tab.activate();
    },

    depthoseMode: function() {
      DepthJS.registerMode = "depthose";
      DepthJS.test.sendTestEvent({type: "DepthoseMode", data:{}});
    },

    pannerMode: function() {
      DepthJS.registerMode = "panner";
      DepthJS.test.sendTestEvent({type: "PannerMode", data:{}});
    },

    selectorBoxMode: function() {
      DepthJS.registerMode = "selectorBox";
      DepthJS.test.sendTestEvent({type: "SelectorBoxMode", data:{}});
    }
  };
  handlers[action](data);
};
