// Do the initialization
$(function() {
  if (window.top === window) {
    // The parent frame is the top-level frame, not an iframe.
    console.log(["Initing DepthJS", DepthJS]);
    DepthJS.selectorBox.init();
    DepthJS.eventLink.initPort();
    DepthJS.depthose.init();

    // Let us know its running
    console.log("Finished initing, sticking in logo");
    $("<img src='https://github.com/doug/depthjs/raw/master/chrome-extension/logo_128x128.png'>").css({
      position: "fixed",
      width: "32px",
      height: "32px",
      bottom: "20px",
      left: "20px"
    }).appendTo("body");
    console.log($("img"));
  }
});
