Pebble.addEventListener("showConfiguration",
  function(e) {
    Pebble.openURL("http://namedfork.net/pbl/roman_digital_cfg_2.html");
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    var config = e.response;
    Pebble.sendAppMessage( { "updateConfig": config } );
  }
);
