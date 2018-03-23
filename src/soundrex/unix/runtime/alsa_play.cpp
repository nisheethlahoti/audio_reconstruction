#include <alsa/asoundlib.h>
#include <soundrex/unix/runtime/lib.h>
#include <iostream>

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
	snd_pcm_sframes_t frames;
	while (frames = snd_pcm_writei(handle, samples, len), frames <= 0)
		snd_pcm_recover(handle, frames, 0);
	if (frames < len)
		std::cerr << "Short write (expected " << len << ", wrote " << frames << ")" << std::endl;
}

void soundrex_main(slice_t<char *>) {
	if (int err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0); err < 0)
		throw std::runtime_error(std::string("Playback open error: ") + snd_strerror(err));

	if (int err = snd_pcm_set_params(handle, pcm_format(byte_depth), SND_PCM_ACCESS_RW_INTERLEAVED,
	                                 num_channels, samples_per_s, 1,
	                                 packet_samples * 2000000ull / samples_per_s);
	    err < 0)
		throw std::runtime_error(std::string("Parameter setting error: ") + snd_strerror(err));

	wait_for_input();
	snd_pcm_prepare(handle);
	play_samples(fill_samples);

	while (buf_read_blocking(samples, num_samples * sizeof samples[0]))
		play_samples(num_samples);
}
