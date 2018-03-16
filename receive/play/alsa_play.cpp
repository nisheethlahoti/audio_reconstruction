#include <alsa/asoundlib.h>
#include <iostream>

#include "../src/receive.h"
#include "../../realtime.h"

static constexpr size_t num_samples = packet_samples;
static constexpr size_t fill_samples = packet_samples / 2;
static constexpr char device[] = "default"; /* playback device */

static snd_pcm_t *handle;
static sample_t samples[std::max(num_samples, fill_samples)];

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

static void play_samples(size_t const len) {
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, samples, len);
	if (frames < 0) {
		snd_pcm_recover(handle, frames, 0);
	} else if (frames < len) {
		std::cerr << "Short write (expected " << len << ", wrote " << frames << ")\n";
	}
}

int main() {
	std::ios::sync_with_stdio(false);
	set_realtime(2, 1);

	int err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		std::cerr << "Playback open error: " << snd_strerror(err) << std::endl;
		return 1;
	}

	if ((err = snd_pcm_set_params(handle,                  // handle
	                              pcm_format(byte_depth),  // Format of individual channel samples
	                              SND_PCM_ACCESS_RW_INTERLEAVED,  // Input-output multiplexing
	                              num_channels,                   // number of channels
	                              samples_per_s,                  // Sample rate
	                              1,                              // ?
	                              packet_samples * 2000000ull / samples_per_s)) < 0) {  // Max delay
		std::cerr << "Parameter setting error: " << snd_strerror(err) << std::endl;
		return 1;
	}

	std::cin.peek();
	snd_pcm_prepare(handle);
	play_samples(fill_samples);

	while (std::cin.read(reinterpret_cast<char *>(samples), num_samples * sizeof samples[0]))
		play_samples(num_samples);

	return 0;
}
