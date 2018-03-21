#include <alsa/asoundlib.h>
#include <signal.h>
#include <soundrex/processor.h>
#include <unix/soundrex/main.h>
#include <iostream>
#include <thread>

static processor_t processor;
snd_pcm_t *handle;
static constexpr char device[] = "default"; /* playback device */

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

static void toggle_corrections(int sigtype) {
	std::cerr << (processor.toggle_corrections() ? "Correcting\n" : "Not correcting\n");
}

static void playing_loop() {
	snd_pcm_prepare(handle);
	auto time = std::chrono::steady_clock::now();
	while (std::cin) {
		processor.play_next();
		std::this_thread::sleep_until(time += duration);
	}
}

void soundrex_main(slice_t<char *>) {
	struct sigaction action {};
	action.sa_handler = toggle_corrections;
	action.sa_flags = SA_RESTART;
	sigaction(SIGURG, &action, nullptr);  // Toggles on receiving SIGURG

	if (int err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0); err < 0)
		throw std::runtime_error(std::string("Playback open error: ") + snd_strerror(err));

	if (int err = snd_pcm_set_params(
	        handle, pcm_format(byte_depth), SND_PCM_ACCESS_RW_INTERLEAVED, num_channels,
	        samples_per_s, 1,
	        std::chrono::duration_cast<std::chrono::microseconds>(2 * duration).count());
	    err < 0)  // Max delay
		throw std::runtime_error(std::string("Parameter setting error: ") + snd_strerror(err));

	std::thread player(playing_loop);
	std::array<uint8_t, sizeof(packet_t)> buf;
	while (std::cin.read(reinterpret_cast<char *>(buf.data()), buf.size()))
		processor.process(buf.data());

	player.join();
}
