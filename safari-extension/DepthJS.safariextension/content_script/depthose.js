// DEPTHOSE ---------------------------------------------------------------------------------------
console.log("DepthJS: Loading Depthose");
DepthJS.depthose.$div = null;
DepthJS.depthose.thumbnailCache = null;

DepthJS.depthose.init = function() {
  console.log("Initting Depthose");
  DepthJS.browser.addContentScriptListener("thumbnails", function(data) {
    DepthJS.depthose.recieveThumbnails(data.tabs);
  });
  DepthJS.depthose.zflowInit();
};

DepthJS.depthose.start = function() {
  if (DepthJS.verbose) console.log("DepthJS: Requesting thumbnails");
  DepthJS.browser.sendMessageToBackground("getThumbnails");
};

DepthJS.depthose.recieveThumbnails = function(thumbnails) {
  if (DepthJS.verbose) console.log(["DepthJS: Got back thumbnails", thumbnails]);
  DepthJS.depthose.thumbnailCache = thumbnails;
  if (DepthJS.depthose.thumbnailCache != null) DepthJS.depthose.show();
};

DepthJS.depthose.show = function() {
  if (DepthJS.depthose.thumbnailCache == null) {
    DepthJS.depthose.start();
    return;
  }
  var thumbnailCache = DepthJS.depthose.thumbnailCache;

  DepthJS.selectorBox.hide();
  //if (DepthJS.verbose) console.log(["DepthJS: Starting depthose with", DepthJS.depthose.thumbnailCache]);
  $("#DepthJS_depthose").remove();
  DepthJS.depthose.$div = $("<div id='DepthJS_depthose'></div>");
  var $div = DepthJS.depthose.$div;
  $div.css({"position": "fixed",
            "width": "100%",
            "height": "100%",
            "background-color": "#333",
            "z-index": "10000",
            "top": "0",
            "left": "0"})
      .addClass("zflow")
      .appendTo("body");

  $div.append("<div class='centering'><div id='DepthJS_tray' class='tray'></div></div>");

  _.each(thumbnailCache, function(tabObj, tabId) {
    tabObj.selectCallback = function() {
      if (DepthJS.verbose) console.log("selecting tab id " + tabId);
      DepthJS.eventHandlers.onUnregister();
      DepthJS.browser.sendMessageToBackground("selectTab", tabObj);
    };
  });

  if (!_.isEmpty(thumbnailCache)) {
    if (DepthJS.verbose) console.log("starting zflow");
    DepthJS.depthose.zflow(_.values(thumbnailCache), "#DepthJS_tray");
    var e = document.createEvent("Event");
    e.initEvent("depthstart");
    e.pageX = $(window).width()/2;
    e.pageY = $(window).height()/2;
    document.getElementById("DepthJS_tray").dispatchEvent(e);
  } else {
    if (DepthJS.verbose) console.log("Not showing Depthose--no windows to show");
  }
};

DepthJS.depthose.hide = function() {
  if (DepthJS.depthose.$div == null) return;
  if (DepthJS.verbose) console.log("DepthJS: Exiting Depthose");
  var e = document.createEvent("Event");
  e.initEvent("depthend");
  document.getElementById("DepthJS_tray").dispatchEvent(e);
  DepthJS.depthose.$div.remove();
  DepthJS.depthose.$div = null;
  DepthJS.depthose.thumbnailCache = null;
};

DepthJS.depthose.move = function(x, y) {
  if (DepthJS.depthose.thumbnailCache == null) {
    if (DepthJS.verbose) console.log("DepthJS: Haven't loaded Depthose yet, ignoring move event");
    return;
  }

  if (DepthJS.verbose == STUPID_VERBOSE) console.log("DepthJS: Move depthose");
  var e = document.createEvent("Event");
  e.initEvent("depthmove");
  e.pageX = x * $(window).width() / 100;
  e.pageY = y * $(window).height() / 100;
  document.getElementById("DepthJS_tray").dispatchEvent(e);
};

DepthJS.depthose.select = function() {
  if (DepthJS.depthose.thumbnailCache == null) {
    if (DepthJS.verbose) console.log("DepthJS: Haven't loaded Depthose yet, ignoring move event");
    return;
  }

  if (DepthJS.verbose) console.log("DepthJS: Selecting in Depthose");
  var e = document.createEvent("Event");
  e.initEvent("depthselect");
  document.getElementById("DepthJS_tray").dispatchEvent(e);
};