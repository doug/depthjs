// SELECTOR BOX ------------------------------------------------------------------------------------
console.log("DepthJS: Loading SelectorBox");

DepthJS.selectorBox.firstMove = null;

DepthJS.selectorBox.init = function() {
  console.log("Initing selector box");
  var $box = $("<div id='DepthJS_selectorBox'></div>");
  $box.appendTo("body").hide();
  DepthJS.selectorBox.$box = $box;
  console.log(DepthJS.selectorBox.$box);
  console.log($("body"));
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
  if (DepthJS.selectorBox.firstMove == null) {
    DepthJS.selectorBox.firstMove = [x, y];
    console.log("first move, x=" + x + " y=" + y);
    return;
  }
  
  // x = x - DepthJS.selectorBox.firstMove[0];
  // y = y - DepthJS.selectorBox.firstMove[1];

  x = (x - 50) / 15.0;
  y = (y - 50) / 15.0;
  
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
  console.log("move selector box to " + x + ", " + y);
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
    var evt = document.createEvent("MouseEvents");
    evt.initMouseEvent("click", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
    $links[0].dispatchEvent(evt);
    return;
  }


  DepthJS.state = "selectorBoxPopup";
  var $box = DepthJS.selectorBox.$box;
  var position = "top:" + $box.position().top + "px; left:" + $box.position().left + "px";
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
  $("#DepthJS_selectorBoxPopup").html(popupContent);
};

DepthJS.selectorBoxPopup.move = function(x, y) {
  if (DepthJS.verbose) console.log("move selector box popup (" + x + ", " + y + ")");
  if (y == 0) y = 1;

  var $links = DepthJS.selectorBoxPopup.$links;
  var popupHeight = $("#DepthJS_selectorBoxPopup").height();
  y = (y * popupHeight / 100) / (popupHeight / $links.length);
  var closestLinkIndex = Math.min($links.length-1, Math.round(y));
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

DepthJS.selectorBoxPopup.openHighlightedLink = function(){
  var $links = DepthJS.selectorBoxPopup.$links;
  var lastHighlightedLinkIndex = DepthJS.selectorBoxPopup.lastHighlightedLinkIndex;

  if (lastHighlightedLinkIndex < 0 || lastHighlightedLinkIndex > $links.length) return;
  var $linkToOpen = $links.eq(lastHighlightedLinkIndex);

  var evt = document.createEvent("MouseEvents");
  evt.initMouseEvent("click", true, true, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
  $linkToOpen[0].dispatchEvent(evt);
};

DepthJS.selectorBoxPopup.hide = function(){
  $("#DepthJS_selectorBoxPopup").fadeOut(300, function(){
    $("#DepthJS_selectorBoxPopup").remove();
  });
};