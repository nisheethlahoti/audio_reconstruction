#include <alsa/asoundlib.h>
#include <soundrex/unix/runtime/lib.h>
#include <cstring>
#include <iostream>

snd_pcm_t *capture_handle;

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

void init_record() {
	int err;
	unsigned int rate = samples_per_s;
	static const char device[] = "plughw:1";
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_format_t format = pcm_format(byte_depth);

	if ((err = snd_pcm_open(&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		std::cerr << "cannot open audio device " << device << " (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "audio interface opened\n";

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		std::cerr << "cannot allocate hardware parameter structure (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "hw_params allocated\n";

	if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
		std::cerr << "cannot initialize hardware parameter structure (" << snd_strerror(err)
		          << ")\n";
		exit(1);
	}

	std::clog << "hw_params initialized\n";

	if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params,
	                                        SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		std::cerr << "cannot set access type (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "hw_params access setted\n";

	if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
		std::cerr << "cannot set sample format (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "hw_params format setted\n";

	if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
		std::cerr << "cannot set sample rate (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "hw_params rate setted\n";

	if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, num_channels)) < 0) {
		std::cerr << "cannot set channel count (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "hw_params channels setted\n";

	if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
		std::cerr << "cannot set parameters (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "hw_params setted\n";

	snd_pcm_hw_params_free(hw_params);

	std::clog << "hw_params freed\n";

	if ((err = snd_pcm_prepare(capture_handle)) < 0) {
		std::cerr << "cannot prepare audio interface for use (" << snd_strerror(err) << ")\n";
		exit(1);
	}

	std::clog << "audio interface prepared\n";
}

std::array<sample_t, packet_samples> buffer;

void soundrex_main(slice_t<char *>) {
	init_record();
	do {
		snd_pcm_readi(capture_handle, buffer.data(), buffer.size());
	} while (wrap_error(buf_write(&buffer, sizeof(buffer)), "writing recorded samples"));
	snd_pcm_close(capture_handle);
	std::clog << "audio interface closed\n";
}
