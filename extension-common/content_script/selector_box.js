if (window.top === window) {
// SELECTOR BOX ------------------------------------------------------------------------------------
console.log("DepthJS: Loading SelectorBox");

DepthJS.selectorBox.firstMove = null;

DepthJS.selectorBox.init = function() {
  console.log("Initing selector box");
  var $box = $("<div id='DepthJS_selectorBox'></div>");
  $box.appendTo("body").hide();
  DepthJS.selectorBox.$box = $box;
};

DepthJS.selectorBox.show = function() {
  DepthJS.selectorBox.$box.show();
  DepthJS.selectorBox.firstMove = null;
};

DepthJS.selectorBox.hide = function() {
  DepthJS.selectorBox.$box.hide();
  DepthJS.selectorBox.firstMove = null;
};

DepthJS.selectorBox.move = function(x, y) {
  x = (x - 50) / 50.0;
  y = (y - 50) / 50.0;

  // Expode out for a smaller range in Kinect-hand space
  x *= 5;
  y *= 5;
  x = Math.min(1, Math.max(-1, x));
  y = Math.min(1, Math.max(-1, y));

  var hwidth = $(window).width() * 0.5;
  var hheight = $(window).height() * 0.5;
  x =  hwidth*x + hwidth;
  y =  hheight*y + hheight;

   // Constrain to window
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  var $box = DepthJS.selectorBox.$box;
  x = Math.min(x, $(window).width() - $box.width());
  y = Math.min(y, $(window).height() - $box.height());
  //console.log("move selector box to " + x + ", " + y);
  if (x != $box.css("left") || y != $box.css("top")) {
    $box.css({left: x, top: y});
  }
};

DepthJS.selectorBox.activate = function() {
  if (DepthJS.verbose) console.log("DepthJS: Activating underneath selectorBox");
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

  if (DepthJS.verbose) console.log("Got " + $intersectingLinks.length + " links");
  if (DepthJS.verbose) console.log($intersectingLinks);
  if ($intersectingLinks.length > 0) {
    DepthJS.selectorBoxPopup.$links = $intersectingLinks;
    DepthJS.selectorBoxPopup.activate();
  }
};


// SELECTOR BOX POPUP ------------------------------------------------------------------------------
DepthJS.selectorBoxPopup.activate = function(){
  if (DepthJS.verbose) console.log("Activating selectorbox popup");
  var $links = DepthJS.selectorBoxPopup.$links;

  if ($links.length == 1){
    DepthJS.selectorBox.$box.fadeOut(200);

    var evt = document.createEvent("MouseEvents");
    evt.initMouseEvent("click", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
    $links[0].dispatchEvent(evt);
    return;
  }


  DepthJS.state = "selectorBoxPopup";
  var $box = DepthJS.selectorBox.$box;
  var top = $box.position().top;
  var left = $box.position().left;
  var position = "top:" + top + "px; left:" + left + "px";
  $("body").append("<div id='DepthJS_selectorBoxPopup' style='" + position + "'></div>");

  var popupContent = "";
  var popupItemIndex = 0;

  for(var i = 0; i < $links.length; i++){
    var linkText = $($links[i]).text();

    if (linkText.length <= 0) continue;

    if (linkText.indexOf("<img") > 0) {
      popupContent += "<div id='DepthJS_popupItem" + popupItemIndex +
      "' class='DepthJS_selectorBoxPopupItem'>" +
      linkText.substring(0,70) + "</div>";
    } else {
      popupContent += "<div id='DepthJS_popupItem" + popupItemIndex +
      "' class='DepthJS_selectorBoxPopupItem'>" +
      $($links[i]).html() + "</div>";
    }
    popupItemIndex += 1;
  }
  DepthJS.selectorBox.$box.hide();
  var $popup = $("#DepthJS_selectorBoxPopup");
  $popup.html(popupContent);
  if (top + $popup.height() > $(window).height()) {
    $popup.css("top", $(window).height() - $popup.height() - 40);
  }
  if (left + $popup.width() > $(window).width()) {
    $popup.css("left", $(window).width() - $popup.width() - 40);
  }
};

DepthJS.selectorBoxPopup.move = function(x, y) {
  if (DepthJS.verbose) console.log("move selector box popup (" + x + ", " + y + ")");
  y = (y - 50) / 50.0; // -1 to 1
  // Expode out for a smaller range in Kinect-hand space
//  y *= 4.0;
  console.log("pre clamp" + y);
  y = Math.min(1.0, Math.max(-1.0, y)); // clamp to -1 to 1
  y = (y + 1.0) / 2.0; // shift to 0 to 1
  console.log(y);

  var $links = DepthJS.selectorBoxPopup.$links;
  var popupHeight = $("#DepthJS_selectorBoxPopup").height();
  var closestLinkIndex = Math.round(y * $links.length-1);
  if (DepthJS.verbose) console.log("Closest link is " + closestLinkIndex);

  var $lastHighlightedLink = DepthJS.selectorBoxPopup.$lastHighlightedLink;
  if ($lastHighlightedLink != null){
    $lastHighlightedLink.removeClass("DepthJS_selectorBoxPopupItemHighlight");
  }
  var $closestLink = $("#DepthJS_popupItem" + closestLinkIndex);
  $closestLink.addClass("DepthJS_selectorBoxPopupItemHighlight");

  DepthJS.selectorBoxPopup.lastHighlightedLinkIndex = closestLinkIndex;
  DepthJS.selectorBoxPopup.$lastHighlightedLink = $closestLink;
};

DepthJS.selectorBoxPopup.openHighlightedLink = function() {
  var $links = DepthJS.selectorBoxPopup.$links;
  if ($links == null) {
    console.log("No links stored / not opening highlighted link");
    return;
  }
  var lastHighlightedLinkIndex = DepthJS.selectorBoxPopup.lastHighlightedLinkIndex;

  if (lastHighlightedLinkIndex < 0 || lastHighlightedLinkIndex > $links.length) return;
  var $linkToOpen = $links.eq(lastHighlightedLinkIndex);

  DepthJS.selectorBoxPopup.hide();

  var evt = document.createEvent("MouseEvents");
  evt.initMouseEvent("click", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
  $linkToOpen[0].dispatchEvent(evt);
};

DepthJS.selectorBoxPopup.hide = function(){
  DepthJS.selectorBox.$links = null;
  DepthJS.selectorBoxPopup.lastHighlightedLinkIndex = undefined;
  $("#DepthJS_selectorBoxPopup").fadeOut(300, function(){
    $("#DepthJS_selectorBoxPopup").remove();
  });
};
}
