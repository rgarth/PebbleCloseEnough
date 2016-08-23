Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  var units = localStorage.getItem('units');
  if (! units) units = "us";
  var bg_color = localStorage.getItem('bg_color');
  if (! bg_color) bg_color = "000000";
  var fg_color = localStorage.getItem('fg_color');
  if (! fg_color) fg_color = "ffffff";
  var invert = localStorage.getItem('invert');
  if (! invert) invert = false;
  console.log(units + " " + bg_color + " " + fg_color + " invert:" + invert);
  
  var color = localStorage.getItem('color');
  if (! color) color = 0;
  console.log ("Color Watch? " + color);
  // Show config page
  console.log('Configuration window opened.');
  Pebble.openURL('http://rgarth.github.io/PebbleCloseEnough/configuration-d.html' +
    '?units=' + units +
    '&color=' + color +
    '&fg=' + fg_color + 
    '&bg=' + bg_color +
    '&invert=' + invert);
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    var configuration = JSON.parse(decodeURIComponent(e.response));
    
    console.log ('BG Color: ' + configuration.bg_color);
    console.log ('FG Color: ' + configuration.fg_color);
    console.log('Units: ' + configuration.units);
    console.log('Invert: ' + configuration.invert);
    var invert = 0;
    if ( configuration.invert ) {
      invert = 1;
    }
    var dictionary = {
      "KEY_UNITS": configuration.units,
      "KEY_BACKGROUND": parseInt(configuration.bg_color, 16),
      "KEY_FOREGROUND": parseInt(configuration.fg_color, 16),
      "KEY_INVERT": invert
    };
    localStorage.setItem('units', configuration.units);
    localStorage.setItem('bg_color', configuration.bg_color);
    localStorage.setItem('fg_color', configuration.fg_color);
    localStorage.setItem('invert', configuration.invert);
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