Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  var units = localStorage.getItem('units');
  if (! units) units = "us";
  var bg_color= parseInt(localStorage.getItem('bg_color'), 16);
  if (! bg_color) bg_color = parseInt("000000", 16);
  var fg_color= parseInt(localStorage.getItem('fg_color'), 16);
  if (! fg_color) fg_color = parseInt("ffffff", 16);
  
  var color = localStorage.getItem('color');
  console.log ("Color Watch? " + color);
  // Show config page
  console.log('Configuration window opened.');
  Pebble.openURL('http://rgarth.github.io/PebbleCloseEnough/configuration-c.html' +
    '?units=' + units +
    '&color=' + color +
    '&fg=' + fg_color + 
    '&bg=' + bg_color);
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    var configuration = JSON.parse(decodeURIComponent(e.response));
    
    console.log ("BG Color: " + configuration.bg_color);
    console.log ("FG Color: " + configuration.fg_color);
    console.log('Units: ' + configuration.units);
    var dictionary = {
      "KEY_UNITS": configuration.units,
      "KEY_BACKGROUND": parseInt(configuration.bg_color, 16),
      "KEY_FOREGROUND": parseInt(configuration.fg_color, 16)
    };
    localStorage.setItem('units', configuration.units);
    localStorage.setItem('bg_color', parseInt(configuration.bg_color, 16));
    localStorage.setItem('fg_color', parseInt(configuration.fg_color, 16));
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