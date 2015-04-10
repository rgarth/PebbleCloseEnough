Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  var units = localStorage.getItem('units');
  if (! units) units = "us";
  // Show config page
  console.log('Configuration window opened.');
  Pebble.openURL('http://rgarth.github.io/PebbleCloseEnough/configuration.html?units=' + units);
  //Pebble.openURL('http://localhost:8000/configuration.html?units=' + units);
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    var configuration = JSON.parse(decodeURIComponent(e.response));
    var dictionary = {
        "KEY_UNITS": configuration.units
    };
    localStorage.setItem('units', configuration.units);
    console.log('Configuration window returned: ' + configuration.units);
    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log("Configuration sent to Pebble successfully!");
      },
      function(e) {
        console.log("Error sending configuration info to Pebble!");
      }
    );
  }
);