DepthJS.tabs.activeWindowTabCache = null;

DepthJS.tabs.populateActiveWindowCache = function(callback) {
  DepthJS.browser.populateActiveWindowCache(callback);
};

DepthJS.tabs.sanitizeTabObject = function(tab) {
  if (tab == null) {
    console.log("Cannot sanitize null tab");
    return undefined;
  }
  return {
    tabId: tab.tabId,
    dataUrl: tab.dataUrl,
    title: tab.title,
    url: tab.url
  };
};