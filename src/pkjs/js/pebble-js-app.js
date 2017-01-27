
Pebble.addEventListener("ready", function(e) {
  console.log("Starting ...");
  sendNoteContents();
});

Pebble.addEventListener("appmessage", function(e) {
  sendNoteContents();
});

function sendNoteContents() {
  var note = localStorage.getItem('note') || '...set this using settings in Pebble App...';
  Pebble.sendAppMessage({ "note": note });
}

Pebble.addEventListener('showConfiguration', function(e) {
  var note = localStorage.getItem('note') || '';
	var uri = 'http://www.themapman.com/pebblewatch/myinfo_slate.html?' +
				'note=' + encodeURIComponent(note);
	console.log('showing configuration at uri: ' + uri);
	Pebble.openURL(uri);
});

Pebble.addEventListener('webviewclosed', function(e) {
	console.log('configuration closed');
	if (e.response) {
		var options = JSON.parse(decodeURIComponent(e.response));
		console.log('options received from configuration: ' + JSON.stringify(options));
		var note = options['note'];
		localStorage.setItem('note', note);
    sendNoteContents();
	} else {
		console.log('no options received');
	}
});
