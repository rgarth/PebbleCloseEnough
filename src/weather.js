
// Location success can only take a single variable
// It was just simply to declare a global
var units = "us";
// OpenWeatherMap API Key
var key = "";

function locationSuccess(pos) {
  var temperature;
  var conditions;

  if (units == "us" || units == "imperial" ) {
    units = "imperial";
  } else {
    units = "metric";
  }
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + pos.coords.latitude +
            '&lon=' + pos.coords.longitude + 
            '&units=' + units +
            '&appid=' + key;
  console.log(url);
  // Send request to OpenWeatherMap
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var json = JSON.parse(this.responseText);
    temperature = Math.round(json.main.temp);
    conditions = json.weather[0].main;
    console.log (temperature + " " + conditions);
    var dictionary = {
      'KEY_TEMPERATURE': temperature,
      'KEY_CONDITIONS': conditions
    };
    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
    function(e) {
      console.log("Weather info sent to Pebble successfully!");
    },
    function(e) {
      console.log("Error sending weather info to Pebble!");
    }
    );
  };
  xhr.open('GET', url);
  xhr.send();
}

function locationError(err) {
  console.log("Error requesting location!");
}

// Get Location lat+lon
function getLocation() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function(e) {
    console.log("PebbleKit JS ready!");
    var dictionary = {
        "KEY_JSREADY": 1
    };

    // Send to Pebble, so we can load units variable and send it back
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log("Ready notice sent to phone!");
      },
      function(e) {
        console.log("Error ready notice to Pebble!");
      }
    );
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received");
    units = e.payload.KEY_UNITS;
    if (typeof units == 'undefined') units = "imperial";
    var color = e.payload.KEY_COLOR;
    localStorage.setItem('color', color);
 
    console.log("Units = " + units);
    getLocation();
  }
);