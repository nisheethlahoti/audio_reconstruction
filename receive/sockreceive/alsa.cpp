#include <alsa/asoundlib.h>
#include <cstdint>
#include <iostream>

using namespace std;

static char const device[] = "default"; /* playback device */
static snd_pcm_t *handle;

static constexpr snd_pcm_format_t pcm_format(int const byte_depth) {
	switch (byte_depth) {
		case 1:
			return SND_PCM_FORMAT_S8;
		case 2:
			return SND_PCM_FORMAT_S16_LE;
		case 3:
			return SND_PCM_FORMAT_S24_3LE;
		case 4:
			return SND_PCM_FORMAT_S32_LE;
		default:
			return SND_PCM_FORMAT_UNKNOWN;
	}
}

void play_samples(void const *samples, size_t len) {
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, samples, len);

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

void initialize(int const num_channels, int const byte_depth,
                uint32_t const sample_rate, size_t const batch_size) {
	int err;
	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		cerr << "Playback open error: " << snd_strerror(err) << endl;
		exit(1);
	}

	if ((err = snd_pcm_set_params(
	         handle,                  // handle
	         pcm_format(byte_depth),  // Format of individual channel samples
	         SND_PCM_ACCESS_RW_INTERLEAVED,  // Input-output multiplexing
	         num_channels,                   // number of channels
	         sample_rate,                    // Sample rate
	         1,                              // ?
	         batch_size * 2000000ull / sample_rate)) < 0) {  // Max delay
		cerr << "Parameter setting error: " << snd_strerror(err) << endl;
		exit(1);
	}
}
