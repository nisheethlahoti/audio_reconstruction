#include <alsa/asoundlib.h>
#include <soundrex/unix/runtime/lib.h>
#include <cstring>
#include <iostream>

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

void soundrex_main(slice_t<char *>) {
	snd_pcm_t *handle;
	static constexpr char device[] = "default"; /* playback device */
	std::array<sample_t, packet_samples> buffer;

	if (int err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0); err < 0)
		throw std::runtime_error(std::string("Playback open error: ") + snd_strerror(err));

	if (int err = snd_pcm_set_params(handle, pcm_format(byte_depth), SND_PCM_ACCESS_RW_INTERLEAVED,
	                                 num_channels, samples_per_s, 1,
	                                 packet_samples * 2000000ull / samples_per_s);
	    err < 0)
		throw std::runtime_error(std::string("Parameter setting error: ") + snd_strerror(err));

	snd_pcm_prepare(handle);
	do {
		snd_pcm_readi(handle, buffer.data(), buffer.size());
	} while (wrap_error(buf_write(&buffer, sizeof(buffer)), "writing recorded samples"));
	snd_pcm_close(handle);
	std::clog << "audio interface closed\n";
}
