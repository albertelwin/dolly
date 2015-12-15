
var LibraryWebAudio = {
	$WebAudio: {},

	web_audio_init: function(channels, samples, samples_per_second, callback, user_ptr) {
		if(window.AudioContext) {
			WebAudio.context = new window.AudioContext();
		}
		else if(window.webkitAudioContext) {
			WebAudio.context = new window.webkitAudioContext();
		}

		if(!WebAudio.context) {
			console.log("WARNING: Web Audio API not supported!");

			WebAudio.samples_per_second = 0;
		}
		else {
			WebAudio.samples_per_second = WebAudio.context.sampleRate;
			WebAudio.bytes_per_sample = 2;
			WebAudio.channels = channels;
			WebAudio.samples = samples;

			WebAudio.callback = callback;
			WebAudio.user_ptr = user_ptr;

			WebAudio.buffer_size = WebAudio.samples * WebAudio.channels * WebAudio.bytes_per_sample;
			WebAudio.buffer_ptr = _malloc(WebAudio.buffer_size);
			WebAudio.buffer_duration = WebAudio.samples / WebAudio.samples_per_second;
			WebAudio.next_play_time = 0.0;

			WebAudio.paused = false;
		}

		{{{ makeSetValue('samples_per_second', '0', 'WebAudio.samples_per_second', 'i32') }}};

		return !!WebAudio.context;
	},

	web_audio_suspend: function() {
		if(WebAudio.context) {
			WebAudio.context.suspend();
			WebAudio.paused = true;			
		}
	},

	web_audio_resume: function() {
		if(WebAudio.context) {
			WebAudio.context.resume();
			WebAudio.paused = false;			
		}
	},

	web_audio_close: function() {
		if(WebAudio.context) {
			WebAudio.context.close();
			WebAudio.paused = true;
		}
	},

	web_audio_request_samples: function() {
		if(WebAudio.context) {
			var time_until_next_play = WebAudio.next_play_time - WebAudio.context.currentTime;
			if(time_until_next_play <= WebAudio.buffer_duration && !WebAudio.paused) {
				Runtime.dynCall('viii', WebAudio.callback, [ WebAudio.user_ptr, WebAudio.buffer_ptr, WebAudio.buffer_size ]);

				var buffer = WebAudio.context.createBuffer(WebAudio.channels, WebAudio.samples, WebAudio.samples_per_second);

				for(var i = 0; i < WebAudio.channels; i++) {
					var channel_data = buffer.getChannelData(i);
					for(var ii = 0; ii < WebAudio.samples; ii++) {
						var sample = {{{ makeGetValue('WebAudio.buffer_ptr', '(ii * WebAudio.channels + i) * 2', 'i16', 0, 0) }}};
						channel_data[ii] = sample / 0x8000;
					}
				}

				var source = WebAudio.context.createBufferSource();
				source.buffer = buffer;
				source.connect(WebAudio.context.destination);

				var current_time = WebAudio.context.currentTime;
				var play_time = WebAudio.next_play_time;
				if(play_time < current_time && play_time > 0.0) {
					console.log("WARNING: Audio starved by " + (current_time - play_time) + " seconds!!");
					play_time = current_time;
				}

				source.start ? source.start(play_time) : source.noteOn(play_time);
				WebAudio.next_play_time = play_time + WebAudio.buffer_duration;
			}
		}
	},
};

autoAddDeps(LibraryWebAudio, '$WebAudio');
mergeInto(LibraryManager.library, LibraryWebAudio);
