// onSuccess Callback
// This method accepts a Position object, which contains the
// current GPS coordinates
//
var onSuccess = function(imageData) {
    console.log('data:image/jpeg;base64,' + imageData);
};

// onError Callback receives a PositionError object
//
function onError(error) {
    console.log('code: '    + error.code    + '\n' +
          'message: ' + error.message + '\n');
}

navigator.camera.getPicture(onSuccess, onError, { destinationType: Camera.DestinationType.DATA_URL });
