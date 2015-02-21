var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var url = "http://wake-treehacks.herokuapp.com/asleep";

  // Send request to Wake backend
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with sleep status info
      var json = JSON.parse(responseText);

      var asleep = json.SLEEP;
      console.log("Asleep value is " + asleep);
      
      // Assemble dictionary using our key
      var dictionary = {
        "KEY_ASLEEP": asleep
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Sleep status info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending sleep status info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getSleepStatus() {
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

    // Get the initial weather
    getSleepStatus();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    
    if (e.payload.KEY_ASLEEP == 5) {
      console.log("Main requested to know SLEEP status from server");
      getSleepStatus();
    } else if (e.payload.KEY_BUTTON_PRESSED == 5) {
      console.log("Main wants to inform the server that the user pressed the button");
      var url = "http://wake-treehacks.herokuapp.com/pebble_button";
      xhrRequest(url, "GET", function() {});
    }
    console.log("JSON stringify e", JSON.stringify(e));
    
  }                     
);
