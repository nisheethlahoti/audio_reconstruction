#include <fstream>
#include <iostream>

#include "alsa.h"

using namespace std;

atomic<int> buf_size(0);
snd_pcm_t *handle;

ofstream binout("out.bin");
void thread_fn(chrono::time_point<chrono::steady_clock> time) {
	while (true) {
		this_thread::sleep_until(time += duration);
		if (buf_size > 0) {
			log(playing_log());
			buf_size--;
		} else {
			log(reader_waiting_log());
		}
	}
}

void write_samples(void const *samples, size_t len) {
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, samples, len);
	binout.write(static_cast<char const *>(samples),
	             len * byte_depth * num_channels);

	if (frames < 0) {
		cerr << "snd_pcm_writei failed: " << snd_strerror(frames) << endl;
	} else if (frames < len) {
		cerr << "Short write (expected " << len << ", wrote " << frames << ')'
		     << endl;
	}

	if (frames < 0)
		cerr << (snd_pcm_recover(handle, frames, 0) ? "Recovered"
		                                            : "Could not recover")
		     << endl;
}

constexpr snd_pcm_format_t pcm_format() {
	switch (byte_depth) {
		case 1:
			return SND_PCM_FORMAT_S8;
		case 2:
			return SND_PCM_FORMAT_S16_LE;
		case 3:
			return SND_PCM_FORMAT_S24_LE;
		case 4:
			return SND_PCM_FORMAT_S32_LE;
		default:
			return SND_PCM_FORMAT_UNKNOWN;
	}
}

void init_pcm() {
	int err;
	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		cerr << "Playback open error: " << snd_strerror(err) << endl;
		exit(1);
	}

	if ((err = snd_pcm_set_params(
	         handle,        // handle
	         pcm_format(),  // Format of individual channel samples
	         SND_PCM_ACCESS_RW_INTERLEAVED,  // Input-output multiplexing
	         num_channels,                   // number of channels
	         samples_per_s,                  // Sample rate
	         1,                              // ?
	         (max_buf_size + 1) * duration.count())) < 0) {  // Max delay
		cerr << "Parameter setting error: " << snd_strerror(err) << endl;
		exit(1);
	}

	thread(thread_fn, chrono::steady_clock::now()).detach();
}
