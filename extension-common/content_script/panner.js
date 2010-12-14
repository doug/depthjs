// PANNER ------------------------------------------------------------------------------------------
console.log("DepthJS: Loading Panner");

DepthJS.panner.initTransform = null;
DepthJS.panner.initTransition = null;
DepthJS.panner.firstMove = null;

DepthJS.panner.show = function() {
  if (DepthJS.panner.initTransform == null) {
    DepthJS.panner.initTransform = $("body").css("-webkit-transform");
  }
  if (DepthJS.panner.initTransition == null) {
    DepthJS.panner.initTransition = $("body").css("-webkit-transition-duration");
  }
  var centerPoint = $(window).height()/2 + $(window).scrollTop();
  $("body").css({"-webkit-transform":"scale(1.55) translate(0px,0px)",
                 "-webkit-transition-duration":"1s",
                 "-webkit-transform-origin":"50% " + centerPoint + "px"});
  DepthJS.panner.firstMove = null;
};

DepthJS.panner.hide = function() {
  if ($("body").css("-webkit-transform") == "none") return;
  $("body").css({"-webkit-transform":"scale(1)","-webkit-transition-duration":"1s"});
  // Reset it to whatever the page had before
  setTimeout(function() {
    $("body").css({"-webkit-transform": DepthJS.panner.initTransform,
                   "-webkit-transition-duration": DepthJS.panner.initTransition});
  }, 1000);
  DepthJS.panner.firstMove = null;
};

DepthJS.panner.move = function(x, y) {
  if (DepthJS.panner.firstMove == null) {
    DepthJS.panner.firstMove = [x, y];
    return;
  }
  var centerPoint = $(window).height()/2 + $(window).scrollTop();
  // use firstMove here
  x = (x-50) * $(document).width() / 100;
  y = (y-50) * $(document).height() / 100;
  $("body").css({"-webkit-transform":"scale(1.55) translate(" + x + "px, " + y + "px)",
                 "-webkit-transition-duration":".25s",
                 "-webkit-transform-origin":"50% " + centerPoint + "px"});
};
