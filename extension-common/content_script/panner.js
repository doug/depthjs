// PANNER ------------------------------------------------------------------------------------------
console.log("DepthJS: Loading Panner");

DepthJS.panner.initTransform = null;
DepthJS.panner.initTransition = null;
DepthJS.panner.firstMove = null;
DepthJS.panner.midTransition = false;

DepthJS.panner.show = function() {
  if (DepthJS.panner.initTransform == null) {
    DepthJS.panner.initTransform = $("body").css("-webkit-transform");
  }
  if (DepthJS.panner.initTransition == null) {
    DepthJS.panner.initTransition = $("body").css("-webkit-transition-duration");
  }
  var centerPoint = $(window).height()/2 + $(window).scrollTop();
  // $("body").css({"-webkit-transform":"scale(1.55) translate(0px,0px)",
  //                "-webkit-transition-duration":"1s",
  //                "-webkit-transform-origin":"50% " + centerPoint + "px"});
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

DepthJS.panner.move = function(x, y, z) {
  if (DepthJS.panner.firstMove == null) {
    DepthJS.panner.firstMove = [x, y, z];
    console.log("first move, x=" + x + " y=" + y);
    return;
  }
  //if (DepthJS.panner.midTransition) return;
  DepthJS.panner.midTransition = true;
  setTimeout(function() {
    DepthJS.panner.midTransition = false;
  }, 150);
  
  var centerPoint = $(window).height()/2 + $(window).scrollTop();
  // use firstMove here
  
  x = DepthJS.panner.firstMove[0] - x;
  y = DepthJS.panner.firstMove[1] - y;
  var scale = 1.3 - ((z - 50) / 50);
  
  // Make the whole bounding box (which is -50, 50) really out of 10
  
  console.log("rel x = " + x + ", y = " + y + ", scale=" + scale);
  
  x = x * $(document).width() / 75;
  y = y * $(document).height() / 75;
  $("body").css({"-webkit-transform":"scale(" + scale + ") translate(" + x + "px, " + y + "px)",
                 "-webkit-transition-duration":".25s",
                 "-webkit-transform-style":"preserve-3d",
                 "-webkit-transform-origin":"50% " + centerPoint + "px"});
};
