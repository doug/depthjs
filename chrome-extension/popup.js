  function connected() {
    $("#disconnect").removeClass("disabled")
                    .addClass("enabled");
    $("#connect").removeClass("enabled")
                 .addClass("disabled");
   }

  function disconnected() {
    $("#connect").removeClass("disabled")
                 .addClass("enabled");
    $("#disconnect").removeClass("enabled")
                    .addClass("disabled");
  }

  function connecting() {
    $("li.server").removeClass("enabled")
                  .addClass("disabled");
  }

  function updateMessage(msg) {
    var state = msg.action || msg.state;
    if (state == "connected") {
      connected();
    } else if (state == "disconnected") {
      disconnected();
    } else if (state == "connecting") {
      connecting();
    }

    if (msg.mode != null) {
      $(".mode").removeClass("disabled")
                .addClass("enabled");
      $("#" + msg.mode + "Mode").removeClass("enabled")
                                .addClass("disabled");
    }
  }

  jQuery(function() {
    $(".enabled").live("click", function() {
      chrome.extension.sendMessage({action: $(this).attr("id")});
      window.close();
    });

    chrome.extension.sendMessage({action: "getConnectState"}, updateMessage);
  });

  chrome.extension.onRequest.addListener(updateMessage);
